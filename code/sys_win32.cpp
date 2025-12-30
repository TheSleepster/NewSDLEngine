/* ========================================================================
   $File: sys_win32.cpp $
   $Date: December 08 2025 07:07 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <p_platform_data.h>
#include <sys_win32.h>

#include <c_types.h>
#include <c_base.h>
#include <c_log.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_globals.h>
#include <c_dynarray.h>

// TODO(Sleepster): UNICODE 

typedef struct multithreading_work_queue_manager multithreading_work_queue_manager_t;

/////////////////////
// MEMORY FUNCTIONS
/////////////////////

void*
sys_allocate_memory(usize allocation_size)
{
    void *result = null;
    result = VirtualAlloc(0, allocation_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if(!result)
    {
        DWORD error = GetLastError();
        LPSTR message_buffer = 0;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       null,           
                       error,          
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR)&message_buffer, 
                       0,              
                       null);

        log_fatal("Failed to allocate virtual memory: '%s'...\n", message_buffer);
        LocalFree(message_buffer);
    }

    return(result);
}

void
sys_free_memory(void *data, usize free_size)
{
    VirtualFree(data, 0, MEM_RELEASE);
}

void*
sys_reallocate_memory(void *offset, u64 allocation_size)
{
    void *result = null;
    result = VirtualAlloc(offset, allocation_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if(!result)
    {
        DWORD error = GetLastError();
        LPSTR message_buffer = 0;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                       null,           
                       error,          
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR)&message_buffer, 
                       0,              
                       null);

        log_fatal("Failed to allocate virtual memory: '%s'...\n", message_buffer);
        LocalFree(message_buffer);
    }

    return(result);
}

///////////////////////////////////
// PLATFORM FILE IO FUNCTIONS
///////////////////////////////////

// TODO(Sleepster): UNICODE CreateFileW 
file_t
sys_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io)
{
    file_t result = {};
    result.file_name = c_string_get_filename_from_path(filepath);
    result.filepath  = filepath;

    u32 overlapping = 0;
    if(overlapping_io)
    {
        overlapping        = FILE_FLAG_OVERLAPPED;
        result.overlapping = true;
    }

    if(for_writing)
    {
        u32 mode = 0;
        if(overwrite) mode = OPEN_ALWAYS;
        else mode = CREATE_ALWAYS;

        result.for_writing = true;

        result.handle = CreateFileA(C_STR(filepath),
                                    FILE_GENERIC_READ|FILE_GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    0,
                                    mode,
                                    overlapping,
                                    0);
    }
    else
    {
        result.handle = CreateFileA(C_STR(filepath),
                                    FILE_GENERIC_READ,
                                    FILE_SHARE_READ,
                                    0,
                                    OPEN_ALWAYS,
                                    0,
                                    0);
    }

    if(result.handle == INVALID_HANDLE_VALUE)
    {
        log_error("Failed to open file: '%s' for reading... error: '%d'...\n",
                  filepath.data, GetLastError());

        ZeroStruct(result);
    }

    return(result);
}

bool8 
sys_file_close(file_t *file_data)
{
    bool8 result = CloseHandle(file_data->handle);
    if(result)
    {
        file_data->handle = null;
    }

    return(result);
}

bool8
sys_file_copy(string_t old_path, string_t new_path)
{
    bool8 result = CopyFile(C_STR(old_path), C_STR(new_path), FALSE);

    return(result);
}

s64
sys_file_get_size(file_t *file_data)
{
    s64 result = 0;
    
    LARGE_INTEGER file_size;
    DWORD success = GetFileSizeEx(file_data->handle, &file_size);
    if(success == 0)
    {
        log_error("failed to get file size for file: '%s\n', error: '%s'...\n",
                  C_STR(file_data->file_name), GetLastError());
        result = 0;
    }

    result = file_size.QuadPart;
    return(result);
}

bool8 
sys_file_read(file_t *file_data, void *memory, u32 file_offset, u32 bytes_to_read)
{
    bool8 result = true;
    
    OVERLAPPED byte_data = {};
    byte_data.Offset = file_offset;
    BOOL success = ReadFile(file_data->handle, memory, bytes_to_read, 0, &byte_data);
    if(!success)
    {
        log_error("Error reading the file: '%s', message: '%s'...\n",
                  C_STR(file_data->filepath), GetLastError());
        result = false;
    }

    return(result);
}

// NOTE(Sleepster): This is blocking... It will block until the buffer has written everything 
bool8 
sys_file_write(file_t *file_data, void *memory, usize bytes_to_write)
{
    bool8 result = true;
    
    u32 written = 0;
    BOOL success = WriteFile(file_data->handle, memory, bytes_to_write, (unsigned long*)&written, null);
    if(!success)
    {
        HRESULT error = HRESULT_FROM_WIN32(GetLastError());
        if(error != ERROR_IO_PENDING)
        {
            log_error("Failed to write '%d' bytes to file '%s'", bytes_to_write, file_data->file_name);
        }

        result = false;
    }

    return(result);
}

mapped_file_t
sys_file_map(string_t filepath)
{
    mapped_file_t result = {};
    result.file = sys_file_open(filepath, false, false, false);
    if(result.file.handle != null)
    {
        s64 size = sys_file_get_size(&result.file);
        if(size == 0) return result;

        result.mapping_handle = CreateFileMappingW(result.mapping_handle, null, PAGE_READONLY, 0, 0, null);
        if(result.mapping_handle && result.file.handle != null)
        {
            void *data = MapViewOfFile(result.mapping_handle, FILE_MAP_READ, 0, 0, size);
            if(data)
            {
                result.mapped_file_data.data  = (u8*)data;
                result.mapped_file_data.count = size;
            }
        }
    }

    
    DWORD error      = GetLastError();
    DWORD error_code = HRESULT_FROM_WIN32(ERROR);
    if(ERROR != 0)
    {
        log_error("Failed to create file mapping and view... error code: '%d', error message: '%s'...\n", error, error_code);
    }

    return(result);
}

bool8 
sys_file_unmap(mapped_file_t *map_data)
{
    bool8 result = true;
    if(map_data->file.handle != INVALID_HANDLE_VALUE)
    {
        if(map_data->mapping_handle)
        {
            if(map_data->mapped_file_data.data)
            {
                UnmapViewOfFile(map_data->mapped_file_data.data);
                map_data->mapped_file_data.data  = null;
                map_data->mapped_file_data.count = 0;
            }

            CloseHandle(map_data->mapping_handle);
            map_data->mapping_handle = null;
        }

        sys_file_close(&map_data->file);
        map_data->file.handle = INVALID_HANDLE_VALUE;
    }
    else
    {
        result = false;
        log_warning("Failed to unmap file... map_data->file.handle is invalid...\n");
    }

    return(result);
}

bool8
sys_file_exists(string_t filepath)
{
    bool8 result = false;
    result = GetFileAttributes(C_STR(filepath)) != INVALID_FILE_ATTRIBUTES;

    return(result);
}

file_data_t
sys_file_get_modtime_and_size(string_t filepath)
{
    file_data_t result = {};
    
    HANDLE file_handle = CreateFileA(C_STR(filepath),
                                     FILE_GENERIC_READ,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     null,
                                     OPEN_ALWAYS,
                                     0,
                                     0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        FILETIME write_time;
        GetFileTime(file_handle, null, null, &write_time);

        ULARGE_INTEGER filetime;
        memcpy(&write_time, &filetime, sizeof(FILETIME));

        LARGE_INTEGER file_size;
        GetFileSizeEx(file_handle, &file_size);

        result.file_size = file_size.QuadPart;
        result.last_modtime = filetime.QuadPart;
        result.filepath     = filepath;
        result.filename     = c_string_get_filename_from_path(filepath);

        CloseHandle(file_handle);
    }
    else
    {
        log_error("Cannot get data for file: '%s'...\n", C_STR(filepath));
    }

    return(result);
}

bool8
sys_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = true;
    
    DWORD attibutes = GetFileAttributes(C_STR(new_file));

    BOOL success = false;
    if(attibutes == INVALID_FILE_ATTRIBUTES)
    {
        success = MoveFile(C_STR(old_file), C_STR(new_file));
    }
    else
    {
        success = ReplaceFile(C_STR(new_file), C_STR(old_file), null, 0, null, null);
    }

    if(!success)
    {
        DWORD error_code = GetLastError();
        log_error("Failed to move file '%s' to '%s'... error code: %d\n",
                  C_STR(old_file), C_STR(new_file), HRESULT_FROM_WIN32(error_code));
        result = false;
    }

    return(result);
}

bool8
sys_directory_exists(string_t filepath)
{
    bool8 result = false;
    DWORD attributes = GetFileAttributes(C_STR(filepath));
    if(attributes != INVALID_FILE_ATTRIBUTES)
    {
        result = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    return(result);
}

void
sys_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    Assert(visit_file_data);
    
    WIN32_FIND_DATA find_data;
    HANDLE          find_handle = INVALID_HANDLE_VALUE;

    u32 cursor = 0;
    DynArray(string_t) directories = c_dynarray_create(string_t);
    c_dynarray_push(directories, filepath);
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(directories); 

    while(cursor < header->size)
    {
        string_t directory_name = c_dynarray_get_at_index(directories, cursor);
        cursor += 1;

        char wildcard_name[256];
        sprintf(wildcard_name, "%s/*", directory_name.data);

        string_t dir_name;
        dir_name.count = c_string_length(wildcard_name);
        dir_name.data  = (byte *)wildcard_name;

        find_handle = FindFirstFileEx(C_STR(dir_name), FindExInfoBasic, &find_data, FindExSearchNameMatch, null, FindExSearchNameMatch);
        if(find_handle == INVALID_HANDLE_VALUE)
        {
            log_error("Filepath: '%s' is invalid... returning...\n", C_STR(filepath));
            return;
        }

        BOOL success = true;
        while(true)
        {
            char *name = find_data.cFileName;
            visit_file_data->filename  = c_string_make_heap(&global_context->temporary_arena, STR(name));
            string_t temp_name         = c_string_concat(&global_context->temporary_arena, directory_name, STR("/"));
            visit_file_data->fullname  = c_string_concat(&global_context->temporary_arena, temp_name, visit_file_data->filename);
 
            bool8 is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if(is_directory)
            {
                if(strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
                {
                    visit_file_data->is_directory = true;
                    if(visit_file_data->recursive)
                    {
                        byte *data = (byte*)c_arena_push_array(&global_context->temporary_arena, byte, visit_file_data->fullname.count);
                        memcpy(data, visit_file_data->fullname.data, visit_file_data->fullname.count);
                        data[visit_file_data->fullname.count] = '\0';

                        string_t data_string = {.data = data, .count = visit_file_data->fullname.count};
                            
                        c_dynarray_push(directories, data_string);
                    }
                }
            }
            else
            {
                visit_file_data->is_directory = false;
                if(visit_file_data->function != null)
                {
                    visit_file_data->function(visit_file_data, visit_file_data->user_data);
                }
            }
            
            success = FindNextFile(find_handle, &find_data);
            if(!success) break;
        }

        FindClose(find_handle);
    }

    c_dynarray_destroy(directories);
}

void*
sys_load_library(string_t filepath)
{
    void *result = null;
    result = (void*)LoadLibraryA(C_STR(filepath));

    return(result);
}

void
sys_free_library(void *library)
{
    FreeLibrary((HMODULE)library);
}

void*
sys_get_proc_address(void *library, string_t procedure)
{
    void *result = null;
    result = (void*)GetProcAddress((HMODULE)library, C_STR(procedure));

    return(result);
}

/* ===========================================
   ======== MULTITHREADING FUNCTIONS =========
   ===========================================*/
