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

struct game_state_t
{
    SDL_Window   *window;
    SDL_Renderer *renderer;
    vec2_t        window_size;

    vec2_t        input_axis;
    vec2_t        player_velocity;
    vec2_t        player_position;
    vec2_t        player_last_p;
};

global_variable bool8 g_running = false;

int
main(int argc, char **argv)
{
    game_state_t *state = Alloc(game_state_t);
    state->window_size = vec2(1920, 1080);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        if(SDL_CreateWindowAndRenderer("Game", 
                                       (u32)state->window_size.x,
                                       (u32)state->window_size.y, 
                                       SDL_WINDOW_RESIZABLE, 
                                      &state->window, 
                                      &state->renderer))
        {
            u64 perf_count_freq = SDL_GetPerformanceFrequency();
            u64 last_tsc        = SDL_GetPerformanceCounter();
            u64 current_tsc     = 0;
            u64 delta_tsc       = 0;

            float32 delta_time    = 0;
            float32 delta_time_ms = 0;

            float64 dt_accumulator = 0.0f;
            float32 tick_rate = 1.0f / 60.0f;

            g_running = true;
            while(g_running)
            {
                state->input_axis = {};

                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    switch(event.type)
                    {
                        case SDL_EVENT_QUIT:
                        {
                            g_running = false;
                        }break;
                    }
                }
                const bool *keyboard_state = SDL_GetKeyboardState(null);
                if(keyboard_state[SDL_SCANCODE_W]) 
                {
                    state->input_axis.y = -1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_A])
                {
                    state->input_axis.x = -1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_S])
                {
                    state->input_axis.y =  1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_D])
                {
                    state->input_axis.x =  1.0f;
                }

                dt_accumulator += delta_time;
                if(delta_time >= (tick_rate * 2.0f))
                {
                    delta_time = tick_rate * 2.0f;
                }

                while(dt_accumulator >= tick_rate)
                {
                    state->player_last_p = state->player_position;
                    state->player_velocity.x = (state->input_axis.x * (200.5f * tick_rate));
                    state->player_velocity.y = (state->input_axis.y * (200.5f * tick_rate));

                    state->player_position = vec2_add(state->player_position, state->player_velocity);
                    state->player_velocity = vec2(0.0f, 0.0f);

                    dt_accumulator -= tick_rate;
                }
                float32 alpha = (dt_accumulator / tick_rate);

                SDL_SetRenderDrawColor(state->renderer, 1, 0, 0, 0);
                SDL_RenderClear(state->renderer);
                
                SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
                vec2_t lerped_pos = vec2_lerp(state->player_last_p, state->player_position, alpha);
                SDL_FRect rect = {
                    .x = lerped_pos.x,
                    .y = lerped_pos.y,
                    .w = 20,
                    .h = 20
                };
                SDL_RenderFillRect(state->renderer, &rect);
                SDL_RenderPresent(state->renderer);

                current_tsc = SDL_GetPerformanceCounter();
                delta_tsc   = current_tsc - last_tsc;
                last_tsc    = current_tsc;

                delta_time    = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
                delta_time_ms = delta_time * 1000.0f;

                printf("delta time: '%.02f'...\n", delta_time_ms);
            }
        }
        else
        {
            Assert(false);
        }
    }
    else
    {
        Assert(false);
    }
}
