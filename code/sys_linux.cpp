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
#include <c_string.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_global_context.h>

// NOTE(Sleepster): Defined for mremap() 
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> 
#include <unistd.h>
#include <dlfcn.h>
#include <string.h> 
#include <poll.h>
#include <stdlib.h>

u64 
sys_align_to_page_size(u64 size)
{
    u64 result = 0;
    u64 page_size = sysconf(_SC_PAGESIZE);

    result = Align(size, page_size);

    return(result);
}

// TODO(Sleepster): 
// mmap is actually NOTHING like VirtualAlloc()... mmap will just map all the memory like this. Ideally we just use:
//
// MAP_ANONYMOUS | PROT_NONE
//
// So as to map the data into the virtual memory space, but not actually put it in physical memory (like MEM_RESERVE|MEM_COMMIT)
// and then just use mprotect() to actually MAP the pages into physical memory as needed (mimics MEM_COMMIT).
//
// But, this needs more work.

void*
sys_allocate_memory(usize allocation_size)
{
    u64 true_allocation = sys_align_to_page_size(allocation_size);
    void *data = mmap(0, true_allocation, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(data == MAP_FAILED)
    {
        int error = errno;
        log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

        data = null;
    }

    return(data);
}

// TODO(Sleepster): 
// Again, mmap is weird. You can't just pass the virtual address and say "give me more memory off of this base address" 
// because... idk unix stuff? Again, this is behavior that Windows actually supports better. As for how to replicate on Unix?
// idk yet...
void*
sys_reallocate_memory(void *base, u64 old_size, u64 allocation_size)
{
    void *result = null;
    if(base)
    {
        // TODO(Sleepster): Maybe MREMAP_MAYMOVE but probably not, It cannot be zero though...
        Assert(base);
        errno = 0;

        u64 true_allocation = sys_align_to_page_size(allocation_size);
        u64 old_allocation  = sys_align_to_page_size(old_size);
        void *result = mremap(base, old_allocation, true_allocation, MREMAP_DONTUNMAP|MREMAP_MAYMOVE);
        if(result == MAP_FAILED)
        {
            int error = errno;
            log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

            result = null;
        }
    }
    else
    {
        result = sys_allocate_memory(allocation_size);
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

// TODO(Sleepster): find the equivilent to that of OPEN_ALWAYS from win32
file_t
sys_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io)
{
    file_t result = {};
    result.file_name   = c_string_get_filename_from_path(filepath);
    result.filepath    = filepath;
    result.for_writing = for_writing;
    result.overlapping = overlapping_io;

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

    char buffer[512];
    sprintf(buffer, "%.*s", (s32)filepath.count, C_STR(filepath));

    result.handle = open(buffer, flags, 0666);
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
sys_file_read(file_t *file_data, void *memory, u32 bytes_to_read, u32 file_offset)
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
        log_error("Failure to read file '%s', error: '%s'...\n", C_STR(file_data->filepath), strerror(errno));
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
        log_error("Failure to write file '%s', error: '%s'...\n", C_STR(file_data->filepath), strerror(errno));
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

    const char *c_filepath = c_string_null_terminated(&global_context->temporary_arena, filepath);
    if(stat(c_filepath, &file_stats) == 0)
    {
        result.file_size    = file_stats.st_size;
        result.last_modtime = file_stats.st_mtime;
        result.filepath     = filepath;
        result.filename     = c_string_get_filename_from_path(filepath);
    }
    else
    {
        log_error("Failure to get information about file '%.*s'... error: '%s'\n", fprint_string(filepath), strerror(errno));
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
sys_directory_get_current_working_dir(byte *buffer, u32 buffer_length)
{
    bool8 result = false;
    if(getcwd((char*)buffer, buffer_length) != null)
    {
        result = true;
    }
    else
    {
        s32 error = errno;
        log_error("Failure to get our current working directory... Error: '%s'...\n", strerror(error));
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

s32
sys_directory_get_file_count(memory_arena_t *arena, string_t filepath, bool8 recursive, string_t file_ext)
{
    s32 result = -1;
    
    const char *c_str_filepath = c_string_null_terminated(arena, filepath);
    DIR *directory = opendir(c_str_filepath);
    if(directory)
    {
        result = 0;
        
        struct dirent *file_entry = readdir(directory);
        while(file_entry) 
        {
            if(file_entry->d_type == DT_REG)
            {
                if(file_ext.data == null)
                {
                    ++result;
                }
                else
                {
                    string_t filename = STR(file_entry->d_name);
                    string_t file_extension = c_string_get_file_ext_from_path(filename);
                    if(c_string_compare(file_ext, file_extension))
                    {
                        ++result;
                    }
                }
            }

            if(file_entry->d_type == DT_DIR && recursive)
            {
                if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0) 
                {
                    continue;
                }

                char subdirectory[1024];
                snprintf(subdirectory, sizeof(subdirectory), "%s/%s", C_STR(filepath), file_entry->d_name);

                result += sys_directory_get_file_count(arena, filepath, recursive, file_ext);
            }

            file_entry = readdir(directory);
        };
    }
    else
    {
        log_error("Could not open a directory by name of: %s... make sure the string you pass is null-terminated...\n", filepath.data);
    }

    closedir(directory);
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
            Assert(visit_file_data->filename.data != null);

            string_t temp_name              = c_string_concat(&global_context->temporary_arena, filepath, STR("/"));
            temp_name                       = c_string_concat(&global_context->temporary_arena, temp_name, visit_file_data->filename);
            visit_file_data->fullname       = c_string_make_copy(&global_context->context_arena, temp_name);
            visit_file_data->directory_name = c_string_make_copy(&global_context->temporary_arena, STR(entry->d_name));

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
sys_get_thread_count()
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

// TODO(Sleepster): sys_semaphore_signal 
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
sys_mutex_lock(sys_mutex_t *mutex, const bool8 should_block)
{
    bool8 result = false;
    while(result == false)
    {
        result = SDL_TryLockMutex(mutex->handle);
        if(!should_block) break;
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

internal_api bool8 
sys_internal_file_watcher_add_path(file_watcher_t *watcher, u32 watch_flags, string_t filepath)
{
    bool8 result = false;
    sys_file_check_event_data_t *directory = c_arena_push_struct(&watcher->watcher_arena, sys_file_check_event_data_t);
    if(directory)
    {
        directory->file_data      = sys_file_open(filepath, false, false, false).handle;
        directory->filename       = c_string_get_filename_from_path(filepath);
        directory->inotify_handle = inotify_add_watch(watcher->sys_watch_data.inotify_instance, C_STR(filepath), watch_flags);
        if(directory->inotify_handle != -1)
        {
            u32 count = watcher->sys_watch_data.directory_data_count++;
            watcher->sys_watch_data.directory_data[count] = directory; 

            result = true;
        }
        else
        {
            log_error("Could not watch path '%s'... error: '%s'...\n", filepath.data, strerror(errno));
        }

        if(watcher->watch_recursively)
        {
            const char *c_filepath = c_string_null_terminated(&watcher->watcher_arena, filepath);
            DIR *directory = opendir(c_filepath);
            if(directory)
            {
                struct dirent *entry = null;
                do {
                    entry = readdir(directory);
                    if(entry)
                    {
                        if(entry->d_type == DT_DIR)
                        {
                            if((strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)) 
                            {
                                string_t subdirectory = c_string_concat(&watcher->watcher_arena, filepath, STR(entry->d_name));
                                Assert(sys_internal_file_watcher_add_path(watcher, watch_flags, subdirectory));
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }while(entry);
                closedir(directory);
            }
        }
    }

    return(result);
}

bool8 
sys_file_watcher_add_path(file_watcher_t *watcher, string_t filepath)
{
    bool8 result = false;

    u32 watch_flags = 0;
    if(watcher->events_to_monitor & FWC_EVENT_ADDED)            watch_flags |= IN_CREATE;
    if(watcher->events_to_monitor & FWC_EVENT_MODIFIED)         watch_flags |= IN_MODIFY;
    if(watcher->events_to_monitor & FWC_EVENT_MOVED)            watch_flags |= IN_MOVED_TO|IN_MOVED_FROM|IN_MOVE_SELF;
    if(watcher->events_to_monitor & FWC_EVENT_ATTRIBUTE_CHANGE) watch_flags |= IN_ATTRIB;
    if(watcher->events_to_monitor & FWC_EVENT_DELETED)          watch_flags |= IN_DELETE|IN_DELETE_SELF;

    Assert(sys_internal_file_watcher_add_path(watcher, watch_flags, filepath));
    return(result);
}

void
sys_file_watcher_issue_check(file_watcher_t *watcher, sys_file_check_event_data_t *directory_data)
{
    (void)watcher;
    (void)directory_data;
}

void
sys_file_watcher_process_changes(file_watcher_t *watcher)
{
    file_watcher_sys_watch_data_t *watch_data = &watcher->sys_watch_data;
    Assert(watch_data->inotify_instance != -1);

    s64 bytes_read = read(watch_data->inotify_instance, watch_data->inotify_data, KB(10));
    if(bytes_read != -1 && bytes_read != 0)
    {
        s64 current_read_offset = 0;
        while(current_read_offset < bytes_read)
        {
            struct inotify_event *event = (struct inotify_event*)(((byte*)watch_data->inotify_data) + current_read_offset);

            sys_file_check_event_data_t *directory = null;
            for(u32 directory_index = 0;
                directory_index < watch_data->directory_data_count;
                ++directory_index)
            {
                sys_file_check_event_data_t *found = watch_data->directory_data[directory_index];
                if(found && found->inotify_handle == event->wd)
                {
                    directory = found;
                    break;
                }
            }

            if(directory)
            {
                string_t base_path = directory->filename;
                string_t filename  = STR("");
                if(event->len)
                {
                    filename = STR(event->name);
                }

                string_t fullpath = c_string_concat(&watcher->watcher_arena, base_path, filename);
                if((event->mask & IN_Q_OVERFLOW) == 0)
                {
                    s32 change_events = 0;
                    // NOTE(Sleepster): For now we don't want this behavior for IN_CLOSE_WRITE events since this
                    // sends 2 "modify" events despite being the same change since IN_CLOSE_WRITE triggers when the program file
                    // operating on the file calls fclose() or some deritive of it.
                    //if(event->mask & IN_MODIFY || event->mask & IN_CLOSE_WRITE) change_events |= FWC_EVENT_MODIFIED;
                    if(event->mask & IN_CREATE) change_events |= FWC_EVENT_ADDED;
                    if(event->mask & IN_MODIFY) change_events |= FWC_EVENT_MODIFIED;
                    if(event->mask & IN_ATTRIB) change_events |= FWC_EVENT_ATTRIBUTE_CHANGE;
                    if(event->mask & IN_DELETE) change_events |= FWC_EVENT_DELETED;
                    if(event->mask & IN_MOVED_FROM)
                    {
                        directory->old_filename = c_string_make_copy(&global_context->temporary_arena, fullpath);
                        directory->last_move_cookie = event->cookie;
                    }
                    if(event->mask & IN_MOVED_TO)
                    {
                        if(directory->last_move_cookie != 0 && directory->last_move_cookie == event->cookie)
                        {
                            change_events |= (FWC_EVENT_MOVED | FWC_EVENT_RENAMED);

                            string_t copy_old_fullname = c_string_make_copy(&global_context->temporary_arena, directory->old_filename);
                            string_t copy_new_fullname = c_string_make_copy(&global_context->temporary_arena, filename);
                            c_file_watcher_add_change_event(watcher, copy_new_fullname, copy_old_fullname, directory, change_events);

                            directory->last_move_cookie = 0;
                        }
                        else
                        {
                            change_events |= FWC_EVENT_ADDED;
                            string_t copy_new_fullname = c_string_make_copy(&global_context->temporary_arena, directory->filename);
                            c_file_watcher_add_change_event(watcher, copy_new_fullname, STR(""), directory, change_events);
                        }
                    }

                    bool8 is_recursive_emmision = ((event->mask & IN_MOVED_TO) && (directory->last_move_cookie == 0));
                    if(!is_recursive_emmision)
                    {
                        if(change_events != 0 && !(event->mask & IN_MOVED_TO))
                        {
                            // NOTE(Sleepster): New Directory added. 
                            if(watcher->watch_recursively && (event->mask & IN_ISDIR) && (event->mask & (IN_CREATE|IN_MOVED_TO)))
                            {
                                sys_file_watcher_add_path(watcher, fullpath);
                            }
                        }

                        if(change_events != 0)
                        {
                            if(directory->old_filename.data)
                            {
                                string_t copy_old_fullname = c_string_make_copy(&global_context->temporary_arena, directory->old_filename);
                                string_t copy_new_fullname = c_string_make_copy(&global_context->temporary_arena, filename);

                                c_file_watcher_add_change_event(watcher, copy_new_fullname, copy_old_fullname, directory, change_events);
                            }
                            else
                            {
                                string_t copy_new_fullname = c_string_make_copy(&global_context->temporary_arena, filename);
                                c_file_watcher_add_change_event(watcher, copy_new_fullname, STR(""), directory, change_events);
                            }
                        }
                    }
                }
                else
                {
                    c_file_watcher_add_change_event(watcher,
                                                    directory->filename,
                                                    STR(""),
                                                    directory,
                                                    FWC_EVENT_MODIFIED|FWC_EVENT_SCAN_CHILDREN);
                }
            }

            s64 this_event_size = OffsetOf(struct inotify_event, name) + event->len;
            current_read_offset += this_event_size;
        }
    }
    else if(bytes_read == -1)
    {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            log_error("inotify read error: %s\n", strerror(errno));
            return;
        }
    }

    c_file_watcher_emit_changes(watcher);
}

/*===========================================
  =============== DLL LOADING ===============
  ===========================================*/

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

/*===========================================
  =============== PROCESSSES ================
  ===========================================*/

void*
sys_create_process(string_t program_path, string_t argument_string)
{
    const char **arguments = c_arena_push_array(&global_context->temporary_arena, const char *, 100);
    arguments[0] = C_STR(program_path);

    u32 argument_index = 1;
    while(argument_string.count > 0)
    {
        // NOTE(Sleepster): This seems silly... but there's not really a winner for what character should split the arguments... 
        s32 space_index = c_string_find_first_char_from_left(argument_string, ' ');
        if(space_index != -1)
        {
            string_t copy = c_string_make_copy(&global_context->temporary_arena, argument_string);
            copy.count = space_index;
            copy.data[copy.count] = '\0';

            arguments[argument_index++] = C_STR(copy);
            c_string_advance_by(&argument_string, space_index + 1);
        }
        else
        {
            arguments[argument_index++] = C_STR(argument_string);
            argument_string.count = 0;
        }
    }

    arguments[argument_index] = null;
    SDL_Process *process = SDL_CreateProcess(arguments, false); 
    if(process == null) 
    {
        log_error("SDL_Error: '%s'...\n", SDL_GetError());
    }

    return(process);
}

bool8
sys_wait_for_process(void *process)
{
    bool8 result = false;

    int return_code = 0;
    bool8 exited = SDL_WaitProcess((SDL_Process*)process, true, &return_code);
    Expect(exited == true, "Process failed to exit...\n");

    result = (return_code == 0);
    return(result);
}
