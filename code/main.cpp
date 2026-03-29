/* ========================================================================
   $File: main.cpp $
   $Date: March 27 2026 06:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
// NOTE(Sleepster): For srand() and rand()
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_global_context.h>
#include <c_zone_allocator.h>
#include <c_program_flag_handler.h>
#include <c_tokenizer.h>
#include <p_platform_data.h>

#include <r_render_image.h>
#include <s_render_RHI.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>

void process_window_events(renderer_state_t *renderer_state, input_manager_t *input_manager);

#define MAX_ENTITIES (100000)

enum entity_type 
{
    ET_Invalid,
    ET_Player,
    ET_Count
};

enum entity_flags
{
    EF_Valid    = 1ul << 0,
    EF_Alive    = 1ul << 1,
    EF_Gravitic = 1ul << 2,
    EF_Actor    = 1ul << 3,
    EF_Static   = 1ul << 4,
    EF_IsGround = 1ul << 5,
};

// NOTE(Sleepster): owner_client_id is used to assign ownership of an entity 
// to that of a specific client 
struct entity_t
{
    u32     e_type;
    u32     e_flags;
    u32     owner_client_id;
    vec2_t  last_position;
    vec2_t  position;
    vec2_t  size;
    vec2_t  velocity;

    float32 rotation;
};

struct entity_manager_t
{
    entity_t entities[MAX_ENTITIES];
    u32      active_entities;
};

struct game_state_t
{
    input_controller_t *controller;
    entity_manager_t   *entity_manager;

    image_t             game_color_buffer;
    image_t             game_depth_buffer;

    render_buffer_t     vertex_buffer;
    render_buffer_t     index_buffer;

    render_vertex_t    *vertex_data;
    u32                 vertex_count;

    u32                 game_renderpass_ID;
};

entity_t*
create_entity(game_state_t *game_state)
{
    Assert(game_state->entity_manager);

    entity_t *result = null;
    entity_t *found  = game_state->entity_manager->entities + game_state->entity_manager->active_entities;
    if((found->e_flags & EF_Valid) == 0)
    {
        result = found;
        result->e_flags |= EF_Valid;

        ++game_state->entity_manager->active_entities;
    }

    Assert(result);
    return(result);
}

vec4_t colors[] = {
    {1.0, 1.0, 1.0, 1.0},
    {1.0, 0.0, 0.0, 1.0},
    {0.0, 1.0, 0.0, 1.0},
    {0.0, 0.0, 1.0, 1.0},
    {1.0, 1.0, 0.0, 1.0},
    {0.0, 1.0, 1.0, 1.0},
    {1.0, 0.0, 1.0, 1.0},
    {0.5, 0.5, 0.5, 1.0},
    {0.0, 0.5, 1.0, 1.0},
    {0.0, 0.5, 0.0, 1.0},
    {1.0, 1.0, 1.0, 1.0},
};

static u32 next_color_idx = 0;

void
draw_entity(game_state_t *game_state, vec2_t position, vec2_t size)
{
    render_vertex_t *vertex_ptr = game_state->vertex_data + game_state->vertex_count;
    render_vertex_t *bottom_right = vertex_ptr + 0;
    render_vertex_t *top_right    = vertex_ptr + 1;
    render_vertex_t *top_left     = vertex_ptr + 2;
    render_vertex_t *bottom_left  = vertex_ptr + 3;

    bottom_right->vPosition = vec2_expand_vec4(vec2(position.x + size.x, position.y), 0.0, 1.0);
    bottom_right->vColor    = colors[(next_color_idx + 1) % ArrayCount(colors)];

    top_right->vPosition    = vec2_expand_vec4(vec2_add(position, size), 0.0, 1.0);
    top_right->vColor       = colors[(next_color_idx + 1) % ArrayCount(colors)];

    top_left->vPosition     = vec2_expand_vec4(vec2(position.x, position.y + size.y), 0.0, 1.0);
    top_left->vColor        = colors[(next_color_idx + 1) % ArrayCount(colors)];

    bottom_left->vPosition  = vec2_expand_vec4(position, 0.0, 1.0);
    bottom_left->vColor     = colors[(next_color_idx + 1) % ArrayCount(colors)];

    ++next_color_idx;

    game_state->vertex_count += 4;
}

void
game_main(void)
{
    game_state_t game_state = {};
    srand(rdtsc());

    input_manager_t *input_manager   = global_context->input_manager;
    renderer_state_t *renderer_state = global_context->renderer_state;
    asset_manager_t *asset_manager   = global_context->asset_manager;

    game_state.controller = s_im_get_primary_controller(global_context->input_manager);
    game_state.entity_manager = c_arena_push_struct(&global_context->context_arena, entity_manager_t);

    image_create_info_t primary_game_color_buffer_create_info = {
        .width  = 2560,
        .height = 1440,
        .format = BMF_BGRA32_UNORM
    };

    image_create_info_t primary_game_depth_buffer_create_info = {
        .width  = 2560,
        .height = 1440,
        .format = BMF_D32_SFLOAT_S8_UINT 
    };

    image_t game_color_buffer = s_renderer_image_create(renderer_state, &primary_game_color_buffer_create_info);
    image_t game_depth_buffer = s_renderer_image_create(renderer_state, &primary_game_depth_buffer_create_info);

    clear_value_t color_buffer_clear_value = {
        .clear_color = {.float_color = {0.3f, 0.4f, 0.6f, 1.0f}},
    };

    clear_value_t depth_buffer_clear_value = {
        .clear_depth = 0.0f
    };

    renderpass_desc_t game_renderpass_desc = {
        .render_width           = 2560,
        .render_height          = 1440,
        .resize_with_window     = false,
        .color_attachment_count = 1,
        .color_attachments = {
            [0] = {
                .access          = RenderpassAttachmentAccessWrite,
                .load_operation  = RenderpassAttachmentLoadOperationClear,
                .store_operation = RenderpassAttachmentStoreOperationStore,

                .image           = &game_color_buffer,
                .clear_value     = color_buffer_clear_value
            },
        },
        .depth_stencil_attachment = {
            .access          = RenderpassAttachmentAccessWrite,
            .load_operation  = RenderpassAttachmentLoadOperationClear,
            .store_operation = RenderpassAttachmentStoreOperationDontCare,

            .image           = &game_depth_buffer,
            .clear_value     = depth_buffer_clear_value
        },
    };

    u32 game_renderpass_ID = s_renderer_build_renderpass(renderer_state, &game_renderpass_desc);
    u32 *indices = c_arena_push_array(&renderer_state->renderer_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
    u32  index_offset = 0;
    for(u32 index = 0;
        index < 60000;
        index += 6)
    {
        indices[index + 0] = index_offset + 0;
        indices[index + 1] = index_offset + 1;
        indices[index + 2] = index_offset + 2;
        indices[index + 3] = index_offset + 2;
        indices[index + 4] = index_offset + 3;
        indices[index + 5] = index_offset + 0;

        index_offset += 4;
    }

    render_buffer_t vertex_buffer = s_renderer_vertex_buffer_create(renderer_state, RenderBufferUsage_Dynamic, null,    sizeof(render_vertex_t) * (4 * MAX_ENTITIES));
    render_buffer_t index_buffer  = s_renderer_index_buffer_create(renderer_state,  RenderBufferUsage_Dynamic, indices, (sizeof(u32) * (6 * MAX_ENTITIES)));
    game_state.vertex_data        = c_arena_push_array(&renderer_state->renderer_arena, render_vertex_t, 4 * 10000);

    uniform_constant_buffer_t *camera_matrices_buffer = s_renderer_get_constant_buffer(renderer_state, STR("CameraMatrices"));
    asset_handle_t basic_triangle = s_asset_manager_acquire_asset_handle(asset_manager, STR("basic_triangle"));
    
    //vec2_t min_pos = vec2(0, 0);
    //vec2_t min_size = vec2(0, 0);
    
    vec2_t max_pos = vec2(2560, 1440);
    vec2_t max_size = vec2(200, 200);

    for(u32 entity_index = 0;
        entity_index < MAX_ENTITIES;
        ++entity_index)
    {
        entity_t *new_entity = create_entity(&game_state);
        
        float32 pos_x  = (float32)(rand() % (u32)max_pos.x);
        float32 pos_y  = (float32)(rand() % (u32)max_pos.y);
        float32 size_x = (float32)(rand() % (u32)max_size.x);
        float32 size_y = (float32)(rand() % (u32)max_size.y);

        if(entity_index % 2 == 0)
        {
            pos_x *= -1;
        }
        if(entity_index % 3 == 0)
        {
            pos_y *= -1;
        }

        new_entity->position = vec2(pos_x,  pos_y);
        new_entity->size     = vec2(size_x > 0 ? size_x : 10, size_y > 0 ? size_y : 10);
    }

    while(global_context->running)
    {
        next_color_idx = 0;

        process_window_events(global_context->renderer_state, input_manager);
        for(u32 entity_index = 0;
            entity_index < MAX_ENTITIES;
            ++entity_index)
        {
            entity_t *entity = game_state.entity_manager->entities + entity_index;

            vec2_t position = vec2_rotate(entity->position, entity->rotation);
            vec2_t size     = entity->size;

            entity->rotation += 0.001;

            draw_entity(&game_state, position, size);
        }
        s_renderer_render_buffer_copy_data(renderer_state, &vertex_buffer, game_state.vertex_data, game_state.vertex_count * sizeof(render_vertex_t), 0);
        game_state.vertex_count = 0;

        render_command_list_t *command_list = s_renderer_get_command_list(renderer_state);
        r_cmd_renderpass_begin(command_list, game_renderpass_ID);
        r_cmd_bind_vertex_buffer(command_list, &vertex_buffer);
        r_cmd_bind_index_buffer(command_list, &index_buffer);
        r_cmd_use_shader_program(command_list, basic_triangle);

        struct camera_matrices {
            mat4_t view_matrix;
            mat4_t projection_matrix;
        }camera_matrix_buffer_data;

        s32 window_width  = Max(renderer_state->window_size.x, 10);
        s32 window_height = Max(renderer_state->window_size.y, 10);

        s32 half_window_width  = renderer_state->window_size.x * 0.5;
        s32 half_window_height = renderer_state->window_size.y * 0.5;
        camera_matrix_buffer_data = {
            .view_matrix       = mat4_identity(),
            .projection_matrix = mat4_RHGL_ortho(-half_window_width, half_window_height, -half_window_width, half_window_height, -1, 1)
        };
        r_cmd_update_buffer_contents(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

        r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
        r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

        r_cmd_draw_indexed(command_list, game_state.entity_manager->active_entities * 6, 0, 1, 0);

        r_cmd_renderpass_end(command_list);
        r_cmd_present(command_list, &game_color_buffer);

        s_renderer_execute_backend_commands(renderer_state);
        c_global_context_reset_temporary_data();

        vertex_buffer.buffer.used = 0;
    }

    return;
}
