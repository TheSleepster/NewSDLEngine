/* ========================================================================
   $File: main.cpp $
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
#include <s_renderer.h>

#include <s_nt_networking.h>
#include <s_input_manager.h>
#include <s_asset_manager.h>
#include <g_game_state.h>
#include <g_entity.h>

#include <asset_file_packer/jfd_asset_file.h>
//#include <meta/GENERATED_program_RTTI.h>

internal_api void
c_process_window_events(SDL_Window *window, renderer_state_t *renderer_state, input_manager_t *input_manager)
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
        asset_handle_t default_material = s_asset_manager_acquire_asset_handle(asset_manager, STR("test_material_archetype"));
        (void)default_shader;
        (void)default_material;

        texture_atlas_t *atlas = s_texture_atlas_create(asset_manager, 1024, 4, BMF_RGBA32, 32);
        s_texture_atlas_add_texture(atlas, &default_texture);
        s_texture_atlas_pack_added_textures(&context, atlas);

        s_nt_socket_api_init(state, argc, argv);
#if 0
        render_context->window = state->window;
        r_renderer_init(render_context, render_state, state->window_size);

        asset_manager->render_context = render_context;
        render_context->default_texture  = Alloc(asset_handle_t);
        *render_context->default_texture = s_asset_manager_acquire_asset_handle(asset_manager, STR("player"));

        render_context->default_shader  = Alloc(asset_handle_t);
        *render_context->default_shader = s_asset_manager_acquire_asset_handle(asset_manager, STR("test_shader"));

        render_context->default_material  = Alloc(asset_handle_t);
        *render_context->default_material = s_asset_manager_acquire_asset_handle(asset_manager, STR("test_material_archetype"));

        r_vulkan_make_gpu_texture(render_context, &render_context->default_texture->slot->texture);
        r_render_state_init(render_state, render_context);

        s_nt_socket_api_init(state, argc, argv);

        texture_atlas_t *atlas = s_texture_atlas_create(asset_manager, 1024, 4, BMF_RGBA32, 32);
        s_texture_atlas_add_texture(atlas, render_context->default_texture);
        s_texture_atlas_pack_added_textures(render_context, atlas);
#endif

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

        // NOTE(Sleepster): Test blit image 
        image_create_info_t image_info = {};
        image_info.image_type       = IMAGE_TYPE_ColorAttachment;
        image_info.format           = BMF_RGBA32;
        image_info.width            = 1920;
        image_info.height           = 1080;

        image_t color_buffer = s_renderer_image_create(&renderer_state, &image_info);

        render_target_attachment_info_t color_attachment = {};
        color_attachment.attachment      = &color_buffer;
        color_attachment.attachment_type = IMAGE_TYPE_ColorAttachment;
        color_attachment.initial_layout  = IMAGE_TYPE_Undefined;
        color_attachment.final_layout    = IMAGE_TYPE_ColorAttachment;
        color_attachment.load_operation  = RTALO_Clear;
        color_attachment.store_operation = RTASO_Store;

        color_attachment.clear_value.clear_color.float_color = vec4(0.4, 0.6, 1.0, 1.0);

        render_target_create_info_t target_info = {};
        target_info.attachments        = &color_attachment;
        target_info.attachment_count   = 1;
        target_info.width              = 1920;
        target_info.height             = 1080;
        target_info.resize_with_window = true;
        render_target_t *test_target = s_renderer_render_target_create(&renderer_state, &target_info);

        // NOTE(Sleepster): Game Texture 
        image_create_info_t game_texture_info = {};
        game_texture_info.image_type = IMAGE_TYPE_ColorAttachment;
        game_texture_info.format     = BMF_RGBA32;
        game_texture_info.width      = 320;
        game_texture_info.height     = 180;

        image_t game_texture = s_renderer_image_create(&renderer_state, &game_texture_info);

        render_target_attachment_info_t game_buffer_attachment = {};
        game_buffer_attachment.attachment      = &game_texture;
        game_buffer_attachment.attachment_type = IMAGE_TYPE_ColorAttachment;
        game_buffer_attachment.initial_layout  = IMAGE_TYPE_Undefined;
        game_buffer_attachment.final_layout    = IMAGE_TYPE_ColorAttachment;
        game_buffer_attachment.load_operation  = RTALO_Clear;
        game_buffer_attachment.store_operation = RTASO_Store;

        game_buffer_attachment.clear_value.clear_color.float_color = vec4(1.0, 0.0, 0.0, 1.0);

        render_target_create_info_t game_target_info = {};
        game_target_info.resize_with_window = false;
        game_target_info.attachments        = &game_buffer_attachment;
        game_target_info.attachment_count   = 1;
        game_target_info.width              = 320;
        game_target_info.height             = 180;

        render_target_t *game_target = s_renderer_render_target_create(&renderer_state, &game_target_info);
        (void)game_target;

        g_running = true;
        while(g_running)
        {
            s_im_reset_controller_states(&input_manager);
            c_process_window_events(state->window, &renderer_state, &input_manager);

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

                // TODO(Sleepster): If we have any input packets form the host, reconcile here... 
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
            r_cmd_bind_render_target(command_list, game_target);
            r_cmd_bind_render_target(command_list, test_target);

            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(1, 1, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);

            render_command_blit_info_t blit_info = {
                .source             = game_target,
                .destination        = test_target,
                .source_offset      = {0, 0},
                .source_size        = vec2(game_target->create_info.width, game_target->create_info.height),
                .destination_offset = vec2(0, 0),
                .destination_size   = vec2(game_target->create_info.width, game_target->create_info.height),
            };
            r_cmd_blit_render_target(command_list, &blit_info);

            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(0, 0, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);

            r_cmd_present(command_list);

            vk_backend_render_frame(&context, &renderer_state);
#if 0
            if(r_vulkan_begin_frame(render_context, render_state, gcv_tick_rate))
            {

                vulkan_shader_data_t *shader = &render_context->default_shader->slot->shader.shader_data;
                mat4_t view_matrix = mat4_identity();
                mat4_t projection_matrix = mat4_RHGL_ortho(render_context->framebuffer_width  * -0.5,
                                                           render_context->framebuffer_width  *  0.5,
                                                           render_context->framebuffer_height * -0.5,
                                                           render_context->framebuffer_height *  0.5,
                                                           0.0,
                                                           1.0);
                shader->camera_matrices = {
                    .view_matrix       = view_matrix,
                    .projection_matrix = projection_matrix
                };

                //r_vulkan_shader_set_uniform_data(render_state->draw_frame.state.active_shader, STR("Matrices"), &shader->camera_matrices, sizeof(shader->camera_matrices));
                r_set_active_render_material(render_state, &render_state->default_material);
                r_set_active_material_constant(render_state, STR("Matrices"), &shader->camera_matrices, sizeof(shader->camera_matrices));

                asset_handle_t material_instance = r_create_material_instance(&render_state->shiny_material);
                r_set_constant_buffer_data(material_instance.set0.matrices, &shader->camera_matrices, sizeof(shader->camera_matrices));

                r_render_group_begin(render_state);
                r_push_texture(render_state, {0, 0}, {100, 100}, {0.0, 1.0, 0.0, 1.0}, 0, render_context->default_texture);
                r_push_texture(render_state, {-100, 100}, {100, 100}, {1.0, 1.0, 1.0, 1.0}, 0, render_context->default_texture);
                r_render_group_end(render_state);

                r_render_group_begin(render_state);
                r_push_rect(render_state, {0, 150}, {100, 100}, {1.0, 1.0, 1.0, 1.0}, 0);
                r_render_group_end(render_state);

                r_render_group_update_used_groups(render_state);
                r_vulkan_end_frame(render_context, render_state, gcv_tick_rate);

            }

            
            render_image_t game_texture = s_renderer_create_texture(renderer_state, 320, 180, TFMT_RGBA32);
            render_image_t game_depth   = s_renderer_create_texture(renderer_state, 320, 180, TFMT_DEPTH32);

            render_image_t images[] = {
                game_texture,
                game_depth
            };
            render_target_create_info info = {};
            info.render_targets      = images;
            info.render_target_count = ArrayCount(images);

            // NOTE(Sleepster): Create the game render_target 
            render_target_t game_render_target = s_renderer_create_render_target(renderer_state, &info);

            render_command_list_t *command_list = r_cmd_get_command_list(renderer_state);
            r_cmd_set_active_render_target(command_list, game_render_target);

            material_archetype_t *basic_material = s_asset_get_material_archetype(asset_manager, STR("basic_material.m_arch"));
            material_instance_t  *vibrant_basic  = s_asset_get_material_instance(asset_manager, STR("basic_vibrant.m_inst"));
            shader_t             *combo_shader   = s_asset_get_shader(asset_manager, STR("combo_shader.spv"));
            asset_handle_t        player_texture = s_asset_get_texture(asset_manager, STR("Player"));

            // NOTE(Sleepster): Gives back a CPU side buffer for the data you want to fill... 
            constant_buffer_t *constant_data = s_asset_material_get_constant_data(basic_material);

            r_cmd_set_active_material(command_list, basic_material);
            r_cmd_begin_render_group(command_list);

            r_cmd_draw_texture(command_list, vec2(200, 200), vec2(20, 20), vec4(1, 1, 1, 1), &player_texture);
            r_cmd_draw_texture(command_list, vec2(100, 220), vec2(20, 20), vec4(1, 1, 1, 1), &player_texture);
            r_cmd_draw_texture(command_list, vec2(200, 260), vec2(20, 20), vec4(1, 1, 1, 1), &player_texture);
            r_cmd_draw_texture(command_list, vec2(190, 360), vec2(20, 20), vec4(1, 1, 1, 1), &player_texture);

            r_cmd_end_render_group(command_list);

            // NOTE(Sleepster): Create the ui_render_target 
            render_image_t ui_texture   = s_renderer_create_texture(renderer_state, 1920, 1080, TFMT_RGBA32);
            render_image_t ui_images[] = {
                ui_texture,
            };
            render_target_create_info uiinfo = {};
            info.render_targets      = ui_images;
            info.render_target_count = ArrayCount(ui_images);
            render_target_t ui_render_target = s_renderer_create_render_target(renderer_state, &uiinfo);

            r_cmd_set_active_render_target(command_list, ui);
            r_cmd_set_active_material(command_list, vibrant_basic);

            asset_handle_t ui_texture = s_asset_get_texture(asset_manager, STR("UI Overlay"));
            r_cmd_begin_render_group(command_list);
            r_cmd_draw_texture(command_list, vec2(1920 * 0.5, 1060), vec2(20, 60), vec4(1, 1, 1, 1), ui_texture);
            r_cmd_end_render_group(command_list);


            // NOTE(Sleepster): Mix the two 
            render_image_t game_texture_upscaled = s_renderer_create_texture(renderer_state, 1920, 1080, TFMT_RGBA32);
            render_image_t game_texture_upscaled_images[] = {
                game_texture_upscaled,
            };
            render_target_create_info combo_info = {};
            info.render_targets      = game_texture_upscaled_images;
            info.render_target_count = ArrayCount(game_texture_upscaled_images);

            render_target_t s_renderer_create_render_target(renderer_state, &combo_info);

            // NOTE(Sleepster): target, source, offsetx, offsety, width, height, sampling 
            r_cmd_texture_blit(game_texture_upscaled_images, game_texture, 0, 0, 320, 180, TFMT_Nearest);

            // combine down here with a special shader, maybe blit the 320x180 texture to full res.
            r_cmd_set_shader(command_list, combination_shader);
            r_cmd_
#if 0
            float32 alpha = (dt_accumulator / gcv_tick_rate);
#endif
#endif
            c_global_context_reset_temporary_data();

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time    = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);

            //float32 delta_time_ms = delta_time * 1000.0f;
            //printf("delta time: '%.02f'...\n", delta_time_ms);
        }
    }
    else
    {
        Assert(false);
    }
}

