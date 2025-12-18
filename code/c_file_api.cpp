/* ========================================================================
   $File: c_file_api.cpp $
   $Date: December 08 2025 08:06 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_file_api.h>
#include <c_globals.h>
#include <p_platform_data.h>

// NOTE(Sleepster): Errors from these calls are handled internally 
file_t
c_file_open(string_t filepath, bool8 create)
{
    return(sys_file_open(filepath, create, false, false));
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

u32
c_file_get_read_size(file_t *file, u32 bytes_to_read, u32 file_offset)
{
    u32 result = 0;

    u32 file_size = sys_file_get_size(file);
    if(bytes_to_read == U32_MAX)
    {
        // read the whole file
        result = file_size;
    }
    else if(bytes_to_read == 0)
    {
        // read what's left of the file from the offset
        result = file_size - file_offset;
    }
    else
    {
        // read the amount desired from the file
        result = bytes_to_read;
    }

    return(result);
}

// TODO(Sleepster): These should all call back to one routine and have the other memory methods call to this single call... this is stupid... 
string_t
c_file_read(string_t filepath, u32 bytes_to_read, u32 file_offset)
{
    string_t result = {};

    file_t file = sys_file_open(filepath, false, false, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = (u8 *)sys_allocate_memory(sizeof(u8) * bytes_to_read);

        sys_file_read(&file, data, file_offset, bytes_to_read);
        sys_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

string_t
c_file_read_arena(memory_arena_t *arena, string_t filepath, u32 bytes_to_read, u32 file_offset)
{
    string_t result = {};
    
    file_t file = sys_file_open(filepath, false, false, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = (byte*)c_arena_push_size(arena, sizeof(u8) * bytes_to_read);

        sys_file_read(&file, data, file_offset, bytes_to_read);
        sys_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

string_t
c_file_read_za(zone_allocator_t *zone, string_t filepath, u32 bytes_to_read, u32 file_offset, za_allocation_tag_t tag)
{
    string_t result = {};
    
    file_t file = sys_file_open(filepath, false, false, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = c_za_alloc(zone, sizeof(u8) * bytes_to_read, tag);

        sys_file_read(&file, data, file_offset, bytes_to_read);
        sys_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

bool8 
c_file_open_and_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite)
{
    bool8 result = false;
    
    file_t file = sys_file_open(filepath, true, overwrite, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        sys_file_write(&file, data, bytes_to_write);
        result = true;
    }

    return(result);
}

bool8 
c_file_write(file_t *file, void *data, s64 bytes_to_write)
{
    return(sys_file_write(file, data, bytes_to_write));
}

bool8 
c_file_write_string(file_t *file, string_t data)
{
    return(c_file_write(file, data.data, data.count));
}

s64
c_file_get_size(string_t filepath)
{
    s64 result = 0;
    
    file_t file = sys_file_open(filepath, false, false, false);
    if(file.handle != INVALID_FILE_HANDLE)
    {
        result = sys_file_get_size(&file);
        sys_file_close(&file);
    }

    return(result);
}

file_data_t
c_file_get_data(string_t filepath)
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

