#if !defined(C_FILE_WATCHER_H)
/* ========================================================================
   $File: c_file_watcher.h $
   $Date: December 08 2025 08:07 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_FILE_WATCHER_H
#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_string.h>
#include <c_hash_table.h>

#include <p_platform_data.h>

/*===========================================
  =============== FILE WATCHER ==============
  ===========================================*/
#define FILE_WATCHER_MAX_CHANGES 128
#define FILE_WATCHER_MAX_PATHS_TO_WATCH 256

typedef struct file_watcher                 file_watcher_t;
typedef struct file_watcher_recorded_change file_watcher_recorded_change_t;

#define FILE_WATCHER_CALLBACK(name) void name(file_watcher_t *watcher, file_watcher_recorded_change_t *change, void *user_data)
typedef FILE_WATCHER_CALLBACK(file_watcher_callback_t);

typedef enum file_watcher_change_event
{
    FWC_EVENT_NONE             = 1 << 0,
    FWC_EVENT_ADDED            = 1 << 1,
    FWC_EVENT_MODIFIED         = 1 << 2,
    FWC_EVENT_DELETED          = 1 << 3,
    FWC_EVENT_MOVED            = 1 << 4,
    FWC_EVENT_ATTRIBUTE_CHANGE = 1 << 5,
    FWC_EVENT_SCAN_CHILDREN    = 1 << 6,
    FWC_EVENT_RENAMED          = 1 << 7,
    FWC_EVENT_ALL              = FWC_EVENT_ADDED|FWC_EVENT_MODIFIED|FWC_EVENT_DELETED|FWC_EVENT_MOVED|FWC_EVENT_ATTRIBUTE_CHANGE|FWC_EVENT_SCAN_CHILDREN|FWC_EVENT_RENAMED,
    WFC_EVENT_COUNT,
}file_watcher_change_event_t;

typedef struct file_watcher_recorded_change
{
    string_t full_path;
    string_t old_filename;

    u32      changes;
    u64      last_change_timestamp;
}file_watcher_recorded_change_t;

typedef struct file_watcher
{
    bool8                          is_valid;
    bool8                          is_verbose;
    // NOTE(Sleepster): this has an arena mainly for copy string.
    memory_arena_t                 watcher_arena;
    
    file_watcher_callback_t       *callback;
    file_watcher_change_event_t    events_to_monitor;
    void                          *user_data;

    bool8                          watch_recursively;
    file_watcher_recorded_change_t observed_changes[FILE_WATCHER_MAX_CHANGES]; 
    u32                            change_count;
    
    string_t                       paths_to_watch[FILE_WATCHER_MAX_PATHS_TO_WATCH];
    u32                            paths_watched;

    u32                            notify_buffer_size;
    file_watcher_sys_watch_data_t  sys_watch_data;

    bool8                          issues_when_checking;
}file_watcher_t;

/*===========================================
  ============ API DEFINITIONS ==============
  ===========================================*/
file_watcher_t  c_file_watcher_create(file_watcher_change_event_t events_to_monitor, bool8 recursive, file_watcher_callback_t *callback, void *user_data, bool8 verbose);
void            c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath);
void            c_file_watcher_issue_check_for_single_path(file_watcher_t *watcher, sys_file_check_event_data_t *watch_data);
void            c_file_watcher_issue_check_over_all_paths(file_watcher_t *watcher);

void            c_file_watcher_add_change_event(file_watcher_t *watcher, string_t fullname, string_t old_filename, sys_file_check_event_data_t *watch_data, u32 changes);
void            c_file_watcher_emit_changes(file_watcher_t *watcher);

#endif // C_FILE_WATCHER_H