s32
sys_get_cpu_count()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    return(system_info.dwNumberOfProcessors);
}

sys_semaphore_t
sys_semaphore_create(s32 initial_thread_count, s32 max_thread_count)
{
    Assert(initial_thread_count <= max_thread_count);
    sys_semaphore_t result = {};

    HANDLE semaphore_handle = CreateSemaphoreExA(null, initial_thread_count, max_thread_count, null, 0, SEMAPHORE_ALL_ACCESS);
    if(semaphore_handle == null)
    {
        log_error("Failure to acquire the Windows semaphore object...\n");
        return(result);
    }

    result.handle = semaphore_handle;
    return(result);
}

void
sys_semaphore_close(sys_semaphore_t *semaphore)
{
    CloseHandle(semaphore->handle);
    semaphore->handle = null;
}

s32
sys_semaphore_release(sys_semaphore_t *semaphore, s32 threads_to_release)
{
    Assert(semaphore);

    s32 result = 0;
    BOOL success = ReleaseSemaphore(semaphore->handle, threads_to_release, (long*)&result);
    if(!success)
    {
        log_error("Failure to release '%d' threads...\n", threads_to_release);
        result = 0;
    }

    return(result);
}

bool8
sys_semaphore_destroy(sys_semaphore_t *semaphore)
{
    Assert(semaphore);

    bool8 result = CloseHandle(semaphore->handle);
    if(!result) log_error("Could not close semaphore handle...\n");

    return(result);
}

