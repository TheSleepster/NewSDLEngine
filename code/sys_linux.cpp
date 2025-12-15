/* ========================================================================
   $File: sys_linux.cpp $
   $Date: December 08 2025 03:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <sys_linux.h>

#include <c_types.h>
#include <c_base.h>
#include <c_log.h>
#include <c_file_api.h>
#include <c_file_watcher.h>

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
#include <stdlib.h>

void*
sys_allocate_memory(usize allocation_size)
{
    errno = 0;
    
    void *data = mmap(0, allocation_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(errno == -1)
    {
        int error = errno;
        log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

        data = null;
    }

    return(data);
}

void*
sys_reallocate_memory(void *offset, u64 allocation_size)
{
    errno = 0;
    
    void *result = mmap(offset, allocation_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(errno == -1)
    {
        int error = errno;
        log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

        result = null;
    }

    return(result);
}

void
sys_free_memory(void *data, usize free_size)
{
    if(munmap(data, free_size) == -1)
    {
        int error = errno;
        log_fatal("munmap failed... error: (%s), code: '%d'...\n", strerror(error), error);
    }
}

//////////////////////
// FILE IO STUFF
/////////////////////

file_t
sys_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io)
{
    file_t result = {};
    result.file_name = c_string_get_filename_from_path(filepath);
    result.filepath  = filepath;

    s32 flags = 0;
    if(for_writing)
    {
        result.for_writing = true;

        if(overwrite)
        {
            flags = O_RDWR|O_CREAT;
        }
        else
        {
            flags = O_RDWR|O_CREAT|O_TRUNC;
        }
    }
    else
    {
        flags = O_RDONLY;
    }

    if(overlapping_io)
    {
        result.overlapping = true;
        flags |= O_NONBLOCK;
    }

    result.handle = open(C_STR(filepath), flags, 0666);
    if(result.handle == -1)
    {
        log_error("Failure to open file '%s' for %s, error: '%s'...\n",
                  filepath.data, for_writing ? "writing" : "reading",
                  strerror(errno));

        ZeroStruct(result);
        result.handle = -1;
    }

    return(result);
}

bool8
sys_file_close(file_t *file_data)
{
    bool8 result = false;
    
    Assert(file_data->handle != -1);
    if(close(file_data->handle) == 0)
    {
        file_data->handle = -1;
        result = true;
    }

    return(result);
}

bool8
sys_file_copy(string_t source_path, string_t dest_path)
{
    bool8 result = false;

    s32 source_handle = open(C_STR(source_path), O_RDONLY);
    if(source_handle < 0)
    {
        log_error("Failed to open source file '%s' for reading: %s\n",
                  source_path.data, strerror(errno));
        return(result);
    }

    struct stat src_stat;
    if(fstat(source_handle, &src_stat) < 0)
    {
        log_error("fstat failed on source '%s': %s\n", source_path.data, strerror(errno));
        close(source_handle);
        return(result);
    }

    s32 dest_handle = open(C_STR(dest_path), O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode & 0777);
    if(dest_handle < 0)
    {
        log_error("Failed to open destination file '%s' for writing: %s\n",
                  dest_path.data, strerror(errno));
        close(source_handle);
        return(result);
    }

    const u64 CHUNK = (1 << 20);
    bool use_fallback = false;

    for(;;)
    {
        ssize_t copied = copy_file_range(source_handle, NULL, dest_handle, NULL, (size_t)CHUNK, 0);
        if(copied > 0)
        {
            continue;
        }
        else if(copied == 0)
        {
            break;
        }
        else 
        {
            if(errno == EINTR)
            {
                continue;
            }

            if(errno == ENOSYS || errno == EINVAL || errno == ESPIPE)
            {
                use_fallback = true;
                break;
            }

            log_error("copy_file_range failed copying '%s' -> '%s': %s\n",
                      source_path.data, dest_path.data, strerror(errno));
            close(source_handle);
            close(dest_handle);
            return(result);
        }
    }

    if(use_fallback)
    {
        if(lseek(source_handle, 0, SEEK_SET) == (off_t)-1)
        {
            log_error("lseek failed on source '%s': %s\n", source_path.data, strerror(errno));
            close(source_handle);
            close(dest_handle);
            return(result);
        }

        char *buf = (char *)malloc((size_t)CHUNK);
        if(!buf)
        { 
            log_error("Out of memory while copying '%s' -> '%s'\n", source_path.data, dest_path.data);
            close(source_handle);
            close(dest_handle);
            return(result);
        }

        for(;;)
        {
            ssize_t r = read(source_handle, buf, (size_t)CHUNK);
            if(r > 0)
            {
                ssize_t wrote = 0;
                while(wrote < r)
                {
                    ssize_t w = write(dest_handle, buf + wrote, (size_t)(r - wrote));
                    if(w < 0)
                    {
                        if(errno == EINTR) continue;
                        log_error("write failed while copying to '%s': %s\n", dest_path.data, strerror(errno));
                        free(buf);
                        close(source_handle);
                        close(dest_handle);
                        return(result);
                    }
                    wrote += w;
                }
            }
            else if(r == 0)
            {
                break;
            }
            else
            {
                if(errno == EINTR) continue;
                log_error("read failed while copying from '%s': %s\n", source_path.data, strerror(errno));
                free(buf);
                close(source_handle);
                close(dest_handle);
                return(result);
            }
        }

        free(buf);
    }

    if(fchmod(dest_handle, src_stat.st_mode & 0777) != 0)
    {
        log_warning("Failed to set permissions on '%s': %s\n", dest_path.data, strerror(errno));
    }

    fsync(dest_handle);
    close(source_handle);
    close(dest_handle);

    result = true;
    return(result);
}


s64
sys_file_get_size(file_t *file_data)
{
    s64 file_size = 0;
    
    struct stat file_stats = {};
    if(fstat(file_data->handle, &file_stats) != -1)
    {
        file_size = file_stats.st_size;
    }
    else
    {
        log_error("Failure to get the file size for file: '%s', error: '%s'...\n",
                  file_data->filepath, strerror(errno));
    }

    return(file_size);
}

bool8
sys_file_read(file_t *file_data, void *memory, u32 file_offset, u32 bytes_to_read)
{
    bool8 result = false;

    usize bytes_read = 0; 
    if(file_offset != 0)
    {
        if(lseek(file_data->handle, file_offset, SEEK_SET) == -1)
        {
            log_error("Failure to set the file pointer...\n");
        }
    }

    bytes_read = read(file_data->handle, memory, bytes_to_read);
    if(bytes_read  == bytes_to_read)
    {
        result = true;
    }

    if(bytes_read == 0)
    {
        log_error("Failure to read file '%s', error: '%s'...\n", file_data->filepath, strerror(errno));
    }

    return(result);
}

bool8
sys_file_write(file_t *file_data, void *memory, usize bytes_to_write)
{
    bool8 result = false;
    
    usize bytes_written = write(file_data->handle, memory, bytes_to_write);
    if(bytes_written == bytes_to_write)
    {
        result = true;
    }
    else
    {
        log_error("Failure to write file '%s', error: '%s'...\n", file_data->filepath, strerror(errno));
    }

    return(result);
}

mapped_file_t
sys_file_map(string_t filepath)
{
    mapped_file_t result = {};

    result.file = sys_file_open(filepath, false, false, false);
    if(result.file.handle >= 0)
    {
        s64 file_size = sys_file_get_size(&result.file);
        if(file_size == 0)
        {
            return(result);
        }

        void *mapped_data = mmap(null, file_size, PROT_READ, MAP_PRIVATE, result.file.handle, 0);
        if(mapped_data == MAP_FAILED)
        {
            log_error("MMAP failed to map the data for file: '%s'... error: '%s'...\n", result.file.filepath, strerror(errno));
        }

        result.mapped_file_data.data  = (byte*)mapped_data;
        result.mapped_file_data.count = file_size;
    }

    return(result);
}

bool8
sys_file_unmap(mapped_file_t *map_data)
{
    bool8 result = false;
    
    Assert(map_data->mapped_file_data.data  != null);
    Assert(map_data->mapped_file_data.count != 0);

    if(munmap(map_data->mapped_file_data.data, map_data->mapped_file_data.count) == 0)
    {
        result = true;
    }
    else
    {
        log_error("Failure to unmap file: '%s'... error: '%s'...\n", map_data->file.filepath, strerror(errno));
    }

    return(result);
}

bool8
sys_file_exists(string_t filepath)
{
    bool8 result = false;

    struct stat file_stats;
    result = (stat(C_STR(filepath), &file_stats) == 0);

    return(result);
}

file_data_t
sys_file_get_modtime_and_size(string_t filepath)
{
    file_data_t result = {};
    struct stat file_stats;
    if(stat(C_STR(filepath), &file_stats) == 0)
    {
        result.file_size    = file_stats.st_size;
        result.last_modtime = file_stats.st_mtime;
        result.filepath     = filepath;
        result.filename     = c_string_get_filename_from_path(filepath);
    }
    else
    {
        log_error("Failure to get information about file '%s'... error: '%s'\n", filepath.data, strerror(errno));
    }

    return(result);
}

bool8
sys_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = false;
    if(rename(C_STR(old_file), C_STR(new_file)) == 0)
    {
        result = true;
    }
    else
    {
        log_error("Failure to rename file '%s' to that of '%s', error: '%s'...\n",
                  old_file.data, new_file.data, strerror(errno));
    }

    return(result);
}

bool8
sys_directory_exists(string_t filepath)
{
    bool8 result = false;
    struct stat file_stats;
    if(stat(C_STR(filepath), &file_stats) == 0)
    {
        result = S_ISDIR(file_stats.st_mode);
    }

    return(result);
}

void
sys_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    DIR *directory = opendir(C_STR(filepath));
    if(directory != null)
    {
        struct dirent *entry;
        while((entry = readdir(directory)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            visit_file_data->filename = c_string_make_heap(&global_context->temporary_arena, STR(entry->d_name));
            string_t temp_name        = c_string_concat(&global_context->temporary_arena, filepath, STR("/"));
            visit_file_data->fullname = c_string_concat(&global_context->temporary_arena, temp_name, visit_file_data->filename);

            bool8 is_directory            = (entry->d_type == DT_DIR);
            visit_file_data->is_directory = is_directory;

            if(is_directory && visit_file_data->recursive)
            {
                sys_directory_visit(visit_file_data->fullname, visit_file_data);
            }
            else if(!is_directory && visit_file_data->function != null)
            {
                visit_file_data->function(visit_file_data, visit_file_data->user_data);
            }
        }
        closedir(directory);
    }
    else
    {
        log_error("Failure to open the directory: '%s'... error of: '%s'...\n",
                  filepath.data, strerror(errno));
    }
}
/* ===========================================
   ======== MULTITHREADING FUNCTIONS =========
   ===========================================*/
