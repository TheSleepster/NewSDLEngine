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
            r_cmd_bind_render_target(command_list, test_target);

            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(1, 1, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);
#if 0
            render_command_blit_info_t blit_info = {
                .source             = game_target,
                .destination        = test_target,
                .source_offset      = {0, 0},
                .source_size        = vec2(game_target->create_info.width, game_target->create_info.height),
                .destination_offset = vec2(0, 0),
                .destination_size   = vec2(game_target->create_info.width, game_target->create_info.height),
            };
            r_cmd_blit_render_target(command_list, &blit_info);
#endif
            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(0, 0, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);

            r_cmd_present(command_list);

            vk_backend_render_frame(&context, &renderer_state);
            c_global_context_reset_temporary_data();

#if 0 
            // NOTE(Sleepster): 
            // For the lightmap we need to run a forward clustered compute shader that will fill the light's 
            // array of indices that index into the array of shadow geometry. The purpose of this array is to
            // make the each of the lights only run occlusion checking over ONLY the peices of geometry that 
            // they are affected by.
            //
            // This means that in the shader we'd have something like this:
            //
            // // light buffer
            // uniform (layout = std140, binding = 0) buffer lights[] = {
            //      render_light_t lights[]; <- read only
            //      u32            shadow_indices[]; <- This is what the compute shader would write into
            // };
            //
            // uniform (layout = std140, binding = 1) readonly buffer shadow_data = {
            //      shadow_geometry shadows[];
            // };
            //
            // There was a way to do this or something similar (perhaps a seperate buffer for the light indices)
            // before, I just need to remember.

            // NOTE(Sleepster): 
            // Render all the items into the game target. The order looks like so:
            //
            // [PER-GAME RENDERING]
            // Lightmap Contents: 
            //     - Color buffer. 8Bit red texture. Full 1.0 value means max intensity, 0.0 means no effect.
            //
            // STEPS FOR THIS PHASE
            // 1.) Run the lighting compute shader to get the appropriate data needed for overlaying lights
            // 2.) Render the lights and their shadows into the lightmap using the shadow data processed by
            //     the compute shader.
            //
            // [GAME RENDER TARGET]
            // NOTE: Relies on lighting
            // 
            // Game Target Contents:
            //      - Game Color Buffer (RGBA32 8 bits per pixel)
            //      - Game Emmision Buffer (perhaps only another R8 texture?)
            //      - Depth Buffer
            //
            // STEPS FOR THIS PHASE
            // 1.) Render the game's opaque geometry into the game render_target, recording the brightest pixels
            //     into one of it's buffers for later processing of emmision.
            // 2.) Overlay the shadows and the lights over the scene, darkening items either receiving no light,
            //     or very little.
            // 3.) Perform some kind of "bloom" pass on the brightest pixels recorded from before for emmision
            // 4.) Render the game's transparent geometry into this buffer.
            //
            // [POST FX TARGET (MUST BE THE SAME DIMENSIONS AS THE GAME TARGET)]
            // Contents:
            //      - Another RGBA32 (8 bit per color) texture
            //      - Perhaps other textures.
            //
            // NOTE: Reads from the game pass's data
            //
            // STEPS FOR THIS PHASE
            // 2.) Just run whatever post effects here into this texture using whatever input you need.
            //
            //
            // [GAME EFFECT COMPOSITION TARGET]
            // NOTE: relies on both the game texture and the postfx texture.  
            // Game Target Contents:
            //      - Just a 32bit 8 bit per channel color buffer 
            //
            // STEPS FOR THIS PHASE
            // 1.) Composite the game texture and the post_fx_target. THESE TWO NEED TO BE THE SAME RESOLUTION
            //
            // [UI TARGET]
            // NOTE: This rely's on nothing but the data used to render the UI, meaning it can be done
            // in parallel
            //
            // Contents:
            //      - An RGBA32 (8 bit per color) texture
            //      - Depth buffer
            //
            // STEPS FOR THIS PHASE
            // 1.) Render the ui into this texture
            // 2.) Perform any effects needed using forward rendering.
            //
            // [FINAL TARGET (THIS IMAGE WILL BE BLIT TO THE SWAPCHAIN)]
            //
            // NOTE: Relies on both the game texture and the post-fx composed texture 
            //
            //  Contents:
            //      - Whatever it needs, likely just a single RGBA32 (8 bit per color) texture.
            //
            // STEPS FOR THIS PHASE
            // 1.) Use the ui target's depth buffer to compose the ui texture overtop of the game/postfx rneder
            // target when appropriate.
 
            // NOTE(Sleepster): INIT TIME 
            render_target_t *game_target    = {};
            render_target_t *lightmap       = {};
            render_target_t *post_fx_target = {};
            render_target_t *ui_target      = {};
            render_target_t *final_target   = {};

            image_create_info_t game_color_buffer_image_info = ...;
            image_t game_color_buffer_image = s_renderer_image_create(renderer_state, &game_color_buffer_image_info);

            image_create_info_t game_depth_buffer_image_info = ...;
            image_t game_depth_buffer_image = s_renderer_image_create(renderer_state, &game_depth_buffer_image_info);

            render_target_attachment_info_t game_target_color_buffer = {
                .attachment = color_buffer_texture,
                .attachment_type = RTAT_ColorBuffer,
            };

            render_target_attachment_info_t game_depth_buffer = {
                .attachment = game_depth_buffer_image,
                .attachment_type = RTAT_DepthBuffer,
            };

            render_target_create_info_t game_info = {
                .width = 320,
                .height = 180,
                .attachments = {game_target_color_buffer, game_depth_buffer},
                .attachment_count = 2,
                .resize_with_window = false,
            };
            game_target_t = s_renderer_render_target_create(renderer_state, &game_target_create_info);
            // repeat this process for each target...

            // NOTE(Sleepster): PERFORMED EACH FRAME 
            // Above all this, is the update loop, for this hypothetical example, we don't include this data.
            // There are a few things to note however, despite the lacking of the update loop for this example, 
            // it is safe to assume that the update loop:
            // - handles entity updates, including adding and removing entities from the entity pool as needed.
            // - handles lights, the game treats lights as game objects, the game OWNS the lights.
            //   and since it needs owns the lights, it needs to generate shadow geometry for each light.
            //   and handle that SSBO accordingly
            //
            // This means that there is an SSBO that is implicitly handled by the engine and out of the user's
            // control. This SSBO is:
            // - The RenderInstances SSBO
            //
            // Why? Because this is where ALL render_instances go.
            render_command_list_t *command_list = s_renderer_get_command_list(renderer_state);

            r_cmd_reset_frame(command_list);
            r_cmd_bind_shader(command_list, light_cluster_compute_shader);

            // TODO(Sleepster): set constant buffer size.

            // NOTE(Sleepster): This should store this information into a CPU side buffer. 
            //
            // One would think lights are owned by the renderer, however in our case we want the
            // user to have the greatest degree of control and expression, therefore the user
            // can just define their own lights and shadow geoemetry if they wish to use those items.
            //
            //
            // This is purely an exampel of the kind of behavior we want, in reality you probably don't want to do this.
            // The idea here is that for the user it looks like it's OpenGL "style" where you supply uniform data, render,
            // repeat. But for the backend, all of these constant buffer updates get merged into a single buffer and we just have
            // some easy way of indexing into the data inside the shader.
            void *particle_data = ...;
            void *lights = ...;
            r_cmd_update_constant_buffer(command_list, light_constant_buffer, lights, sizeof(light) * light_count);

            // dispath count x, dispath count y, dispatch count z
            //
            // NOTE: These numbers are arbitrary, I will do something ACTUALLY smart in a real implementation.
            r_cmd_dispatch_compute(command_list, 20, 20, 1);

            // NOTE(Sleepster): We set memory barrier manually here.
            r_cmd_wait_for_compute(command_list);

            r_cmd_bind_render_target(command_list, game_render_target);
            r_cmd_update_constant_buffer(command_list, game_camera_constant_buffer, &game_camera, sizeof(camera));

            // ... we'd render types of entities like so:
            //
            // NOTE: THESE ARE ALL OPAQUE
            for(u32 entity_type_index = 0;
                entity_type_index < entity_type_count;
                ++entity_type_index)
            {
                entity_t *entity_type = entities[entity_type_index];

                if(entity->material.opacity != MATERIAL_OPACITY_TRANSPARENT)
                {
                    r_cmd_bind_material(command_list, entity_type->material);
                    r_cmd_begin_render_group();
                    for(u32 entity_index = 0;
                        entity_index < entity_count;
                        ++entity_index)
                    {
                        entity_t *entity = entity_type + entity_index;
                        r_cmd_render_bitmap(command_list, entity->bitmap);
                    }
                    r_cmd_end_render_group();
                }
            }

            // NOTE(Sleepster): The idea is simple, we are able to set the details needed for each of the render_groups
            // without effecting the other groups. So, when we go to execute the command, the render_instance being rnedered
            // would use the data associated with the render_group. The rencder_group handles information like
            // what the contents of constant buffers are at the time "begin" is called, what the used shader is (materials 
            // are just wrappers around a shader and some preset constants the user may want in the shader), and other such things
            // like push constants.

            // NOTE(Sleepster): These are transparent 
            for(u32 entity_type_index = 0;
                entity_type_index < entity_type_count;
                ++entity_type_index)
            {
                entity_t *entity_type = entities[entity_type_index];
                // NOTE(Sleepster): Same as above 
                // ...
                if(entity->material.opacity == MATERIAL_OPACITY_TRANSPARENT)
                {
                    // ...
                }
            }
            // NOTE(Sleepster): The user's shader(s) will handle rendering the items into the emmision buffer
            
            // TODO(Sleepster): Denote how to render the lights and shadows. 

            r_cmd_bind_render_target(command_list, post_fx_target);
            // NOTE(Sleepster): Maybe something like this since e KNOW we need to read the contents of this item? 
            // The idea is that on the backend we might be able to do something related to making this readable from
            // a shader and/or a renderpass to prevent the need to blit or take up a GPU texture slot. Not sure
            // how possible this is  though... Just an idea.
            r_cmd_set_input_texture(command_list, post_fx_target, game_color_buffer_image);
#endif

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