void
sys_semaphore_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms)
{
    Assert(semaphore);
    if(wait_duration_ms == 0)
    {
        wait_duration_ms = INFINITE;
    }
    WaitForSingleObjectEx(semaphore->handle, wait_duration_ms, FALSE);
}

sys_thread_t
sys_thread_create(thread_proc_t *proc, void *user_data, bool8 close_handle)
{
    Assert(proc);
    
    sys_thread_t result;
    result.handle = CreateThread(null, 0, (LPTHREAD_START_ROUTINE)proc, user_data, 0, (LPDWORD)&result.thread_id);
    result.user_data = user_data;
    if(result.handle != null)
    {
        if(close_handle) CloseHandle(result.handle);
    }
    else
    {
        log_error("sys_create_thread() failed...\n");
        ZeroStruct(result);
    }

    return(result);
}

bool8
sys_thread_close_handle(sys_thread_t *thread_data)
{
    Assert(thread_data);

    bool8 result = false;
    result = CloseHandle(thread_data->handle);
    if(result == false)
    {
        log_error("Failed to close thread handle...\n");
    }
    
    return(result);
}

sys_mutex_t
sys_mutex_create()
{
    sys_mutex_t result;
    result.handle = CreateMutexA(0, 0, 0);
    if(result.handle == null)
    {
        log_error("Failure to create OS mutex...\n");
    }

    return(result);
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
    Assert(mutex);
    CloseHandle(mutex->handle);

    mutex->handle = null;
}

