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
#include <c_global_context.h>
#include <c_zone_allocator.h>
#include <c_program_flag_handler.h>
#include <c_tokenizer.h>
#include <p_platform_data.h>

#include <vk_backend_core.h>
#include <r_render_image.h>
#include <s_render_RHI.h>

#include <s_nt_networking.h>
#include <s_input_manager.h>
#include <s_asset_manager.h>

#include <asset_file_packer/jfd_asset_file.h>
//#include <meta/GENERATED_program_RTTI.h>

int game_main(void);

void
process_window_events(renderer_state_t *renderer_state, input_manager_t *input_manager)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        s_im_handle_window_inputs(&event, input_manager);
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                global_context->running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                s32 window_x = 0;
                s32 window_y = 0;
                SDL_GetWindowSizeInPixels(renderer_state->window, &window_x, &window_y);

                s_renderer_handle_window_resize(renderer_state, vec2(window_x, window_y));
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    int linked   = SDL_GetVersion();
    int compiled = SDL_VERSION;

    SDL_Log("We compiled against SDL version %d.%d.%d ...\n",
            SDL_VERSIONNUM_MAJOR(compiled),
            SDL_VERSIONNUM_MINOR(compiled),
            SDL_VERSIONNUM_MICRO(compiled));

    SDL_Log("But we are linking against SDL version %d.%d.%d.\n",
            SDL_VERSIONNUM_MAJOR(linked),
            SDL_VERSIONNUM_MINOR(linked),
            SDL_VERSIONNUM_MICRO(linked));

    c_global_context_init();
    global_context->renderer_state = c_arena_push_struct(&global_context->context_arena, renderer_state_t);
    global_context->asset_manager  = c_arena_push_struct(&global_context->context_arena, asset_manager_t);
    global_context->input_manager  = c_arena_push_struct(&global_context->context_arena, input_manager_t);

    global_context->renderer_state->window_size = vec2(2560, 1440);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        global_context->renderer_state->window = SDL_CreateWindow("Vulkan...", 
                                                  global_context->renderer_state->window_size.x,
                                                  global_context->renderer_state->window_size.y, 
                                                  SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
        if(global_context->renderer_state->window == null)
        {
            log_fatal("Could not create SDL window... Error: '%s'...\n", SDL_GetError());
        }

        global_context->renderer_state->render_context = c_arena_push_struct(&global_context->context_arena, vulkan_context_t);
        global_context->asset_manager->vulkan_context  = (vulkan_context_t*)global_context->renderer_state->render_context; 
        vulkan_context_t *vulkan_context = (vulkan_context_t*)global_context->renderer_state->render_context;

        vk_backend_init(vulkan_context, global_context->renderer_state->window);

        // TODO(Sleepster): The count will need to be adjusted in the future. But this is fine for now 
        u32 thread_count = sys_get_thread_count() - 1;
        c_threadpool_init(&global_context->main_threadpool, thread_count, MB(10), true);

        s_asset_manager_init(global_context->asset_manager);

        s_im_init_input_manager(global_context->input_manager);
        s_renderer_state_init(global_context->renderer_state, vulkan_context);

        global_context->running = true;
        while(global_context->running)
        {
            // TODO(Sleepster): Eventually hot reloading... 
            s32 value = game_main();
            if(value == -1) 
            {
                // this is where we would reset and flush ALL state and reload the game DLL
            }
            else if(value == 0)
            {
                global_context->running = false;
                break;
            }
        }
    }
    else
    {
        Expect(false, "Could not initialize SDL... SDL_Init failed with error: '%s'...\n", SDL_GetError());
    }

    return(0);
}