#include <SDL3/SDL.h>

s32
sys_get_cpu_count()
{
    s32 result = 0;
    result = SDL_GetNumLogicalCPUCores();

    return(result);
}

sys_semaphore_t
sys_semaphore_create(s32 initial_thread_count, s32 max_thread_count)
{
    Assert(initial_thread_count < max_thread_count);
    sys_semaphore_t result = {};
    sys_semaphore_handle_t semaphore_handle = SDL_CreateSemaphore(initial_thread_count);
    if(semaphore_handle != null)
    {
        result.handle = semaphore_handle;
    }
    else
    {
        log_error("Failed to create an SDL_Semaphore... Error: %s\n", SDL_GetError());
    }

    return(result);
}

void
sys_semaphore_close(sys_semaphore_t *semaphore)
{
    Assert(semaphore         != null);
    Assert(semaphore->handle != null);
    SDL_DestroySemaphore(semaphore->handle);

    semaphore->handle = null;
}

s32
sys_semaphore_release(sys_semaphore_t *semaphore, s32 threads_to_release)
{
    Assert(semaphore         != null);
    Assert(semaphore->handle != null);
    s32 result = 0;

    for(s32 awake_index = 0;
        awake_index < threads_to_release;
        ++awake_index)
    {
        SDL_SignalSemaphore(semaphore->handle);
        result++;
    }

    return(result);
}

