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

            vk_backend_render_frame(&context, &renderer_state);
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

