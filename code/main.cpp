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
#include <r_immediate_rendering.h>
#include <s_render_RHI.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>
#include <s_ui_core.h>

#define MAX_ENTITIES (100000)

void process_window_events(renderer_state_t *renderer_state, input_manager_t *input_manager);

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

    image_t             fullscreen_color_buffer;
    image_t             fullscreen_depth_buffer;

    vertex_buffer_t     vertex_buffer;
    render_buffer_t     index_buffer;

    u32                 game_renderpass_ID;
    u32                 fullscreen_renderpass_ID; 
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
    texture2D_t *texture = null;
    vec2_t       uv_min  = vec2_zero();
    vec2_t       uv_max  = vec2_zero();

    if(entity->e_flags & EF_HasSprite)
    {
        subtexture_data_t *data = entity->sprite->subtexture_data;
        if(data)
        {
            texture = &data->atlas->texture;
            uv_min  =  data->uv_min;
            uv_max  =  data->uv_max;
        }
        else
        {
            texture = entity->sprite->texture;
            uv_min  = vec2(0.0f, 0.0f);
            uv_max  = vec2(1.0f, 1.0f);
        }
    }
    
    immediate_quad_ex(command_list,
                     &game_state->vertex_buffer, 
                      vec2_expand_vec3(entity->position, 0.8f), 
                      entity->size, 
                      vec4(1.0, 1.0, 1.0, 1.0),
                      uv_min,
                      uv_max,
                      vec2_zero(),
                      texture);
}

