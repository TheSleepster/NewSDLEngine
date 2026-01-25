#if !defined(P_PLATFORM_DATA_H)
/* ========================================================================
   $File: p_platform_data.h $
   $Date: December 08 2025 08:00 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define P_PLATFORM_DATA_H
#include <c_base.h>

#if OS_WINDOWS
    #include <sys_win32.h>
#elif OS_LINUX
    #include <sys_linux.h>
#elif OS_MAC
    #error "lmao really?"
#endif

#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_string.h>

typedef struct mapped_file      mapped_file_t;
typedef struct file_data        file_data_t;
typedef struct file_watcher     file_watcher_t;
typedef struct visit_file_data  visit_file_data_t;

/*===========================================
  ============== OS MEMORY API ==============
  ===========================================*/
void* sys_allocate_memory(usize allocation_size);
void  sys_free_memory(void *data, usize free_size);
void* sys_reallocate_memory(void *offset, u64 allocation_size);

/*===========================================
  ============== FILE IO STUFF ==============
  ===========================================*/
file_t        sys_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io);
bool8         sys_file_close(file_t *file_data);
bool8         sys_file_copy(string_t old_path, string_t new_path);
s64           sys_file_get_size(file_t *file_data);
bool8         sys_file_read(file_t *file_data, void *memory, u32 bytes_to_read, u32 file_offset);
bool8         sys_file_write(file_t *file_data, void *memory, usize bytes_to_write);

mapped_file_t sys_file_map(string_t filepath);
bool8         sys_file_unmap(mapped_file_t *map_data);

bool8         sys_file_exists(string_t filepath);
file_data_t   sys_file_get_modtime_and_size(string_t filepath);
bool8         sys_file_replace_or_rename(string_t old_file, string_t new_file);

bool8         sys_directory_exists(string_t filepath);
bool8         sys_directory_get_current_working_dir(byte *buffer, u32 buffer_length);
void          sys_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);

void*         sys_load_library(string_t filepath);
void          sys_free_library(void *library);
void*         sys_get_proc_address(void *library, string_t procedure);

/*===========================================
  ======= FILE WATCHER OS FUNCTIONS =========
  ===========================================*/
void  sys_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_sys_watch_data_t *watch_data);
bool8 sys_file_watcher_add_path(file_watcher_t *watcher, string_t path);
void  sys_file_watcher_issue_check(file_watcher_t *watcher, sys_file_check_event_data_t *directory_data);
void  sys_file_watcher_process_changes(file_watcher_t *watcher, bool8 *changed);

/*===========================================
  ============== MULTITHREADING =============
  ===========================================*/
s32             sys_get_cpu_count();
sys_semaphore_t sys_semaphore_create(s32 initial_thread_count, s32 max_thread_count);
void            sys_semaphore_close(sys_semaphore_t *semaphore);
void            sys_semaphore_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms);
s32             sys_semaphore_release(sys_semaphore_t *semaphore, s32 threads_to_release);
bool8           sys_semaphore_destroy(sys_semaphore_t *semaphore);
sys_thread_t    sys_thread_create(thread_proc_t *proc, void *user_data, bool8 close_handle);
void            sys_thread_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms);
bool8           sys_thread_close_handle(sys_thread_t *thread_data);
sys_mutex_t     sys_mutex_create();
void            sys_mutex_free(sys_mutex_t *mutex);
bool8           sys_mutex_lock(sys_mutex_t *mutex, bool8 should_block);
bool8           sys_mutex_unlock(sys_mutex_t *mutex);

#endif // P_PLATFORM_DATA_H