bool8
sys_mutex_lock(sys_mutex_t *mutex, bool8 should_block)
{
    Assert(mutex);
    bool8 result = false;
    
    if(!should_block)
    {
        DWORD value = WaitForSingleObject(mutex->handle, INFINITE);
        while(value == WAIT_OBJECT_0)
        {
            result = true;
        }
    }
    else
    {
        DWORD value = 0xBEEF;
        while(value != WAIT_OBJECT_0)
        {
            value = WaitForSingleObject(mutex->handle, INFINITE);
        }
        result = true;
    }

    return(result);
}

bool8
sys_mutex_unlock(sys_mutex_t *mutex)
{
    Assert(mutex);
    
    bool8 result = false;
    s32 value    = ReleaseMutex(mutex->handle);
    if(value != 0)
    {
        result = true;
    }

    return(result);
}

/*===========================================
  =============== FILE WATCHER ==============
  ===========================================*/

void
sys_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_sys_watch_data_t *watch_data)
{
}

string_t
c_string_utf8_to_wide(string_t input)
{
    string_t result;

    u32 needed = MultiByteToWideChar(CP_UTF8, 0, (char*)input.data, input.count, null, 0);
    byte *buffer = (byte*)c_arena_push_size(&global_context->temporary_arena, sizeof(u16) * needed);
    u32 count = MultiByteToWideChar(CP_UTF8, 0, (char*)input.data, input.count, (LPWSTR)buffer, needed);

    result.data  = buffer;
    result.count = count;

    LPWSTR str = (LPWSTR)result.data;
    str[count] = L'\0';

    return(result);
}