bool8
sys_semaphore_destroy(sys_semaphore_t *semaphore)
{
    Assert(semaphore         != null);
    Assert(semaphore->handle != null);
        
    SDL_DestroySemaphore(semaphore->handle);
    semaphore->handle = null;

    return(true);
}

void
sys_semaphore_wait(sys_semaphore_t *semaphore, u64 wait_duration_ms)
{
    if(wait_duration_ms == 0)
    {
        SDL_WaitSemaphore(semaphore->handle);
    }
    else
    {
        SDL_WaitSemaphoreTimeout(semaphore->handle, wait_duration_ms);
    }
}

sys_thread_t
sys_thread_create(thread_proc_t *proc, void *user_data, bool8 close_handle)
{
    sys_thread_t result;
    result.handle    = SDL_CreateThread((SDL_ThreadFunction)proc, null, user_data);
    if(result.handle)
    {
        result.user_data = user_data;
        result.thread_id = SDL_GetThreadID(result.handle);
        if(close_handle)
        {
            //SDL_DetachThread(result.handle);
            result.handle = null;
        }
    }
    else
    {
        log_error("Could not create an SDL_Thread... Error: %s\n", SDL_GetError());
    }

    return(result);
}

// NOTE(Sleepster): There is no "close handle" like there is on Windows to my knowledge.
bool8
sys_thread_close_handle(sys_thread_t *thread_data)
{
    Assert(thread_data);
    Assert(thread_data->handle);
    //SDL_WaitThread(thread_data->handle, null);

    return(true);
}

