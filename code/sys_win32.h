#if !defined(SYS_WIN32_H)
/* ========================================================================
   $File: sys_win32.h $
   $Date: December 08 2025 07:04 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define SYS_WIN32_H

#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <timeapi.h> 

#define SOCKET int

#define INVALID_FILE_HANDLE (null)

#include <c_types.h>
#include <c_string.h>

typedef void* sys_handle_t;

typedef struct sys_file_check_event_data
{
    string_t   filename;
    string_t   old_filename;

    s32        bytes_returned;
    OVERLAPPED overlapped_data;
    HANDLE     file_handle;
    void      *notify_data;

    bool8      read_failed;
}sys_file_check_event_data_t;

typedef struct file_watcher_watch_data
{
    sys_file_check_event_data_t *directory_data[256];
    u32                         directory_data_count;
}file_watcher_sys_watch_data_t;

#define PLATFORM_THREAD_PROC(name) DWORD WINAPI name(void *user_data)
typedef PLATFORM_THREAD_PROC(thread_proc_t);

DWORD WINAPI sys_work_queue_entry_proc(void *lpParam);
#endif // SYS_WIN32_H

