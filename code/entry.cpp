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

void game_main(void);

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
                global_context->running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                s32 window_x = 0;
                s32 window_y = 0;
                SDL_GetWindowSizeInPixels(window, &window_x, &window_y);

                g_window_size.x = (float32)window_x;
                g_window_size.y = (float32)window_y;

                s_renderer_handle_window_resize(renderer_state, g_window_size);
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    vulkan_context_t *vulkan_context = Alloc(vulkan_context_t);

    global_context->renderer_state->window_size = vec2(600, 600);
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
        c_global_context_init();
        vk_backend_init(vulkan_context, global_context->renderer_state->window);

        // TODO(Sleepster): The count will need to be adjusted in the future. But this is fine for now 
        u32 thread_count = sys_get_thread_count() - 1;
        c_threadpool_init(&global_context->main_threadpool, thread_count, MB(10), true);

        s_asset_manager_init(global_context->asset_manager);
        s_asset_manager_load_asset_file(global_context->asset_manager, STR("asset_data.jfd"));
        global_context->asset_manager->vulkan_context = vulkan_context;

        s_im_init_input_manager(global_context->input_manager);
        s_renderer_state_init(global_context->renderer_state, vulkan_context);

        global_context->running = true;
        game_main();
        // call game main
    }
    else
    {
        Assert(false);
    }
}