sys_mutex_t
sys_mutex_create()
{
    sys_mutex_t result;
    result.handle = SDL_CreateMutex();
    if(result.handle == null)
    {
        log_error("Failure to generate an SDL_Mutex... Error: %s\n", SDL_GetError());
    }
    return(result);
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
    Assert(mutex);
    Assert(mutex->handle);

    SDL_DestroyMutex(mutex->handle);
    mutex->handle = null;
}

bool8
sys_mutex_lock(sys_mutex_t *mutex, bool8 should_block)
{
    bool8 result = false;
    if(should_block)
    {
        SDL_LockMutex(mutex->handle);
        result = true;
    }
    else
    {
        result = SDL_TryLockMutex(mutex->handle);
    }
    return(result);
}

bool8
sys_mutex_unlock(sys_mutex_t *mutex)
{
    SDL_UnlockMutex(mutex->handle);
    return(true);
}

/*===========================================
  =============== FILE WATCHER ==============
  ===========================================*/

void
sys_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_sys_watch_data_t *watch_data)
{
    watch_data->inotify_instance = inotify_init1(IN_NONBLOCK);
    if(watch_data->inotify_instance == -1)
    {
        log_error("Could not init an Inotify instance... error: %s...\n", strerror(errno));
        return;
    }
    watch_data->inotify_data = c_arena_push_size(arena, KB(10));
}

