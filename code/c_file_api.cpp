/* ========================================================================
   $File: c_file_api.cpp $
   $Date: December 08 2025 08:06 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_file_api.h>
#include <c_string.h>

#include <c_globals.h>
#include <p_platform_data.h>

// NOTE(Sleepster): Errors from these calls are handled internally 
file_t
c_file_open(string_t filepath, bool8 create)
{
    file_t result = sys_file_open(filepath, create, false, false);
    result.file_size            = c_file_get_size(&result);
    result.current_read_offset  = 0;
    result.current_write_offset = 0;

    return(result);
}

bool8 
c_file_close(file_t *file)
{
    bool8 result = sys_file_close(file);
    if(!result)
    {
        log_error("Failed to close the file passed, filename: '%s'...", file->file_name.data);
    }

    file->handle = 0;
    return(result);
}

bool8
c_file_copy(string_t old_path, string_t new_path)
{
    bool8 result = sys_file_copy(old_path, new_path);
    return(result);
}

internal_api void*
c_file_allocate_file_data(memory_arena_t *arena, zone_allocator_t *zone, za_allocation_tag_t tag, u32 allocation_size)
{
    void *result = null;
    if(arena  != null) result = c_arena_push_size(arena, allocation_size);
    if(zone   != null) result = c_za_alloc(zone, allocation_size, tag);
    if(result == null) result = AllocSize(allocation_size);
    Assert(result != null);

    return(result);
}

string_t
c_file_read(file_t             *file_data, 
            u32                 bytes_to_read, 
            memory_arena_t     *arena, 
            zone_allocator_t   *zone, 
            za_allocation_tag_t tag,
            bool8               create)
{
    Assert(file_data->handle != INVALID_FILE_HANDLE);

    string_t result;
    void *memory = c_file_allocate_file_data(arena, zone, tag, bytes_to_read);

    result.data  = (byte*)memory;
    result.count = bytes_to_read;
    sys_file_read(file_data, result.data, result.count, file_data->current_read_offset); 
    file_data->current_read_offset += bytes_to_read;
        
    return(result);
}

string_t 
c_file_read_from_offset(file_t             *file_data, 
                        u32                 bytes_to_read, 
                        u32                 offset, 
                        memory_arena_t     *arena, 
                        zone_allocator_t   *zone, 
                        za_allocation_tag_t tag)
{
    Assert(file_data->handle != INVALID_FILE_HANDLE);

    string_t result;
    void *memory = c_file_allocate_file_data(arena, zone, tag, bytes_to_read);
    result.data  = (byte*)memory;
    result.count = bytes_to_read;

    bool8 success = sys_file_read(file_data, memory, bytes_to_read, offset); 
    if(!success)
    {
        result.count = 0;
        log_error("Failure to read '%d' bytes from file '%s' with offset '%s'...\n",
                  bytes_to_read, C_STR(file_data->file_name), bytes_to_read);
    }

    return(result);
}

string_t 
c_file_read_to_end(file_t             *file_data, 
                   u32                 offset, 
                   memory_arena_t     *arena, 
                   zone_allocator_t   *zone, 
                   za_allocation_tag_t tag)
{
    Assert(file_data->handle != INVALID_FILE_HANDLE);
    string_t result;

    u32 bytes_to_read = file_data->file_size - offset;
    result = c_file_read_from_offset(file_data, bytes_to_read, offset, arena, zone, tag);
    Assert(result.data != null);

    return(result);
}


string_t
c_file_read_entirety(string_t            filepath, 
                     memory_arena_t     *arena, 
                     zone_allocator_t   *zone, 
                     za_allocation_tag_t tag)
{
    string_t result;
    file_t file_data = c_file_open(filepath, false);
    Assert(file_data.handle != INVALID_FILE_HANDLE);

    s64 file_size    = c_file_get_size(&file_data);
    result = c_file_read(&file_data, file_size, arena, zone, tag, true);
    if(result.data == null)
    {
        log_error("Failure to read file: '%s'...\n", C_STR(filepath));
    }
    c_file_close(&file_data);

    return(result);
}

bool8 
c_file_write(file_t *file, void *data, s64 bytes_to_write)
{
    Assert(file->for_writing);
    Assert(file->handle != INVALID_FILE_HANDLE);

    bool8 success = sys_file_write(file, data, bytes_to_write);
    file->current_write_offset += bytes_to_write;

    return(success);
}

bool8 
c_file_open_and_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite)
{
    bool8 result = false;
    
    file_t file = sys_file_open(filepath, true, overwrite, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        result = c_file_write(&file, data, bytes_to_write);
    }

    return(result);
}

bool8 
c_file_write_string(file_t *file, string_t data)
{
    Assert(file->for_writing);
    Assert(file->handle != INVALID_FILE_HANDLE);

    bool8 result = c_file_write(file, data.data, data.count);
    return(result);
}

s64
c_file_get_size(file_t *file_data)
{
    Assert(file_data->handle != INVALID_FILE_HANDLE);
    s64 result = 0;

    result = sys_file_get_size(file_data);
    return(result);
}

file_data_t
c_file_get_file_system_info(string_t filepath)
{
    return(sys_file_get_modtime_and_size(filepath));
}

bool8
c_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = false;
    result = sys_file_replace_or_rename(old_file, new_file);

    return(result);
}

mapped_file_t
c_file_map(string_t filepath)
{
    return(sys_file_map(filepath));
}

bool8
c_file_unmap(mapped_file_t *map_data)
{
    return(sys_file_unmap(map_data));
}

u32
c_file_ext_string_to_enum(string_t file_extension)
{
    u32 result = 0;
    if(c_string_compare(file_extension, STR(".ttf")))
    {
        result = FILE_EXT_TTF;
        return(result);
    }
    if(c_string_compare(file_extension, STR(".wav")))
    {
        result = FILE_EXT_WAV;
        return(result);
    }
    if(c_string_compare(file_extension, STR(".png")))
    {
        result = FILE_EXT_PNG;
        return(result);
    }
    if(c_string_compare(file_extension, STR(".glsl")))
    {
        result = FILE_EXT_GLSL;
        return(result);
    }
    if(c_string_compare(file_extension, STR(".dll")))
    {
        result = FILE_EXT_OS_DLL;
        return(result);
    }
    if(c_string_compare(file_extension, STR(".so")))
    {
        result = FILE_EXT_OS_DLL;
        return(result);
    }

    return(result);
}

/////////////////
// DIRECTORY
////////////////

visit_file_data_t
c_directory_create_visit_data(visit_files_pfn_t *function, bool8 recursive, void *user_data)
{
    visit_file_data_t result = {};

    result.function  = function;
    result.recursive = recursive;
    result.user_data = user_data;

    return(result);
}

void
c_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    return(sys_directory_visit(filepath, visit_file_data));
}

bool8
c_directory_exists(string_t filepath)
{
    return(sys_directory_exists(filepath));
}