bool8
sys_file_watcher_add_path(file_watcher_t *watcher, string_t path)
{
    bool8 result = false;
    HANDLE event_handle = CreateEventW(null, FALSE, FALSE, null);
    if(event_handle != null)
    {
        string_t wide_string = c_string_utf8_to_wide(path);
        HANDLE file_handle = CreateFileW((LPWSTR)wide_string.data,
                                         FILE_LIST_DIRECTORY,
                                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                         null,
                                         OPEN_EXISTING,
                                         FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
                                         null);
        if(file_handle == INVALID_HANDLE_VALUE)
        {
            log_error("Could not open file: '%s' error was: '%d'...\n", path, GetLastError());
            CloseHandle(event_handle);

            return(result);
        }

        string_t copied = c_string_make_copy(&watcher->watcher_arena, path);
        sys_file_check_event_data_t *directory = c_arena_push_struct(&watcher->watcher_arena, sys_file_check_event_data_t);
        directory->overlapped_data.hEvent = event_handle;
        directory->overlapped_data.Offset = 0;
        directory->file_handle            = file_handle;
        directory->filename               = copied;
        directory->notify_data            = c_arena_push_size(&watcher->watcher_arena, KB(10));

        u32 count = watcher->sys_watch_data.directory_data_count++;
        watcher->sys_watch_data.directory_data[count] = directory;
        sys_file_watcher_issue_check(watcher, directory);
    
        result = true;
    }

    return(result);
}

void
sys_file_watcher_issue_check(file_watcher_t *watcher, sys_file_check_event_data_t *directory_data)
{
    Assert(directory_data->file_handle != INVALID_HANDLE_VALUE);

    u32 notify_flags = 0;
    if(watcher->events_to_monitor & (FWC_EVENT_ADDED|FWC_EVENT_MOVED|FWC_EVENT_DELETED))
    {
        notify_flags |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_CREATION;
    }
    if(watcher->events_to_monitor & FWC_EVENT_MODIFIED)
    {
        notify_flags |= FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
    }
    if(watcher->events_to_monitor & FWC_EVENT_ATTRIBUTE_CHANGE)
    {
        notify_flags |= FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_ATTRIBUTES;
    }

    memset(&directory_data->overlapped_data, 0, sizeof(OVERLAPPED));
    BOOL success = ReadDirectoryChangesW(directory_data->file_handle,
                                         directory_data->notify_data,
                                         KB(10),
                                         watcher->watch_recursively,
                                         notify_flags,
                                         null,
                                        &directory_data->overlapped_data,
                                         null);
    if(!success)
    {
        HRESULT error = GetLastError();
        if(watcher->is_verbose)
        {
            log_error("ReadDirectoryChanges() failed for directory: '%s'... error: '%d'...\n",
                      directory_data->filename.data, error);
        }

        watcher->issues_when_checking = true;
    }
}

FILE_NOTIFY_INFORMATION*
sys_file_watcher_move_info_forward(FILE_NOTIFY_INFORMATION *info)
{
    FILE_NOTIFY_INFORMATION *result = null;
    if(info->NextEntryOffset != 0)
    {
        result = (FILE_NOTIFY_INFORMATION*)((byte*)info + info->NextEntryOffset);
    }

    return(result);
}