bool8 
sys_file_watcher_add_path(file_watcher_t *watcher, string_t path)
{
    bool8 result = false;
    string_t filepath = c_string_make_copy(&watcher->watcher_arena, path);

    u32 flags = 0;
    if(watcher->events_to_monitor & FWC_EVENT_ADDED)
    {
        flags |= IN_CREATE;
    }
    if(watcher->events_to_monitor & FWC_EVENT_MODIFIED)
    {
        flags |= IN_MODIFY|IN_CLOSE_WRITE;
    }
    if(watcher->events_to_monitor & FWC_EVENT_MOVED)
    {
        flags |= IN_MOVED_TO|IN_MOVED_FROM|IN_MOVE_SELF;
    }
    if(watcher->events_to_monitor & FWC_EVENT_ATTRIBUTE_CHANGE)
    {
        flags |= IN_ATTRIB;
    }
    if(watcher->events_to_monitor & FWC_EVENT_DELETED)
    {
        flags |= IN_DELETE|IN_DELETE_SELF;
    }

    sys_file_check_event_data_t *directory = c_arena_push_struct(&watcher->watcher_arena, sys_file_check_event_data_t);
    if(directory)
    {
        directory->file_data      = sys_file_open(path, false, false, false).handle;
        directory->filename       = c_string_make_copy(&watcher->watcher_arena, c_string_get_filename_from_path(path));
        directory->inotify_handle = inotify_add_watch(watcher->sys_watch_data.inotify_instance, C_STR(filepath), flags);
        if(directory->inotify_handle == -1)
        {
            log_error("Could not watch path '%s'... error: '%s'...\n", filepath.data, strerror(errno));
            return(result);
        }

        u32 count = watcher->sys_watch_data.directory_data_count++;
        watcher->sys_watch_data.directory_data[count] = directory; 

        result = true;
    }
    return(result);
}

void
sys_file_watcher_issue_check(file_watcher_t *watcher, sys_file_check_event_data_t *directory_data)
{
    (void)watcher;
    (void)directory_data;
}

