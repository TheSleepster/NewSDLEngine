#if !defined(C_FILE_API_H)
/* ========================================================================
   $File: c_file_api.h $
   $Date: December 08 2025 07:08 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_FILE_API_H
#include <c_types.h>
#include <c_string.h>

#include <c_zone_allocator.h>
#include <p_platform_data.h>

/* TODO:
 * 1.) Seperate c_file_read_all() and c_file_read() with an offset
 * 2.) Make ALL of the file operations work around the file_t type
 * 3.) Make it more clear how the file operations affect the actual contents of the files
 */

// TODO(Sleepster): THREAD SAFE OVERLAPPING IO 

// NOTE(Sleepster): This is stupid... but C doesn't make this easier. 
typedef struct zone_allocator zone_allocator_t;

typedef enum file_extension
{
    FILE_EXT_INVALID,
    FILE_EXT_TTF,
    FILE_EXT_WAV,
    FILE_EXT_PNG,
    FILE_EXT_GLSL,
    FILE_EXT_OS_DLL,
    FILE_EXT_COUNT
}file_extension_t;

typedef struct file
{
    sys_handle_t handle;
    string_t    file_name;
    string_t    filepath;

    u64         file_size;
    u64         current_read_offset;
    u64         current_write_offset;

    bool8       overlapping;
    bool8       for_writing;
}file_t;

typedef struct mapped_file_data
{
    file_t      file;
    sys_handle_t mapping_handle;
    string_t    mapped_file_data;
}mapped_file_t;

typedef struct file_data
{
    u64      last_modtime;
    u64      file_size;

    string_t filename;
    string_t filepath;
}file_data_t;

typedef struct overlap_io_data 
{
    u64          offset_to_read;
    u64          bytes_to_read;

    u32          status;
    u32          bytes_transfered;

    sys_handle_t event_handle;
}overlap_io_data_t;

struct visit_file_data;
#define VISIT_FILES(name) void name(struct visit_file_data *visit_file_data, void *user_data)
typedef VISIT_FILES(visit_files_pfn_t);

typedef struct visit_file_data
{
    visit_files_pfn_t *function;
    void              *user_data;

    string_t           filename;
    string_t           fullname;

    bool8              recursive;
    bool8              is_directory;
}visit_file_data_t;

/* TODO(Sleepster): c_file_write_overlapped()
 *
 * We just generally need async fileIO...
 *
 * We might want to be able to supply an offset to c_write_file_*
 * functions for specific operating systems.  Windows keeps a write
 * pointer for each file you create with CreateFile... But I don't
 * know how Linux or Mac does it...
 */

/*===========================================
  =============== GENERAL API ===============
  ===========================================*/
file_t            c_file_open(string_t filepath, bool8 create);
bool8             c_file_close(file_t *file);
bool8             c_file_copy(string_t old_path, string_t new_path);


string_t          c_file_read(file_t *file_data, u32 bytes_to_read, u32 offset, memory_arena_t *arena = null, zone_allocator_t *zone = null, za_allocation_tag_t tag = ZA_TAG_STATIC, bool8 create = true);
string_t          c_file_read_entirety(string_t filepath, memory_arena_t *arena = null, zone_allocator_t *zone = null, za_allocation_tag_t tag = ZA_TAG_STATIC);
string_t          c_file_read_from_offset(file_t *file_data, u32 bytes_to_read, u32 offset, memory_arena_t *arena = null, zone_allocator_t *zone = null, za_allocation_tag_t tag = ZA_TAG_STATIC);
string_t          c_file_read_to_end(file_t *file_data, u32 offset, memory_arena_t *arena = null, zone_allocator_t *zone = null, za_allocation_tag_t tag = ZA_TAG_STATIC);

bool8             c_file_open_and_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite);
bool8             c_file_write(file_t *file, void *data, s64 bytes_to_write);
bool8             c_file_write_string(file_t *file, string_t data);

s64               c_file_get_size(file_t *file_data);
file_data_t       c_file_get_file_system_info(string_t filepath);
bool8             c_file_replace_or_rename(string_t old_file, string_t new_file);
mapped_file_t     c_file_map(string_t filepath);
bool8             c_file_unmap(mapped_file_t *map_data);
u32               c_file_ext_string_to_enum(string_t file_ext);

bool8             c_directory_exists(string_t filepath);
visit_file_data_t c_directory_create_visit_data(visit_files_pfn_t *function, bool8 recursive, void *user_data);
void              c_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);

#endif // C_FILE_API_H

