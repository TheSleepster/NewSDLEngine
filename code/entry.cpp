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
#include <s_RHI_image.h>
#include <s_RHI_core.h>

#include <s_nt_networking.h>
#include <s_input_manager.h>
#include <s_asset_manager.h>

#include <asset_file_packer/jfd_asset_file.h>

#ifndef RELEASE 
typedef int game_main_t(global_context_t *_global_context);
global_variable game_main_t *game_main;
#else
int game_main(global_context_t *_global_context);
#endif

#ifndef RELEASE

// TODO(Sleepster): HANDLE THE HOTRELOADING IN HERE!!!!!!! 
FILE_WATCHER_CALLBACK(main_file_watcher)
{
    (void)watcher;
    (void)user_data;

    asset_manager_t *asset_manager = gc->asset_manager;

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
            if(*handle.is_valid == true)
            {
                s_asset_manager_queue_asset_load(asset_manager, handle.slot);
            }
            else
            {
                log_warning("Could not create a valid asset handle for asset by name of: '%.*s', this asset may not be valid or known to the asset system...\n",
                            fprint_string(filename));
            }
        }
    }
}
#endif

void
process_window_events(RHI_context_t *RHI_context, input_manager_t *input_manager)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        s_im_handle_window_inputs(&event, input_manager);
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                gc->running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                s32 window_x = 0;
                s32 window_y = 0;
                SDL_GetWindowSizeInPixels(RHI_context->window, &window_x, &window_y);

                RHI_handle_window_resize(RHI_context, vec2(window_x, window_y));
            }break;
        }
    }

    // NOTE(Sleepster): Consume redundant events
    u32 redundant_events[MAX_INPUT_EVENTS] = {};
    u32 events_to_remove = 0;

    for(s32 event_index = 0;
        event_index < input_manager->event_count - 1;
        ++event_index)
    {
        input_event_t *event      = input_manager->events + (event_index);
        input_event_t *next_event = input_manager->events + (event_index + 1);
        if(is_same_event(event, next_event))
        {
            redundant_events[events_to_remove++] = (event_index + 1);
            if(event->input_stream.data == null && next_event->input_stream.data != null)
            {
                event->input_stream = next_event->input_stream;
            }
        }
    }

    for(u32 removal_index = 0;
        removal_index < events_to_remove;
        ++removal_index)
    {
        u32 index = (redundant_events[removal_index] - removal_index);
        c_array_remove(input_manager->events, index, input_manager->event_count - removal_index);
    }

    input_manager->event_count -= events_to_remove;

    // NOTE(Sleepster): Dispatch to the according device 
    u64 most_recent_timestamp = 0;
    for(s32 event_index = 0;
        event_index < input_manager->event_count;
        ++event_index)
    {
        input_event_t *event = input_manager->events + event_index;
        input_device_t *device = null;
        if(event->input_type == INPUT_DEVICE_TYPE_KEYBOARD) device = s_im_find_first_keyboard_device(input_manager, null);
        else                                                device = s_im_find_first_gamepad_device(input_manager, null);

        Assert(device);
        append_input_event(device->events, &device->event_count, event);
        if(event->timestampMS >= most_recent_timestamp)
        {
            input_manager->active_device_index = device->device_index;
            most_recent_timestamp = event->timestampMS;
        }
    }
    input_manager->event_count = 0;
}

int
main(void)
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
    gc->RHI_context    = c_arena_push_struct(&gc->persistent_arena, RHI_context_t);
    gc->asset_manager  = c_arena_push_struct(&gc->persistent_arena, asset_manager_t);
    gc->input_manager  = c_arena_push_struct(&gc->persistent_arena, input_manager_t);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD))
    {
        s32            display_count = 0;
        SDL_DisplayID *display_ids   = SDL_GetDisplays(&display_count);
        Expect(display_ids, "Failure to call SDL_GetDisplays... error: '%s'", SDL_GetError());

        const SDL_DisplayMode *display_data = SDL_GetCurrentDisplayMode(display_ids[0]);
        Expect(display_data, "Could not get the display_mode... error: '%s'...\n", SDL_GetError());

        gc->RHI_context->window_size = vec2(display_data->w, display_data->h);
        gc->RHI_context->window = SDL_CreateWindow("Vulkan...", 
                                                   gc->RHI_context->window_size.x,
                                                   gc->RHI_context->window_size.y, 
                                                   SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE);
        if(gc->RHI_context->window == null)
        {
            log_fatal("Could not create SDL window... Error: '%s'...\n", SDL_GetError());
        }

        SDL_StartTextInput(gc->RHI_context->window);

        gc->RHI_context->backend_render_context = c_arena_push_struct(&gc->persistent_arena, vulkan_context_t);
        gc->asset_manager->RHI_context          = gc->RHI_context; 
        vulkan_context_t *vulkan_context        = (vulkan_context_t*)gc->RHI_context->backend_render_context;

        gc->RHI_context->backend_initialize(gc->RHI_context->window);

        // TODO(Sleepster): The count will need to be adjusted in the future. But this is fine for now 
        u32 thread_count = sys_get_thread_count() - 4;
        c_threadpool_init(&gc->main_threadpool, thread_count, MB(10), true, false);

        s_asset_manager_init(gc->asset_manager);

        s_im_init_input_manager(gc->input_manager);
        RHI_context_init(gc->RHI_context, vulkan_context);

#ifndef RELEASE
        gc->file_watcher = c_file_watcher_create(FWC_EVENT_ALL, true, main_file_watcher, null, false);
        c_file_watcher_add_path(&gc->file_watcher, STR("../res/"));
        c_file_watcher_issue_check_over_all_paths(&gc->file_watcher);

        gc->input_manager_playback_file = c_file_open(STR("../input_manager_playback_file.inpdat"), true);

        // NOTE(Sleepster): Load the game code 
        gc->game_library  = sys_load_library(game_dll_name);
        gc->game_dll_path = c_string_make_copy(&gc->persistent_arena, game_dll_name);
        gc->game_dll_data = c_file_get_file_system_info(game_dll_name);
        Assert(gc->game_library);

        
        game_main = (game_main_t*)sys_get_proc_address(gc->game_library, STR("game_main"));
        Assert(game_main);
#endif

        gc->running = true;
        while(gc->running)
        {
            s32 value = game_main(gc);
            if(value == 0)
            {
                gc->running = false;
                break;
            }

#ifndef RELEASE
            // NOTE(Sleepster): Reload the game code 
            if(gc->should_reload)
            {
                SDL_Delay(5);
                sys_free_library(gc->game_library);

                gc->game_library  = sys_load_library(gc->game_dll_path);
                Assert(gc->game_library);

                game_main = (game_main_t*)sys_get_proc_address(gc->game_library, STR("game_main"));
                Assert(game_main);

                log_info("DLL reloaded...\n");
                gc->should_reload = false;
            }
#endif
        }
    }
    else
    {
        Expect(false, "Could not initialize SDL... SDL_Init failed with error: '%s'...\n", SDL_GetError());
    }

    return(0);
}
