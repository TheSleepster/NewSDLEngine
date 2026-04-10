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
    EF_Valid     = 1ul << 0,
    EF_Alive     = 1ul << 1,
    EF_Gravitic  = 1ul << 2,
    EF_Actor     = 1ul << 3,
    EF_Static    = 1ul << 4,
    EF_IsGround  = 1ul << 5,
    EF_HasSprite = 1ul << 6,
};

// NOTE(Sleepster): owner_client_id is used to assign ownership of an entity 
// to that of a specific client 
struct entity_t
{
    u32             e_type;
    u32             e_flags;
    vec2_t          last_position;
    vec2_t          render_position;
    vec2_t          position;
    vec2_t          size;
    vec2_t          velocity;

    float32         rotation;
    asset_handle_t *sprite;
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
entity_create(game_state_t *game_state)
{
    Assert(game_state->entity_manager);

    entity_t *result = null;
    entity_t *found  = game_state->entity_manager->entities + game_state->entity_manager->active_entities;
    if((found->e_flags & EF_Valid) == 0)
    {
        result = found;
        result->e_flags = EF_Valid;

        ++game_state->entity_manager->active_entities;
    }

    Assert(result);
    return(result);
}

void
entity_render(game_state_t *game_state, render_command_list_t *command_list, entity_t *entity)
{
    render_vertex_t *vertex_pointer = game_state->vertex_data + game_state->vertex_count;

    render_vertex_t *bottom_right = vertex_pointer + 0;
    render_vertex_t *top_right    = vertex_pointer + 1;
    render_vertex_t *top_left     = vertex_pointer + 2;
    render_vertex_t *bottom_left  = vertex_pointer + 3;

    float32 top    = entity->render_position.y + entity->size.y;
    float32 bottom = entity->render_position.y;
    float32 left   = entity->render_position.x;
    float32 right  = entity->render_position.x + entity->size.x;

    bottom_left->vPosition  = vec4(left,  bottom, 0, 1);
    bottom_right->vPosition = vec4(right, bottom, 0, 1);
    top_left->vPosition     = vec4(left,  top,    0, 1);
    top_right->vPosition    = vec4(right, top,    0, 1);

    bottom_left->vColor  = vec4(1.0, 1.0, 1.0, 1.0);
    bottom_right->vColor = vec4(1.0, 1.0, 1.0, 1.0);
    top_left->vColor     = vec4(1.0, 1.0, 1.0, 1.0);
    top_right->vColor    = vec4(1.0, 1.0, 1.0, 1.0);

    if(entity->e_flags & EF_HasSprite)
    {
        // TODO(Sleepster): This should be two different valid code paths for if the sprite is atlased or not  
        subtexture_data_t *data = entity->sprite->subtexture_data;
        if(data)
        {
            asset_handle_t texture_handle = {};
            texture_handle.type = AT_Bitmap;
            texture_handle.texture = &entity->sprite->subtexture_data->atlas->texture;

            r_cmd_bind_texture(command_list, &texture_handle);
            float32 tbottom = data->uv_max.y;
            float32 ttop    = data->uv_min.y;
            float32 tleft   = data->uv_min.x;
            float32 tright  = data->uv_max.x;

            bottom_left->vTexCoord  = vec2(tleft,  tbottom);
            bottom_right->vTexCoord = vec2(tright, tbottom);
            top_left->vTexCoord     = vec2(tleft,  ttop);
            top_right->vTexCoord    = vec2(tright, ttop);
        }
    }

    game_state->vertex_count += 4;
}

int
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
        .width  = 320,
        .height = 180,
        .format = BMF_BGRA32_UNORM
    };

    image_create_info_t primary_game_depth_buffer_create_info = {
        .width  = 320,
        .height = 180,
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
        .render_width           = 320,
        .render_height          = 180,
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

    render_buffer_t vertex_buffer = s_renderer_vertex_buffer_create(renderer_state, RenderBufferAllocationTypeMapped, null,     sizeof(render_vertex_t) * (4 * MAX_ENTITIES));
    render_buffer_t index_buffer  = s_renderer_index_buffer_create(renderer_state,  RenderBufferAllocationTypeMapped, indices, (sizeof(u32) * (6 * MAX_ENTITIES)));
    game_state.vertex_data        = c_arena_push_array(&renderer_state->renderer_arena, render_vertex_t, 4 * 10000);

    uniform_constant_buffer_t *camera_matrices_buffer = s_renderer_get_constant_buffer(renderer_state, STR("CameraMatrices"));
    asset_handle_t basic_triangle = s_asset_manager_acquire_asset_handle(asset_manager, STR("basic_triangle"));
    asset_handle_t player_sprite  = s_asset_manager_acquire_asset_handle(asset_manager, STR("player"));
    asset_handle_t basic_font     = s_asset_manager_acquire_asset_handle(asset_manager, STR("LiberationMono_Regular"));

    dynamic_render_font_varient_t *font_data = s_asset_font_acquire_font_at_size(asset_manager, &basic_font, 16);

    u8 character = 'A';
    glyph_metric_t *metric = s_asset_font_fetch_glyph(asset_manager, font_data, &character);
    (void)metric;

    entity_t *player =  entity_create(&game_state);
    player->sprite   = &player_sprite;
    player->e_flags |= EF_HasSprite;
    player->size = vec2(20, 20);

    texture_atlas_t *atlas = s_texture_atlas_create(asset_manager, 1024, 4, BMF_RGBA32_SRGB, 32);
    s_texture_atlas_add_texture(atlas, &player_sprite);
    s_texture_atlas_pack_added_textures(asset_manager, atlas);

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    u64 last_tsc        = SDL_GetPerformanceCounter();
    u64 current_tsc     = 0;
    u64 delta_tsc       = 0;

    float32 delta_time    = 0;
    float64 dt_accumulator = 0.0f;
    //float32 delta_time_ms = 0;
    while(global_context->running)
    {
        process_window_events(global_context->renderer_state, input_manager);

        vec2_t input_axis = {};
        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_W))
        {
            input_axis.y += 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_A))
        {
            input_axis.x -= 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_S))
        {
            input_axis.y -= 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_D))
        {
            input_axis.x += 1.0f;
        }

        const float64 TICK_RATE = 1.0 / 60.0;
        if(delta_time >= (TICK_RATE * 2.0f))
        {
            delta_time = TICK_RATE * 2.0f;
        }

        dt_accumulator += delta_time;
        while(dt_accumulator >= TICK_RATE)
        {
            input_axis = vec2_normalize(input_axis);
            player->last_position = player->position;
            player->position = vec2_add(player->position, vec2_scale(vec2_scale(input_axis, 250), TICK_RATE));

            dt_accumulator -= TICK_RATE;
        }

        float32 alpha = (float32)(dt_accumulator / TICK_RATE);
        render_command_list_t *command_list = s_renderer_get_command_list(renderer_state);
        for(u32 entity_index = 0;
            entity_index < game_state.entity_manager->active_entities;
            ++entity_index)
        {
            entity_t *entity = game_state.entity_manager->entities + entity_index;
            Assert(entity->e_flags & EF_Valid);

            player->render_position = vec2_lerp(player->last_position, player->position, alpha);
            entity_render(&game_state, command_list, entity);
        }

        // TODO(Sleepster): This should just be a command, we can store the vertex data for this buffer using a transient frame allocator and just bind the data
        // later on.
        s_renderer_render_buffer_copy_data(renderer_state, &vertex_buffer, game_state.vertex_data, Align16(game_state.vertex_count * sizeof(render_vertex_t)), 0);

        r_cmd_renderpass_begin(command_list, game_renderpass_ID);
        r_cmd_bind_vertex_buffer(command_list, &vertex_buffer);
        r_cmd_bind_index_buffer(command_list, &index_buffer);
        r_cmd_use_shader_program(command_list, basic_triangle);

        struct camera_matrices {
            mat4_t view_matrix;
            mat4_t projection_matrix;
        }camera_matrix_buffer_data;

        s32 window_width  = Max(game_renderpass_desc.render_width, 10);
        s32 window_height = Max(game_renderpass_desc.render_height, 10);

        camera_matrix_buffer_data = {
            .view_matrix       = mat4_identity(),
            .projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1)
        };
        r_cmd_update_buffer_contents(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

        r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
        r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

        r_cmd_draw_indexed(command_list, game_state.entity_manager->active_entities * 6, 0, 1, 0);

        r_cmd_renderpass_end(command_list);
        r_cmd_present(command_list, &game_color_buffer);

        s_renderer_execute_backend_commands(renderer_state);
        s_asset_manager_update(asset_manager);

        c_global_context_reset_temporary_data();

        game_state.vertex_count   = 0;
        vertex_buffer.buffer.used = 0;

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time    = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

    return(0);
}
