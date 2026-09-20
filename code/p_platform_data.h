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
    string_t game_dll_name = STR("../../build/Win32/Debug/game_DLL.dll");
#elif OS_LINUX
    #include <sys_linux.h>
    string_t game_dll_name = STR("../../build/Linux/Debug/game_DLL.so");
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
typedef struct sockaddr_in      sockaddr_in_t;

/*===========================================
  ============== OS MEMORY API ==============
  ===========================================*/
enum os_memory_access_flags
{
    OS_MEMORY_ACCESS_FLAG_NONE    = BIT(0),
    OS_MEMORY_ACCESS_FLAG_READ    = BIT(1),
    OS_MEMORY_ACCESS_FLAG_WRITE   = BIT(2),
    OS_MEMORY_ACCESS_FLAG_EXECUTE = BIT(3),
    OS_MEMORY_ACCESS_FLAG_ALL     = OS_MEMORY_ACCESS_FLAG_READ|OS_MEMORY_ACCESS_FLAG_WRITE|OS_MEMORY_ACCESS_FLAG_EXECUTE
};

ENGINE_API u64   sys_get_virtual_memory_page_size(void);
ENGINE_API u32   sys_align_to_page_size(u32 size);

ENGINE_API void* sys_allocate_memory(void *base_address, usize allocation_size);
ENGINE_API void  sys_free_memory(void *data, usize free_size);
ENGINE_API void* sys_reallocate_memory(void *base, u64 old_size, u64 allocation_size);
ENGINE_API bool8 sys_set_memory_access_flags(void *memory, u64 memory_size, int access_flags);

/*===========================================
  ============== FILE IO STUFF ==============
  ===========================================*/
ENGINE_API file_t        sys_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io);
ENGINE_API bool8         sys_file_close(file_t *file_data);
ENGINE_API bool8         sys_file_copy(string_t old_path, string_t new_path);
ENGINE_API s64           sys_file_get_size(file_t *file_data);
ENGINE_API bool8         sys_file_read(file_t *file_data, void *memory, s64 bytes_to_read, s64 file_offset);
ENGINE_API bool8         sys_file_write(file_t *file_data, void *memory, s64 bytes_to_write);

ENGINE_API mapped_file_t sys_file_map(string_t filepath);
ENGINE_API bool8         sys_file_unmap(mapped_file_t *map_data);

ENGINE_API bool8         sys_file_exists(string_t filepath);
ENGINE_API file_data_t   sys_file_get_modtime_and_size(string_t filepath);
ENGINE_API bool8         sys_file_replace_or_rename(string_t old_file, string_t new_file);

ENGINE_API bool8         sys_directory_exists(string_t filepath);
ENGINE_API s32           sys_directory_get_file_count(memory_arena_t *arena, string_t filepath, bool8 recursive, string_t file_ext);
ENGINE_API bool8         sys_directory_get_current_working_dir(byte *buffer, u32 buffer_length);
ENGINE_API void          sys_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);

ENGINE_API void*         sys_load_library(string_t filepath);
ENGINE_API void          sys_free_library(void *library);
ENGINE_API void*         sys_get_proc_address(void *library, string_t procedure);

/*===========================================
  ======= FILE WATCHER OS FUNCTIONS =========
  ===========================================*/
ENGINE_API void  sys_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_sys_watch_data_t *watch_data);
ENGINE_API bool8 sys_file_watcher_add_path(file_watcher_t *watcher, string_t path);
ENGINE_API void  sys_file_watcher_issue_check(file_watcher_t *watcher, sys_file_check_event_data_t *directory_data);
ENGINE_API void  sys_file_watcher_process_changes(file_watcher_t *watcher);

/*===========================================
  ============== MULTITHREADING =============
  ===========================================*/
ENGINE_API s32             sys_get_thread_count(void);
ENGINE_API sys_semaphore_t sys_semaphore_create(s32 initial_thread_count, s32 max_thread_count);
ENGINE_API void            sys_semaphore_close(sys_semaphore_t *semaphore);
ENGINE_API void            sys_semaphore_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms);
ENGINE_API s32             sys_semaphore_release(sys_semaphore_t *semaphore, s32 threads_to_release);
ENGINE_API bool8           sys_semaphore_destroy(sys_semaphore_t *semaphore);
ENGINE_API void            sys_semaphore_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms);
ENGINE_API sys_thread_t    sys_thread_create(thread_proc_t *proc, void *user_data, bool8 close_handle);
ENGINE_API bool8           sys_thread_close_handle(sys_thread_t *thread_data);
ENGINE_API s32             sys_thread_wait(sys_thread_t *handle);
ENGINE_API sys_mutex_t     sys_mutex_create(void);
ENGINE_API void            sys_mutex_free(sys_mutex_t *mutex);
ENGINE_API bool8           sys_mutex_lock(sys_mutex_t *mutex, bool8 should_block);
ENGINE_API bool8           sys_mutex_unlock(sys_mutex_t *mutex);

/*===========================================
  ================ MULTIPROCESS =============
  ===========================================*/

ENGINE_API void *sys_create_process(string_t program_path, string_t argument_string);
ENGINE_API bool8 sys_wait_for_process(void *process);

#endif // P_PLATFORM_DATA_H

