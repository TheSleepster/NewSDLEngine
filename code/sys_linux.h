#if !defined(SYS_LINUX_H)
/* ========================================================================
   $File: sys_linux.h $
   $Date: December 08 2025 03:49 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define SYS_LINUX_H
#include <c_types.h>
#include <c_base.h>
#include <c_hash_table.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> 
#include <unistd.h>
#include <dlfcn.h>
#include <string.h> 
#include <poll.h>

#define INVALID_SOCKET      (-1)
#define INVALID_FILE_HANDLE (-1)

#define closesocket(socket) close(socket)


typedef int sys_handle_t;

typedef struct sys_file_check_event_data
{
    s32         file_data;
    u32         last_move_cookie;
    sys_handle_t inotify_handle;

    string_t    filename;
    string_t    old_filename;
}sys_file_check_event_data_t;

typedef struct file_watcher_watch_data
{
    s32          inotify_instance;
    void        *inotify_data;
    s64          inotify_bytes_read;
    s64          inotify_cursor;

    sys_file_check_event_data_t *directory_data[256];
    u32                         directory_data_count;
}file_watcher_sys_watch_data_t;

#define PLATFORM_THREAD_PROC(name) int name(void *user_data)
typedef PLATFORM_THREAD_PROC(thread_proc_t);

int sys_work_queue_entry_proc(void *lpParam);

#endif // OS_LINXU_H

