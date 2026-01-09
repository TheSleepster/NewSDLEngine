#if !defined(NEW_ASSET_PACKER_H)
/* ========================================================================
   $File: new_asset_packer.h $
   $Date: January 09 2026 01:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define NEW_ASSET_PACKER_H
#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_globals.h>
#include <c_log.h>
#include <c_string.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_dynarray.h>
#include <c_zone_allocator.h>
#include <c_threadpool.h>
#include <c_memory_arena.h>
#include <c_hash_table.h>
#include <p_platform_data.h>

#include <s_asset_manager.h>

#define FOURCC(string) (((u32)(string[0]) << 0) | ((u32)(string[1]) << 8) | ((u32)(string[2]) << 16) | ((u32)(string[3]) << 24))

#define ASSET_FILE_HEADER_MAGIC (FOURCC("jfd "))
#define ASSET_FILE_CHUNK_MAGIC  (FOURCC("entr"))

#define ASSET_FILE_VERSION (010)

#define MAX_ENTRIES (4096)

#pragma pack(push, 1)
typedef struct jfd_file_header 
{
    u32 magic_value;
    u32 version;
    u32 flags;
    u32 entry_count;
}jfd_file_header_t;

/* NOTE: What do we need? 
 *
 * - header info
 * - filename for hashing in engine
 * - filedata for obvious reasons
 *
 **/

typedef struct jfd_package_chunk_header
{
    u32      magic_value;
    u32      total_entry_size;
    u32      asset_type; 
    u32      filename_count;
    u32      entry_data_coumt;
}jfd_package_chunk_header_t;

typedef struct jfd_package_entry
{
    jfd_package_chunk_header_t chunk_header;
    byte                      *filename_data;
    byte                      *asset_entry_data;
}jfd_package_entry;
#pragma pack(pop)

StaticAssert(sizeof(jfd_file_header_t) % 4 == 0, "jfd file header is not 4 byte aligned...\n");
StaticAssert(sizeof(jfd_package_entry) % 4 == 0, "jfd packge entry struct is not 4 byte aligned...\n");

typedef struct asset_entry_info
{
    string_t     filename;
    string_t     fullpath;
    string_t     asset_data;
    asset_type_t type;
}asset_entry_info_t;


#endif // NEW_ASSET_PACKER_H

