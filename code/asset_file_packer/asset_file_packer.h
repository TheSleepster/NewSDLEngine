#if !defined(ASSET_FILE_PACKER_H)
/* ========================================================================
   $File: asset_file_packer.h $
   $Date: December 10 2025 01:11 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ASSET_FILE_PACKER_H
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
#include <p_platform_data.h>

#include <s_asset_manager.h>
#include <r_asset_texture.h>
#include <r_asset_dynamic_render_font.h>

#define ASSET_FILE_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define ASSET_FILE_VERSION 1UL

#pragma pack(push, 1)
typedef struct asset_file_header
{
    u32 magic_value;
    u32 version;
    u32 flags;
    u32 offset_to_table_of_contents;
}asset_file_header_t;

StaticAssert(sizeof(asset_file_header_t) == 16, "Asset File Packer's Header is not 16 byte aligned...\n");

typedef struct asset_file_table_of_contents
{
    u32 magic_value;
    u32 reserved0;

    s64 entry_count;
    u64 reserved[6];
}asset_file_table_of_contents_t;

typedef struct asset_package_entry
{
    string_t     name;
    string_t     filepath;
    string_t     entry_data;
    u32          ID;
    u32          file_ID;
    asset_type_t type;

    u64          data_offset_from_start_of_file;
}asset_package_entry_t;
#pragma pack(pop)

typedef struct packer_state
{
    string_t              resource_dir_path;
    string_t              output_dir;
    string_t              packed_file_name;
    string_t              file_extension;

    memory_arena_t        packer_arena;
    file_t                asset_file_handle;

    string_builder_t      header;
    string_builder_t      data;
    string_builder_t      table_of_contents;

    asset_package_entry_t entries[4096];
    u32                   next_entry_to_write;
    u32                   next_entry_ID;
    u32                   entry_count;
}packer_state_t;


#endif // ASSET_FILE_PACKER_H