void
sys_file_watcher_process_changes(file_watcher_t *watcher, bool8 *changed)
{
    file_watcher_sys_watch_data_t *osdata = &watcher->sys_watch_data;
    s32 inotify_instance = osdata->inotify_instance;
    if(inotify_instance == -1) return;

    s64 bytes_read = read(inotify_instance, osdata->inotify_data, KB(10));
    if(bytes_read == -1)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        else
        {
            log_error("inotify read error: %s\n", strerror(errno));
            return;
        }
    }
    if(bytes_read == 0) return;

    u64 offset = 0;
    while(offset < (u64)bytes_read)
    {
        struct inotify_event *event = (struct inotify_event*)((byte*)osdata->inotify_data + offset);

        sys_file_check_event_data_t *directory_data = NULL;
        for(u32 i = 0; i < watcher->sys_watch_data.directory_data_count; ++i)
        {
            sys_file_check_event_data_t *found = watcher->sys_watch_data.directory_data[i];
            if(found && found->inotify_handle == event->wd)
            {
                directory_data = found;
                break;
            }
        }

        if(!directory_data)
        {
            u64 this_event_size = IntFromPtr(OffsetOf(struct inotify_event, name) + event->len);
            offset += this_event_size;
            continue;
        }

        string_t base_path = directory_data->filename;

        string_t name_str = STR("");
        if(event->len)
        {
            name_str = STR(event->name);
        }

        string_t full_path = c_string_concat(&global_context->temporary_arena, base_path, name_str);
        c_string_override_file_separators(&full_path);

        if(event->mask & IN_Q_OVERFLOW)
        {
            c_file_watcher_add_change_event(watcher,
                                            directory_data->filename,
                                            STR(""),
                                            directory_data,
                                            FWC_EVENT_MODIFIED|FWC_EVENT_SCAN_CHILDREN);
        }
        else
        {
            u32 change_events = 0;

            if(event->mask & IN_CREATE)
            {
                change_events |= FWC_EVENT_ADDED;
            }
            if(event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE)
            {
                change_events |= FWC_EVENT_MODIFIED;
            }
            if(event->mask & IN_ATTRIB)
            {
                change_events |= FWC_EVENT_ATTRIBUTE_CHANGE;
            }
            if(event->mask & IN_DELETE)
            {
                change_events |= FWC_EVENT_DELETED;
            }

            if(event->mask & IN_MOVED_FROM)
            {
                directory_data->old_filename = c_string_make_copy(&global_context->temporary_arena, full_path);
                directory_data->last_move_cookie = event->cookie;
            }
            if(event->mask & IN_MOVED_TO)
            {
                if(directory_data->last_move_cookie != 0 && directory_data->last_move_cookie == event->cookie)
                {
                    change_events |= (FWC_EVENT_MOVED | FWC_EVENT_RENAMED);
                    c_file_watcher_add_change_event(watcher, full_path, directory_data->old_filename, directory_data, change_events);

                    directory_data->last_move_cookie = 0;
                    directory_data->old_filename = STR("");
                }
                else
                {
                    change_events |= FWC_EVENT_ADDED;
                    c_file_watcher_add_change_event(watcher, full_path, STR(""), directory_data, change_events);
                }
            }

            bool emitted_by_prev_branch = ((event->mask & IN_MOVED_TO) && directory_data->last_move_cookie == 0 );
            if(!emitted_by_prev_branch)
            {
                if(change_events != 0 && !(event->mask & IN_MOVED_TO))
                {
                    if(watcher->watch_recursively && (event->mask & IN_ISDIR) && (event->mask & (IN_CREATE|IN_MOVED_TO)))
                    {
                        u32 event_flags = 0;
                        if(watcher->events_to_monitor & FWC_EVENT_ADDED)
                        {
                            event_flags |= IN_CREATE;
                        }
                        if(watcher->events_to_monitor & FWC_EVENT_MODIFIED)
                        {
                            event_flags |= IN_MODIFY | IN_CLOSE_WRITE;
                        }
                        if(watcher->events_to_monitor & FWC_EVENT_MOVED)
                        {
                            event_flags |= IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF;
                        }
                        if(watcher->events_to_monitor & FWC_EVENT_ATTRIBUTE_CHANGE)
                        {
                            event_flags |= IN_ATTRIB;
                        }
                        if(watcher->events_to_monitor & FWC_EVENT_DELETED)
                        {
                            event_flags |= IN_DELETE | IN_DELETE_SELF;
                        }

                        int sub_wd = inotify_add_watch(inotify_instance,
                                                       C_STR(full_path),
                                                       event_flags);
                        if(sub_wd != -1)
                        {
                            sys_file_check_event_data_t *new_dir = c_arena_push_struct(&watcher->watcher_arena, sys_file_check_event_data_t);
                            if(new_dir)
                            {
                                new_dir->file_data = sys_file_open(full_path, false, false, false).handle;
                                new_dir->filename  = c_string_make_copy(&watcher->watcher_arena, full_path);
                                new_dir->inotify_handle = sub_wd;
                                new_dir->last_move_cookie = 0;
                                new_dir->old_filename = STR("");
                                u32 idx = watcher->sys_watch_data.directory_data_count++;
                                watcher->sys_watch_data.directory_data[idx] = new_dir;
                            }
                        }
                    }

                    c_file_watcher_add_change_event(watcher, full_path, STR(""), directory_data, change_events);
                }
            }
        }

        size_t this_event_size = sizeof(struct inotify_event);
        offset += this_event_size;
    }

    c_file_watcher_emit_changes(watcher);
}

void*
sys_load_library(string_t filepath)
{
    void *result = dlopen(C_STR(filepath), RTLD_NOW);
    if(result == null)
    {
        log_fatal("Could not load library: %s... Error: %s...\n", C_STR(filepath), dlerror());
    }
    return result;
}

void
sys_free_library(void *library)
{
    if(library)
    {
        dlclose(library);
    }
}

void*
sys_get_proc_address(void *library, string_t procedure)
{
    void *result = null;
    if(library)
    {
        result = dlsym(library, C_STR(procedure));
    }
    return result;
}

