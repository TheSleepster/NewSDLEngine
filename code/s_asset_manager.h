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
#include <c_threadpool.h>

#define ASSET_CATALOG_MAX_LOOKUPS     (4099)
#define ASSET_MANAGER_MAX_ASSET_FILES (32)

typedef struct asset_file_header asset_file_header_t;
typedef struct asset_file_table_of_contents asset_file_table_of_contents_t;
typedef struct asset_file_package_entry asset_file_package_entry_t;
typedef struct vulkan_shader_data vulkan_shader_data_t;
typedef struct vulkan_texture vulkan_texture_t;
typedef struct asset_manager asset_manager_t;
typedef struct asset_slot asset_slot_t;

typedef enum asset_type
{
    AT_Invalid,
    AT_Bitmap,
    AT_Shader,
    AT_Font,
    AT_Sound,
    AT_Count
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
}asset_slot_load_status_t;

typedef enum bitmap_format
{
    BMF_Invalid,
    BMF_R8,
    BMF_RGBA32,
    BMF_RGB24,
    BMF_Count,
}bitmap_format_t;

#include <r_vulkan.h>

/* NOTE(Sleepster): 
 * The Asset handle is very simple, it is simply a means to only do the expensive hash lookups once, 
 * and then have a means to update the asset's state in a way that is much less expensive than the lookups
 * themselves. All actions relating to the asset performed through the handle. Actions like these are such as:
 * - Allocating from the asset file
 * - Freeing the asset's data
 */
typedef struct asset_handle
{
    bool32        is_valid;
    asset_type_t  type;
    s32           owner_asset_file_index;

    asset_slot_t *slot;
}asset_handle_t;

typedef struct bitmap
{
    u32      width;
    u32      height;
    u32      channels;
    u32      format;

    // NOTE(Sleepster): Treated as byte arrays
    string_t pixels;
}bitmap_t;

typedef struct texture2D
{
    bitmap_t          bitmap;
    vulkan_texture_t  gpu_data;
    
    u32              current_generation;
}texture2D_t;

typedef struct shader 
{
    u32                   ID;
    vulkan_shader_data_t  shader_data;
}shader_t;

typedef struct asset_slot 
{
    asset_slot_load_status_t    slot_state;
    asset_type_t                type;
    
    string_t                    name;
    string_t                    file_data;
    file_t                      owner_asset_file;
    asset_file_package_entry_t *package_entry;

    // NOTE(Sleepster): Should only be modified using atomic_* functions 
    volatile u32                ref_counter;
    union 
    {
        texture2D_t texture;
        shader_t    shader;
    };
}asset_slot_t;

typedef struct asset_catalog
{
    u32                                 ID;
    asset_type_t                        catalog_type;
    asset_manager_t                    *asset_manager;

    // TODO(Sleepster): Should this be a * to asset_slots?
    HashTable_t(asset_slot_t)           asset_lookup;
}asset_catalog_t;

// NOTE(Sleepster): Everything file related lives and dies with this arena. 
typedef struct asset_manager_asset_file_data
{
    bool8                           is_initialized;
    u32                             ID;
    memory_arena_t                  init_arena;

    asset_slot_load_status_t        load_status;
    file_t                          file_info;
 
    string_t                        raw_file_data;

    asset_file_package_entry_t     *package_entries;
    u32                             package_entry_count;
    HashTable_t(s32)                entry_hash;

    asset_file_header_t            *header_data;
    asset_file_table_of_contents_t *table_of_contents;
    u64                             package_entries_offset;
}asset_manager_asset_file_data_t;

typedef struct asset_manager
{
    bool8                           is_initialized;
    memory_arena_t                  manager_arena;
    threadpool_t                    worker_pool;

    // TODO(Sleepster): Hash table for hashing asset filenames with thier associated asset file
    // Ex: "player.png" -> "/run_tree/res/main_asset_file.wad"
    // or even beter "player.png" -> index 0 of the asset_file array
    asset_manager_asset_file_data_t asset_files[ASSET_MANAGER_MAX_ASSET_FILES];
    HashTable_t(s32)                asset_name_to_file;
    u32                             loaded_file_count;

    asset_slot_t                   *asset_load_queue[256];
    asset_slot_t                   *asset_unload_queue[256];

    // TODO(Sleepster): Replace this zone allocator thing. Not great for more than one thread... 
    zone_allocator_t               *asset_allocator;
    asset_catalog_t                 asset_catalogs[AT_Count];
    asset_catalog_t                *texture_catalog;
    asset_catalog_t                *shader_catalog;
}asset_manager_t;

void  s_asset_manager_init(asset_manager_t *asset_manager);
bool8 s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath);
asset_handle_t s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name);

#endif // S_ASSET_MANAGER_H

