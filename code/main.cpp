/* ========================================================================
   $File: main.cpp $
   $Date: November 28 2025 06:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_globals.h>

#include <s_nt_networking.h>
#include <s_input_manager.h>
#include <s_renderer.h>
#include <g_game_state.h>
#include <g_entity.h>

#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#include <windows.h>
#else
#error "window only..."
#endif

void
c_process_window_events(SDL_Window *window, input_manager_t *input_manager)
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
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    game_state_t *state = Alloc(game_state_t);
    ZeroStruct(*state);

    render_state_t *render_state = Alloc(render_state_t);
    ZeroStruct(*render_state);

    state->window_size = vec2(600, 600);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        s_nt_socket_api_init(state, argc, argv);
        s_init_renderer(render_state, state);

        u64 perf_count_freq = SDL_GetPerformanceFrequency();
        u64 last_tsc        = SDL_GetPerformanceCounter();
        u64 current_tsc     = 0;
        u64 delta_tsc       = 0;

        float32 delta_time    = 0;
        //float32 delta_time_ms = 0;
        float64 dt_accumulator = 0.0f;

        input_manager_t input_manager = {};
        s_im_init_input_manager(&input_manager);
        input_controller_t *game_controller = s_im_get_primary_controller(&input_manager);

        g_running = true;
        while(g_running)
        {
            s_im_reset_controller_states(&input_manager);
            c_process_window_events(state->window, &input_manager);

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
#if 0
            float32 alpha = (dt_accumulator / gcv_tick_rate);

            SDL_SetRenderDrawColor(state->renderer, 1, 0, 0, 0);
            SDL_RenderClear(state->renderer);

            for(u32 entity_index = 0;
                entity_index < state->entity_manager.active_entities;
                ++entity_index)
            {
                entity_t *entity = state->entity_manager.entities + entity_index;
                SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
                vec2_t lerped_pos = vec2_lerp(entity->last_position, entity->position, alpha);
                SDL_FRect rect = {
                    .x = lerped_pos.x,
                    .y = lerped_pos.y,
                    .w = 60,
                    .h = 60
                };
                SDL_RenderFillRect(state->renderer, &rect);
            }

            SDL_RenderPresent(state->renderer);
#endif
            s_renderer_draw_frame(state, render_state);
            SDL_GL_SwapWindow(state->window);

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time    = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);

            //delta_time_ms = delta_time * 1000.0f;
            //printf("delta time: '%.02f'...\n", delta_time_ms);
        }
    }
    else
    {
        Assert(false);
    }
}
