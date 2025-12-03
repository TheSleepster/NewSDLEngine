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
            g_running = true;
            while(g_running)
            {
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

                SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 0);
                SDL_RenderClear(state->renderer);
                SDL_RenderPresent(state->renderer);
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