void
sys_file_watcher_process_changes(file_watcher_t *watcher, bool8 *changed)
{
    for(u32 data_index = 0;
        data_index < watcher->sys_watch_data.directory_data_count;
        ++data_index)
    {
        sys_file_check_event_data_t *watch_data = watcher->sys_watch_data.directory_data[data_index];
        if(watch_data->read_failed)
        {
            watch_data->read_failed = false;
            sys_file_watcher_issue_check(watcher, watch_data);
            if(watch_data->read_failed) continue;
        }

        HANDLE file_handle = watch_data->file_handle;
        Assert(file_handle != null);

        u32 bytes_transferred = 0;
        BOOL success = GetOverlappedResult(file_handle, &watch_data->overlapped_data, (LPDWORD)&bytes_transferred, FALSE);
        if(success)
        {
            if(bytes_transferred == 0)
            {
                // NOTE(Sleepster): This means we've overflowed our buffer... 
                c_file_watcher_add_change_event(watcher, watch_data->filename, STR(""), watch_data, FWC_EVENT_MODIFIED|FWC_EVENT_SCAN_CHILDREN);
                continue;
            }

            if(bytes_transferred > 0)
            {
                string_t new_filename = {};

                for(FILE_NOTIFY_INFORMATION *event_data = (FILE_NOTIFY_INFORMATION*)watch_data->notify_data;
                    event_data;
                    event_data = sys_file_watcher_move_info_forward(event_data))
                {
                    char filename[512];
                    int filename_count = WideCharToMultiByte(CP_UTF8,
                                                             0,
                                                             event_data->FileName,
                                                             event_data->FileNameLength / sizeof(WCHAR),
                                                             null, 0, null, null);
                    WideCharToMultiByte(CP_UTF8, 0,
                                        event_data->FileName, event_data->FileNameLength / sizeof(WCHAR),
                                        filename, filename_count, null, null);
                    filename[filename_count] = '\0';

                    string_t filename_str = STR(filename);
                    filename_str = c_string_concat(&global_context->temporary_arena, watch_data->filename, filename_str);
                    c_string_override_file_separators(&filename_str);

                    u32 change_events = 0;
                    switch(event_data->Action)
                    {
                        case FILE_ACTION_ADDED:
                        {
                            change_events |= FWC_EVENT_ADDED;
                        };
                        case FILE_ACTION_MODIFIED:
                        {
                            change_events |= FWC_EVENT_MODIFIED;

                            if(watcher->watch_recursively                 &&
                              ((change_events & FWC_EVENT_MODIFIED) != 0) &&
                               sys_directory_exists(filename_str))
                            {
                                change_events |= FWC_EVENT_SCAN_CHILDREN;
                            }

                            c_file_watcher_add_change_event(watcher, filename_str, new_filename, watch_data, change_events);
                        }break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                        {
                            change_events |= FWC_EVENT_MOVED|FWC_EVENT_RENAMED;
                            watch_data->old_filename = c_string_make_copy(&global_context->temporary_arena, filename_str);
                        }break;
                        case FILE_ACTION_RENAMED_NEW_NAME:
                        {
                            change_events |= FWC_EVENT_MOVED|FWC_EVENT_RENAMED;
                            c_file_watcher_add_change_event(watcher, filename_str, watch_data->old_filename, watch_data, change_events);
                        }break;
                        case FILE_ACTION_REMOVED:
                        {
                            change_events |= FWC_EVENT_DELETED;

                            c_file_watcher_add_change_event(watcher, filename_str, new_filename, watch_data, change_events);
                        }break;
                        default:
                        {
                        }break;
                    }
                }
            }
        }
        else
        {
            DWORD error = GetLastError();
            if (error == ERROR_IO_INCOMPLETE)
            {
                // Still pending; nothing to do this tick.
                if(watcher->is_verbose) log_info("IO PENDING...\n");
                continue;
            }
        }

        sys_file_watcher_issue_check(watcher, watch_data);
    }

    c_file_watcher_emit_changes(watcher);
}