int
game_main(void)
{
    game_state_t game_state = {};
    srand(rdtsc());

    input_manager_t  *input_manager  = global_context->input_manager;
    renderer_state_t *renderer_state = global_context->renderer_state;
    asset_manager_t  *asset_manager  = global_context->asset_manager;
    ui_state_t       *main_ui        = Alloc(ui_state_t);

    game_state.controller = s_im_get_primary_controller(global_context->input_manager);
    game_state.entity_manager = c_arena_push_struct(&global_context->context_arena, entity_manager_t);

    // NOTE(Sleepster): Clear colors 
    clear_value_t color_buffer_clear_value = {
        .float_color = {0.1f, 0.1f, 0.8f, 1.0f},
    };

    clear_value_t depth_buffer_clear_value = {
        .depth   = 1.0f,
        .stencil = 0
    };

    // NOTE(Sleepster): Game Renderpass
    renderpass_desc_t game_renderpass_desc;
    game_state.game_color_buffer = {};
    game_state.game_depth_buffer = {};
    {
        image_create_info_t primary_game_color_buffer_create_info = {
            .width  = 320,
            .height = 180,
            .format = BMF_RGBA32_UNORM,
            .usage  = IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT
        };

        image_create_info_t primary_game_depth_buffer_create_info = {
            .width  = 320,
            .height = 180,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT 
        };

        game_state.game_color_buffer = s_renderer_image_create(renderer_state, &primary_game_color_buffer_create_info);
        game_state.game_depth_buffer = s_renderer_image_create(renderer_state, &primary_game_depth_buffer_create_info);

        game_renderpass_desc = {
            .render_width           = 320,
            .render_height          = 180,
            .resize_with_window     = false,
            .color_attachment_count = 1,
            .color_attachments = {
                [0] = {
                    .access          = RenderpassAttachmentAccessWrite,
                    .load_operation  = RenderpassAttachmentLoadOperationClear,
                    .store_operation = RenderpassAttachmentStoreOperationStore,

                    .image           = &game_state.game_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RenderpassAttachmentAccessWrite,
                .load_operation  = RenderpassAttachmentLoadOperationClear,
                .store_operation = RenderpassAttachmentStoreOperationStore,

                .image           = &game_state.game_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    // NOTE(Sleepster): Fullscreen Renderpass
    renderpass_desc_t fullscreen_renderpass_desc;
    game_state.fullscreen_color_buffer = {};
    game_state.fullscreen_depth_buffer = {};
    {
        image_create_info_t fullscreen_color_buffer_create_info = {
            .width  = (u32)renderer_state->window_size.x,
            .height = (u32)renderer_state->window_size.y,
            .format = BMF_RGBA32_UNORM,
            .usage  = (render_image_usage_t)(IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT|IMAGE_USAGE_BLIT_SOURCE),
        };

        image_create_info_t fullscreen_depth_buffer_create_info = {
            .width  = (u32)renderer_state->window_size.x,
            .height = (u32)renderer_state->window_size.y,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT,
        };

        game_state.fullscreen_color_buffer = s_renderer_image_create(renderer_state, &fullscreen_color_buffer_create_info);
        game_state.fullscreen_depth_buffer = s_renderer_image_create(renderer_state, &fullscreen_depth_buffer_create_info);

        fullscreen_renderpass_desc = {
            .render_width           = (u32)renderer_state->window_size.x,
            .render_height          = (u32)renderer_state->window_size.y,
            .resize_with_window     = true,
            .color_attachment_count = 1,
            .color_attachments = {
                [0] = {
                    .access          = RenderpassAttachmentAccessWrite,
                    .load_operation  = RenderpassAttachmentLoadOperationLoad,
                    .store_operation = RenderpassAttachmentStoreOperationStore,

                    .image           = &game_state.fullscreen_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RenderpassAttachmentAccessWrite,
                .load_operation  = RenderpassAttachmentLoadOperationClear,
                .store_operation = RenderpassAttachmentStoreOperationStore,

                .image           = &game_state.fullscreen_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    game_state.game_renderpass_ID       = s_renderer_build_renderpass(renderer_state, &game_renderpass_desc);
    game_state.fullscreen_renderpass_ID = s_renderer_build_renderpass(renderer_state, &fullscreen_renderpass_desc);

    ui_state_init(main_ui, input_manager, asset_manager, renderer_state, game_state.fullscreen_renderpass_ID);

    u32 *indices = c_arena_push_array(&renderer_state->transient_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
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

    const u32 VERTEX_BUFFER_SIZE = 4 * 10000;
    immediate_vertex_t *vertices = c_arena_push_array(&renderer_state->renderer_arena, immediate_vertex_t, VERTEX_BUFFER_SIZE);
    game_state.vertex_buffer = s_renderer_vertex_buffer_create(renderer_state, 
                                                               RenderBufferAllocationTypeMapped, 
                                                               RenderBufferAdvanceRate_PerElement, 
                                                               (byte*)vertices, 
                                                               sizeof(immediate_vertex_t), 
                                                               VERTEX_BUFFER_SIZE);
    game_state.index_buffer  = s_renderer_index_buffer_create(renderer_state,  
                                                              RenderBufferAllocationTypeGPUOnly, 
                                                              sizeof(u32),
                                                              indices, 
                                                              (sizeof(u32) * (6 * MAX_ENTITIES)));

    uniform_constant_buffer_t *camera_matrices_buffer = s_renderer_get_constant_buffer(renderer_state, STR("CameraMatrices"));
    asset_handle_t basic_triangle = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_textured_unnormalized"));
    asset_handle_t font_shader    = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_font"));
    asset_handle_t player_sprite  = s_asset_manager_acquire_asset_handle(asset_manager, STR("player"));
    asset_handle_t basic_font     = s_asset_manager_acquire_asset_handle(asset_manager, STR("LiberationMono_Regular"));

    entity_t *player =  entity_create(&game_state);
    player->sprite   = &player_sprite;
    player->e_flags |=  EF_HasSprite;

    player->position = vec2(0,  40);
    player->size     = vec2(20, 20);

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
        s_im_reset_controller_states(input_manager);
        process_window_events(global_context->renderer_state, input_manager);

        ui_state_begin_frame(main_ui);

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

        ui_signal_t main_panel = ui_widget_panel(main_ui, STR("Test panel..."), vec2(20, 20), vec4(0.4, 0.4, 0.4, 0.5));
        ui_parent(main_ui, main_panel.widget)
        {
            ui_widget_set_parent_layout(main_ui, WIDGET_LAYOUT_STYLE_VERTICAL);

            ui_signal_t signal = ui_widget_sized_button(main_ui, 
                                                        STR("Test button..."), 
                                                        vec2(20, 20), 0);
            if(ui_pressed(signal))
            {
                main_panel.widget->state->toggled = !main_panel.widget->state->toggled;
            }

            if(main_panel.widget->toggled)
            {
                for(u32 index = 0;
                    index < 4;
                    ++index)
                {
                    ui_widget_seed(main_ui, index);
                    ui_widget_sized_button(main_ui, 
                                           STR("Test button..."), 
                                           vec2(20, 20), 0);
                }
                ui_row(main_ui) 
                {
                    ui_widget_labeled_button(main_ui, STR("Test Label..."), vec4(0.4, 0.4, 0.4, 1.0)); 
                }
            }
        }

        // NOTE(Sleepster): Game renderpass
        render_command_list_t *command_list = s_renderer_get_command_list(renderer_state, RENDER_COMMAND_LIST_TYPE_GRAPHICS);
        {
            for(u32 entity_index = 0;
                entity_index < game_state.entity_manager->active_entities;
                ++entity_index)
            {
                entity_t *entity = game_state.entity_manager->entities + entity_index;
                Assert(entity->e_flags & EF_Valid);

                player->render_position = vec2_lerp(player->last_position, player->position, alpha);
                entity_render(&game_state, command_list, entity);
            }

            r_cmd_update_buffer_contents(command_list, &game_state.vertex_buffer);

            r_cmd_renderpass_begin(command_list,    game_state.game_renderpass_ID);
            r_cmd_bind_vertex_buffer(command_list, &game_state.vertex_buffer);
            r_cmd_bind_index_buffer(command_list,  &game_state.index_buffer);
            r_cmd_use_shader_program(command_list,  basic_triangle);

            s32 window_width  = Max(game_renderpass_desc.render_width, 10);
            s32 window_height = Max(game_renderpass_desc.render_height, 10);

            camera_matrices_t camera_matrix_buffer_data = {
                .view_matrix       = mat4_identity(),
                .projection_matrix = mat4_RHDX_ortho(-160, 160, -90, 90, -1, 1)
            };
            r_cmd_update_constant_buffer(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

            r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

            r_cmd_draw_indexed(command_list, game_state.entity_manager->active_entities * 6, 0, 1, 0);
            r_cmd_renderpass_end(command_list);

            game_state.vertex_buffer.vertex_count = 0;
        }

        // NOTE(Sleepster): Fullscreen Renderpass 
        {
            r_cmd_blit_renderpass(command_list, game_state.game_renderpass_ID, game_state.fullscreen_renderpass_ID);
            r_cmd_renderpass_begin(command_list, game_state.fullscreen_renderpass_ID);
            s32 window_width  = Max(renderer_state->window_size.x, 10);
            s32 window_height = Max(renderer_state->window_size.y, 10);

            s32 half_window_width  = window_width  * 0.5f;
            s32 half_window_height = window_height * 0.5f;

            camera_matrices_t camera_matrix_buffer_data = {
                .view_matrix       = mat4_identity(),
                .projection_matrix = mat4_RHDX_ortho(-half_window_width, half_window_width, -half_window_height, half_window_height, -1, 1)
            };
            r_cmd_update_constant_buffer(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

            render_pipeline_state_t font_state = {};
            font_state.blend_enabled = false;
            font_state.src_alpha_blend_mode = RBM_SrcAlpha;
            font_state.dst_alpha_blend_mode = RBM_OneMinusSrcAlpha;
            r_cmd_set_render_state(command_list, &font_state);

            r_cmd_use_shader_program(command_list, font_shader);
            r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

            immediate_text(command_list, 
                          &game_state.vertex_buffer, 
                           asset_manager, 
                          &basic_font, 
                          STR("This is a test string..."), 
                          vec3(-200, -100, 0.0f), 
                          vec4(1.0f, 1.0f, 1.0f, 1.0f), 
                          vec2_zero(),
                          32);

            r_cmd_update_buffer_contents(command_list, &game_state.vertex_buffer);
            r_cmd_draw_indexed(command_list, (game_state.vertex_buffer.vertex_count * 0.25f) * 6, 0, 1, 0);

            ui_state_end_frame(main_ui, command_list);

            r_cmd_renderpass_end(command_list);
        }
        r_cmd_present(command_list, &game_state.fullscreen_color_buffer);

        s_renderer_execute_backend_commands(renderer_state);
        s_renderer_buffer_reset(renderer_state, &game_state.vertex_buffer);

        s_asset_manager_update(asset_manager);
        c_global_context_reset_temporary_data();

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

    return(0);
}
