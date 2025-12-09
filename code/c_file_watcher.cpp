/* ========================================================================
   $File: c_file_watcher.cpp $
   $Date: December 08 2025 08:12 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>
#include <c_file_watcher.h>

file_watcher_t 
c_file_watcher_create(file_watcher_change_event_t events_to_monitor,
                      bool8                       recursive,
                      file_watcher_callback_t    *callback,
                      void                       *user_data,
                      bool8                       is_verbose)
{
    file_watcher_t result = {};
    
    result.callback          = callback;
    result.is_verbose        = is_verbose;
    result.events_to_monitor = events_to_monitor;
    result.user_data         = user_data;
    result.watch_recursively = recursive;
    result.watcher_arena     = c_arena_create(MB(10));
    result.is_valid          = true;
    sys_file_watcher_init_watch_data(&result.watcher_arena, &result.sys_watch_data);

    return(result);
}

void
c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath)
{
    watcher->paths_to_watch[watcher->paths_watched] = c_string_make_copy(&watcher->watcher_arena, filepath);
    sys_file_watcher_add_path(watcher, filepath);
}

void
c_file_watcher_issue_check_for_single_path(file_watcher_t *watcher, sys_file_check_event_data_t *watch_data)
{
    sys_file_watcher_issue_check(watcher, watch_data);
}

void
c_file_watcher_issue_check_over_all_paths(file_watcher_t *watcher)
{
    for(u32 data_index = 0;
        data_index < watcher->sys_watch_data.directory_data_count;
        ++data_index)
    {
        sys_file_check_event_data_t *watch_data = watcher->sys_watch_data.directory_data[data_index];
        if(watch_data)
        {
            sys_file_watcher_issue_check(watcher, watch_data);
        }
    }
}

void
c_file_watcher_process_changes(file_watcher_t *watcher)
{
    sys_file_watcher_process_changes(watcher, null);
}

void
c_file_watcher_add_change_event(file_watcher_t *watcher,
                                string_t fullname,
                                string_t old_filename,
                                sys_file_check_event_data_t *watch_data,
                                u32 changes)
{
    for(u32 file_change_index = 0;
        file_change_index < watcher->change_count;
        ++file_change_index)
    {
        file_watcher_recorded_change_t *change = watcher->observed_changes + file_change_index;
        if(c_string_compare(change->full_path, fullname))
        {
            change->changes              |= changes;
            change->last_change_timestamp = SDL_GetTicks();

            if(watcher->is_verbose) log_info("[FILE WATCHER]: Overrided entry: '%s'...\n", fullname.data);
        }
    }

    file_watcher_recorded_change_t new_change;
    new_change.full_path             = fullname;
    new_change.changes               = changes;
    new_change.last_change_timestamp = SDL_GetTicks();
    new_change.old_filename          = old_filename;

    watcher->observed_changes[watcher->change_count] = new_change;
    watcher->change_count++;
}

void
c_file_watcher_emit_changes(file_watcher_t *watcher)
{
    Assert(watcher->callback);
    if(watcher->change_count == 0) return;

    u32 max_changes = watcher->change_count;
    for(u32 file_change_index = 0;
        file_change_index < max_changes;
        ++file_change_index)
    {
        file_watcher_recorded_change_t *change = watcher->observed_changes + file_change_index;
        if(change)
        {
            watcher->callback(watcher, change, watcher->user_data);
            watcher->change_count--;
        }
    }
    ZeroMemory(watcher->observed_changes, sizeof(file_watcher_recorded_change_t) * max_changes);
}

