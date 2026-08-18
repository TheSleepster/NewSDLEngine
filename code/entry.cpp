/* ========================================================================
   $File: entry.cpp $
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

int game_main(void);

// TODO(Sleepster): Toggle this off in release 
FILE_WATCHER_CALLBACK(main_file_watcher)
{
    asset_manager_t *asset_manager = global_context->asset_manager;
    (void)asset_manager;

    string_t file_ext = c_string_get_file_ext_from_path(change->full_path);
    u32 ext_type = c_file_ext_string_to_enum(file_ext);

    string_t filename = c_string_get_filename_from_path_and_ext(change->full_path);
    //string_t old_filename = c_string_get_filename_from_path_and_ext(change->old_filename);
    if(ext_type == FILE_EXT_JFD)
    {
        // TODO(Sleepster): For now, we just NEVER hotreload the asset package files since this seems to cause A LOT of problems. 

        // NOTE(Sleepster): Delaying to prevent an error when reading the asset file too quickly:
        // 'resource temporarily unavailable'
        //s_asset_manager_signal_asset_file_reload(asset_manager, change->full_path);
    }
    else
    {
        if((change->changes & FWC_EVENT_MODIFIED))
        {
            asset_handle_t handle = s_asset_manager_acquire_asset_handle(asset_manager, filename);
            if(handle.is_valid)
            {
                s_asset_manager_queue_asset_load(asset_manager, handle.slot);
            }
            else
            {
                log_error("Could not create a valid asset handle for asset by name of: '%.*s', this asset may not be valid or known to the asset system...\n",
                          fprint_string(filename));
            }
        }
    }
}

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
    const int linked   = SDL_GetVersion();
    const int compiled = SDL_VERSION;

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
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        s32            display_count = 0;
        SDL_DisplayID *display_ids   = SDL_GetDisplays(&display_count);
        Expect(display_ids, "Failure to call SDL_GetDisplays... error: '%s'", SDL_GetError());

        const SDL_DisplayMode *display_data = SDL_GetCurrentDisplayMode(display_ids[0]);
        Expect(display_data, "Could not get the display_mode... error: '%s'...\n", SDL_GetError());

        global_context->renderer_state->window_size = vec2(display_data->w, display_data->h);
        global_context->renderer_state->window = SDL_CreateWindow("Vulkan...", 
                                                 global_context->renderer_state->window_size.x,
                                                 global_context->renderer_state->window_size.y, 
                                                 SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
        if(global_context->renderer_state->window == null)
        {
            log_fatal("Could not create SDL window... Error: '%s'...\n", SDL_GetError());
        }

        SDL_StartTextInput(global_context->renderer_state->window);

        global_context->renderer_state->render_context = c_arena_push_struct(&global_context->context_arena, vulkan_context_t);
        global_context->asset_manager->renderer_state  = global_context->renderer_state; 
        vulkan_context_t *vulkan_context = (vulkan_context_t*)global_context->renderer_state->render_context;

        global_context->renderer_state->backend_initialize(global_context->renderer_state->window);

        // TODO(Sleepster): The count will need to be adjusted in the future. But this is fine for now 
        u32 thread_count = sys_get_thread_count() - 4;
        c_threadpool_init(&global_context->main_threadpool, thread_count, MB(10), true, false);

        s_asset_manager_init(global_context->asset_manager);

        s_im_init_input_manager(global_context->input_manager);
        s_renderer_state_init(global_context->renderer_state, vulkan_context);

        global_context->file_watcher = c_file_watcher_create(FWC_EVENT_ALL, true, main_file_watcher, null, false);
        c_file_watcher_add_path(&global_context->file_watcher, STR("../res/"));
        c_file_watcher_issue_check_over_all_paths(&global_context->file_watcher);

        global_context->running = true;
        while(global_context->running)
        {
            s32 value = game_main();
            if(value == 0)
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
