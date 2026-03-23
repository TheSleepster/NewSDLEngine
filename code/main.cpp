/* ========================================================================
   $File: main.c $
   $Date: November 28 2025 06:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#define MATH_IMPLEMENTATION
#define HASH_TABLE_IMPLEMENTATION
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_globals.h>
#include <c_zone_allocator.h>
#include <c_program_flag_handler.h>
#include <c_tokenizer.h>
#include <p_platform_data.h>

#if 0
#include <r_vulkan_types.h>
#include <r_vulkan_core.h>
#include <r_render_group.h>
#endif

#include <vk_backend_core.h>
#include <r_render_image.h>
#include <s_renderer.h>

#include <s_nt_networking.h>
#include <s_input_manager.h>
#include <s_asset_manager.h>
#include <g_game_state.h>
#include <g_entity.h>

#include <asset_file_packer/jfd_asset_file.h>
//#include <meta/GENERATED_program_RTTI.h>

internal_api void
process_window_events(SDL_Window *window, renderer_state_t *renderer_state, input_manager_t *input_manager)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        s_im_handle_window_inputs(&event, input_manager);
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                g_running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                s32 window_x = 0;
                s32 window_y = 0;
                SDL_GetWindowSizeInPixels(window, &window_x, &window_y);

                g_window_size.x = (float32)window_x;
                g_window_size.y = (float32)window_y;
#if 0
                r_vulkan_on_resize(render_context, g_window_size);
#endif
                s_renderer_handle_window_resize(renderer_state, g_window_size);
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    game_state_t            *state          = Alloc(game_state_t);
    asset_manager_t         *asset_manager  = Alloc(asset_manager_t);
#if 0
    vulkan_render_context_t *render_context = Alloc(vulkan_render_context_t);
    render_state_t          *render_state   = Alloc(render_state_t);
#endif

    state->window_size = vec2(600, 600);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        state->window = SDL_CreateWindow("Vulkan...", 
                                         state->window_size.x,
                                         state->window_size.y, 
                                         SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
        if(state->window == null)
        {
            log_fatal("Could not create SDL window... Error: '%s'...\n", SDL_GetError());
        }
        c_global_context_init();

        vulkan_context_t context = {};
        vk_backend_init(&context, state->window);
        c_threadpool_init(&global_context->main_threadpool);

        s_asset_manager_init(asset_manager);
        s_asset_manager_load_asset_file(asset_manager, STR("asset_data.jfd"));

        asset_manager->vulkan_context = &context;

        asset_handle_t default_texture  = s_asset_manager_acquire_asset_handle(asset_manager, STR("player"));
        asset_handle_t default_shader   = s_asset_manager_acquire_asset_handle(asset_manager, STR("test_shader"));
        asset_handle_t basic_traiangle  = s_asset_manager_acquire_asset_handle(asset_manager, STR("basic_triangle"));
        asset_handle_t default_material = s_asset_manager_acquire_asset_handle(asset_manager, STR("test_material_archetype"));
        (void)default_shader;
        (void)default_material;

        texture_atlas_t *atlas = s_texture_atlas_create(asset_manager, 1024, 4, BMF_RGBA32_SRGB, 32);
        s_texture_atlas_add_texture(atlas, &default_texture);
        s_texture_atlas_pack_added_textures(&context, atlas);

        s_nt_socket_api_init(state, argc, argv);

        input_manager_t input_manager = {};
        s_im_init_input_manager(&input_manager);
        input_controller_t *game_controller = s_im_get_primary_controller(&input_manager);

        u64 perf_count_freq = SDL_GetPerformanceFrequency();
        u64 last_tsc        = SDL_GetPerformanceCounter();
        u64 current_tsc     = 0;
        u64 delta_tsc       = 0;

        //float32 delta_time_ms  = 0;
        float32 delta_time     = 0;
        float64 dt_accumulator = 0.0f;

        renderer_state_t renderer_state = {};
        s_renderer_state_init(&renderer_state, &context);

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

        image_t game_color_buffer = s_renderer_image_create(&renderer_state, &primary_game_color_buffer_create_info);
        image_t game_depth_buffer = s_renderer_image_create(&renderer_state, &primary_game_depth_buffer_create_info);

        clear_value_t color_buffer_clear_value = {
            .clear_color = {.float_color = {0.3f, 0.4f, 0.6f}},
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

        u32 game_renderpass_ID = s_renderer_build_renderpass(&renderer_state, &game_renderpass_desc);

        render_vertex_t vertices[] = {
            [0] = {
                .vPosition = vec4( 80, -45, 0.0, 1.0),
            },
            [1] = {
                .vPosition = vec4( 80,  45, 0.0, 1.0),
            },
            [2] = {
                .vPosition = vec4(-80,  45, 0.0, 1.0),
            },
            [3] = {
                .vPosition = vec4(-80, -45, 0.0, 1.0),
            }
        };
        render_buffer_t vertex_buffer = s_renderer_vertex_buffer_create(&renderer_state, RenderBufferUsage_Dynamic, vertices, sizeof(vertices));
        u32 indices[] = {
            0, 1, 2, 2, 3, 0
        };
        render_buffer_t index_buffer = s_renderer_index_buffer_create(&renderer_state, RenderBufferUsage_Dynamic, indices, sizeof(indices));

        uniform_constant_buffer_t *camera_matrices_buffer = s_renderer_get_constant_buffer(&renderer_state, STR("CameraMatrices"));

        g_running = true;
        while(g_running)
        {
            s_im_reset_controller_states(&input_manager);
            process_window_events(state->window, &renderer_state, &input_manager);

            state->input_axis = {};
            if(s_im_is_keyboard_key_down(game_controller, SDL_SCANCODE_W))
            {
                state->input_axis.y = -1.0f;
            }
            if(s_im_is_keyboard_key_down(game_controller, SDL_SCANCODE_A))
            {
                state->input_axis.x = -1.0f;
            }
            if(s_im_is_keyboard_key_down(game_controller, SDL_SCANCODE_S))
            {
                state->input_axis.y =  1.0f;
            }
            if(s_im_is_keyboard_key_down(game_controller, SDL_SCANCODE_D))
            {
                state->input_axis.x =  1.0f;
            }

            if(delta_time >= (gcv_tick_rate * 2.0f))
            {
                delta_time = gcv_tick_rate * 2.0f;
            }

            dt_accumulator += delta_time;
            while(dt_accumulator >= gcv_tick_rate)
            {
                client_data_t *client   = state->clients + state->client_id;
                input_data_t input_data = {.input_axis = state->input_axis};

                client->input_data_buffer[client->input_data_head] = input_data;
                client->input_data_head = (client->input_data_head + 1) % MAX_BUFFERED_INPUTS;

                // NOTE(Sleepster): If we have any input packets from the host, reconcile here... 
                // If we are the host update the client's players...
                s_nt_client_check_packets(state);
                s_nt_client_send_packets(state);

                for(u32 entity_index = 0;
                    entity_index < state->entity_manager.active_entities;
                    ++entity_index)
                {
                    entity_t *entity = state->entity_manager.entities + entity_index;
                    switch(entity->e_type)
                    {
                        case ET_Player:
                        {
                            client_data_t *entity_client = state->clients + entity->owner_client_id;
                            while(entity_client->input_data_tail != entity_client->input_data_head)
                            {
                                input_data_t *input_data = entity_client->input_data_buffer + entity_client->input_data_tail;
                                entity_client->input_data_tail = (entity_client->input_data_tail + 1) % MAX_BUFFERED_INPUTS;
                                entity_simulate_player(entity, input_data, gcv_tick_rate);
                            }
                        }break;
                    }
                }

                dt_accumulator -= gcv_tick_rate;
            }

            render_command_list_t *command_list = s_renderer_get_command_list(&renderer_state);
            r_cmd_renderpass_begin(command_list, game_renderpass_ID);
            r_cmd_bind_vertex_buffer(command_list, &vertex_buffer);
            r_cmd_bind_index_buffer(command_list, &index_buffer);
            r_cmd_use_shader_program(command_list, basic_traiangle);

            struct camera_matrices {
                mat4_t view_matrix;
                mat4_t projection_matrix;
            }camera_matrix_buffer_data;

            camera_matrix_buffer_data = {
                .view_matrix       = mat4_identity(),
                .projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1)
            };
            r_cmd_update_buffer_contents(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

            r_cmd_set_viewport(command_list, vec2(0, 180), vec2(320, -180));
            r_cmd_set_scissor(command_list,  vec2(0, 0),   vec2(320,  180));

            vec4_t color = {0.0, 1.0, 0.0, 1.0};
            r_cmd_update_push_constants(command_list, 0, sizeof(vec4_t), &color);
            r_cmd_draw_indexed(command_list, 6, 0, 1, 0);

            r_cmd_renderpass_end(command_list);
            r_cmd_present(command_list, &game_color_buffer);

#if 0
            // TODO(Sleepster):  
            // Implement a hash table inside of renderer_state_t. This hash table will take the name of the uniform buffer object / SSBO
            // and return us an poiner to the constant_buffer_t. This pointer is just the constant_buffer_t itself from inside the hash table
            // This can be done like so:
            
            constant_buffer_t *light_buffer = r_renderer_get_constant_buffer(renderer_state, STR("light_buffer"));

            // Through doing this, it means that if other shaders need to access the data within "light_buffer" we have the pointer for quick access, allowing
            // the hash lookup to only have to ever be performed a single time. Updating then becomes extremely easy:

            point_light_t *lights = ...;
            r_cmd_update_buffer_contents(light_buffer, lights, sizeof(point_light_t) * light_count);

            // This is copied to the cpu side buffer, then is staged later when we call execute a command similar to that of idRenderProgManager::CommitCurrent
            // which will move the data to the gpu by mapping the buffer, copying the cpu side data and writing the amount into the correct offset, then finally
            // unmapping the gpu buffer.
            //
            // After that, we will do the exact same methods of descriptor writing and allocating.
            //
            // In the case of shader local uniforms, they will go through the same hash lookup and treated exactly as though they were global, their data
            // would simply just be written to the ubo exactly the same. Therefore the notion of "shared" and "local" descriptors is now gone.

            // init time
            asset_handle_t game_material     = s_asset_manager_get_material(asset_manager, STR("game_basic_material"));
            asset_handle_t lightmap_material = s_asset_manager_get_material(asset_manager, STR("lightmap_material"));

            // this should be able to be accessed across many shaders...
            constant_buffer_t *light_buffer = s_material_get_constant_buffer(renderer_state, STR("light_buffer"));

            point_light_t *lights = ...;
            r_cmd_update_buffer_contents(light_buffer, lights, sizeof(point_light_t) * light_count);

            // render time
            render_command_list_t *command_list = s_renderer_get_command_list(&renderer_state);
            r_cmd_renderpass_begin(command_list, game_renderpass_ID);
           
            // If this is an archetype, we use the base instance
            // If this is an instance, we use the instance data
            //
            // This needs to somehow know about the light buffer as well.
            //
            // In this case, we would give this instance's RenderIntensityUBO as the descriptor somehow...
            r_cmd_bind_material(&game_basic_material);

            // This is then a LOCAL constant buffer.
            constant_buffer_t *game_material_camera_matrices = r_material_get_constant_buffer(game_basic_material, STR("CameraMatricesUBO"));
            r_cmd_update_buffer_contents(game_material_camera_matrices, ); 
            r_cmd_draw(...);

            // This is an entirely different archetype, and thus a completely different shader...
            r_cmd_bind_material(&game_lightmap_material);
            constant_buffer_t *game_lightmap_vibrance_data = r_material_get_constant_buffer(&game_lightmap_material, STR("VibranceDataUBO"));
            r_cmd_draw(...);

            r_cmd_renderpass_end(command_list);
            r_cmd_present(command_list, &game_color_buffer);
#endif

            vk_backend_render_frame(&context, &renderer_state);
            c_global_context_reset_temporary_data();

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time  = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);

            //float32 delta_time_ms = delta_time * 1000.0f;
            //printf("delta time: '%.02f'...\n", delta_time_ms);
        }
    }
    else
    {
        Assert(false);
    }
}


