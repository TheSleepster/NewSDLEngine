#if !defined(S_ASSET_MANAGER_H)
/* ========================================================================
   $File: s_asset_manager.h $
   $Date: January 06 2026 04:57 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ASSET_MANAGER_H
#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_hash_table.h>

#include <r_vulkan.h>

#define ASSET_MANAAGER_CATALOG_COUNT  (2)
#define ASSET_CATALOG_MAX_LOOKUPS     (4096)
#define ASSET_MANAGER_MAX_ASSET_FILES (32)

typedef enum asset_type
{
    AT_Invalid,
    AT_Bitmap,
    AT_Shader,
}asset_type_t;

typedef enum asset_slot_state
{
    ASLS_Invalid,
    ASLS_Unloaded,
    ASLS_LoadQueued,
    ASLS_Loaded,
    ASLS_ShouldUnload,
    ASLS_ShouldReload,
    ASLS_Count
}asset_slot_load_status;

typedef enum bitmap_format
{
    BMF_Invalid,
    BMF_R8,
    BMF_RGBA32,
    BMF_RGB24,
    BMF_Count,
}bitmap_format_t;

typedef struct bitmap
{
    u32      width;
    u32      height;
    u32      channels;
    u32      format;

    // NOTE(Sleepster): Treated as byte arrays
    string_t file_data;
    string_t pixels;
}bitmap_t;

typedef struct texture2D
{
    bitmap_t         bitmap;
    vulkan_texture_t texture_data;
}texture2D_t;

typedef struct shader 
{
    u32                   ID;
    vulkan_shader_data_t *shader_data;
}shader_t;

typedef struct asset_slot 
{
    asset_type_t           type;
    asset_slot_load_status load_status;
    
    string_t               name;
    file_t                 owner_asset_file;

    // NOTE(Sleepster): Should only be modified using atomic_* functions 
    volatile u32           ref_counter;
    union 
    {
        texture2D_t texture;
        shader_t    shader;
    };
}asset_slot_t;

typedef struct asset_manager asset_manager_t;
typedef struct asset_catalog
{
    u32               ID;
    asset_type_t      catalog_type;
    hash_table_t      asset_lookup;

    asset_manager_t  *asset_manager;
}asset_catalog_t;

typedef struct asset_file_header asset_file_header_t;
typedef struct asset_file_table_of_contents asset_file_table_of_contents_t;
typedef struct asset_file_package_entry asset_file_package_entry_t;
typedef struct asset_manager_asset_file_data
{
    bool8                          is_initialized;
    asset_slot_load_status         load_status;
    file_t                         file_info;

    // NOTE(Sleepster): Everything lives and dies with this. 
    zone_allocator_t              *file_allocator;
    string_t                       raw_file_data;

    asset_file_package_entry_t    *package_entries;

    asset_file_header_t            *header_data;
    asset_file_table_of_contents_t *table_of_contents;
    u64                             package_entries_offset;
}asset_manager_asset_file_data_t;

typedef struct asset_manager
{
    bool8                           is_initialized;
    memory_arena_t                  manager_arena;

    // TODO(Sleepster): Hash table for hashing asset filenames with thier associated asset file
    // Ex: "player.png" -> "/run_tree/res/main_asset_file.wad"
    // or even beter "player.png" -> index 0 of the asset_file array
    asset_manager_asset_file_data_t asset_files[ASSET_MANAGER_MAX_ASSET_FILES];
    u32                             loaded_file_count;

    asset_catalog                   asset_catalogs[ASSET_MANAAGER_CATALOG_COUNT];
    asset_catalog_t                *texture_catalog;
    asset_catalog_t                *shader_catalog;
}asset_manager_t;

void  s_asset_manager_init(asset_manager_t *asset_manager);
bool8 s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath);

#endif // S_ASSET_MANAGER_H

