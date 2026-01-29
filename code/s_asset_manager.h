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
#include <c_dynarray.h>

#define ASSET_CATALOG_MAX_LOOKUPS         (4099)
#define ASSET_MANAGER_MAX_TEXTURE_ATLASES (128)
#define ASSET_MANAGER_MAX_ASSET_FILES     (32)

typedef struct vulkan_shader_data vulkan_shader_data_t;
typedef struct vulkan_texture     vulkan_texture_t;
typedef struct asset_manager      asset_manager_t;
typedef struct asset_slot         asset_slot_t;
typedef struct subtexture_data    subtexture_data_t;
typedef struct texture_atlas      texture_atlas_t;

typedef struct jfd_package_entry  jfd_package_entry_t;
typedef struct jfd_file_header    jfd_file_header_t;

typedef enum asset_type
{
    AT_Invalid,
    AT_Bitmap,
    AT_Shader,
    AT_Font,
    AT_Sound,
    AT_Material,
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

#include <r_vulkan_types.h>

/* NOTE(Sleepster): 
 * The Asset handle is very simple, it is simply a means to only do the expensive hash lookups once, 
 * and then have a means to update the asset's state in a way that is much less expensive than the lookups
 * themselves. All actions relating to the asset performed through the handle. Actions like these are such as:
 * - Allocating from the asset file
 * - Freeing the asset's data
 */
typedef struct asset_handle
{
    bool32             is_valid;
    asset_type_t       type;
    s32                owner_asset_file_index;

    subtexture_data_t *subtexture_data;
    asset_slot_t      *slot;
}asset_handle_t;


/*===========================================
  ================= TEXTURES ================
  ===========================================*/

typedef struct bitmap
{
    u32      width;
    u32      height;
    u32      channels;
    u32      format;

    // NOTE(Sleepster): Treated as a byte array
    string_t pixels;
}bitmap_t;

typedef struct texture2D
{
    bitmap_t          bitmap;
    vulkan_texture_t  gpu_data;
}texture2D_t;

typedef struct subtexture_data
{
    vec2_t           uv_min;
    vec2_t           uv_max;

    vec2_t           offset;
    vec2_t           size;

    u32              atlas_subtexture_index;
    texture_atlas_t *atlas;
}subtexture_data_t;

typedef struct texture_atlas
{
    texture2D_t                   texture;
    bitmap_t                     *bitmap_data;

    u32                           ID;
    u32                           merge_counter;
    DynArray_t(asset_handle_t*)   textures_to_merge;

    // TODO(Sleepster): Technically we need "add" to this array, just pull from it. What do we do about this? 
    //                  Guess it's not a problem for now.
    DynArray_t(subtexture_data_t) packed_subtextures;
    u32                           packed_subtexture_count;
    bool32                        is_valid;

    u32                           atlas_cursor_x;
    u32                           atlas_cursor_y;
    u32                           tallest_y;
    u32                           atlas_size;
}texture_atlas_t;

/*===========================================
  ================== SHADERS ================
  ===========================================*/

typedef struct shader 
{
    u32                  ID;
    vulkan_shader_data_t shader_data;

    // NOTE(Sleepster): Storing some basic things here. 
    vulkan_shader_uniform_data_t *camera_uniform;
    vulkan_shader_uniform_data_t *texture_uniform;
}shader_t;

/* MATERIAL CONFIG:
 * - Shader name
 * - ID
 * - Name of the material
 * - Default pipeline state (blend mode, blend enabled, depth mode, depth enabled, etc.)
 */

// TODO(Sleepster): Effect flags and such for special rendering layers. 
typedef struct material
{
    u64                     ID;
    string_t                name;
    
    string_t                shader_binary_name;
    shader_t               *shader;
    texture2D_t            *textures[MAX_RENDER_GROUP_BOUND_TEXTURES];

    u32                     renderer_effect_flags;
    render_pipeline_state_t pipeline_state;
}material_t;

/*===========================================
  ============= ASSET FILE DATA =============
  ===========================================*/

typedef struct asset_slot 
{
    asset_slot_load_status_t slot_state;
    asset_type_t             type;
    
    string_t                 name;
    file_t                   owner_asset_file;
    jfd_package_entry_t     *package_entry;

    // NOTE(Sleepster): Should only be modified using atomic_* functions 
    volatile u32             package_generation;
    volatile u32             ref_counter;
    union 
    {
        texture2D_t texture;
        shader_t    shader;
        material_t  material;
    };
}asset_slot_t;

// NOTE(Sleepster): Everything file related lives and dies with this arena. 
typedef struct asset_manager_asset_file_data
{
    bool8                    is_initialized;
    u32                      ID;
    memory_arena_t           init_arena;

    asset_slot_load_status_t load_status;
    file_t                   file_info;
 
    string_t                 raw_file_data;

    jfd_package_entry_t     *package_entries;
    u32                      package_entry_count;
    HashTable_t(s32)         entry_hash;

    jfd_file_header_t       *header_data;
}asset_manager_asset_file_data_t;

/*===========================================
  =========== ASSET MANAGER DATA ============
  ===========================================*/

typedef struct asset_catalog
{
    u32                       ID;
    asset_type_t              catalog_type;
    asset_manager_t          *asset_manager;

    // TODO(Sleepster): Should this be a * to asset_slots?
    HashTable_t(asset_slot_t) asset_lookup;
}asset_catalog_t;

// TODO(Sleepster): thread safety
typedef struct texture_atlas_registry
{
    texture_atlas_t atlases[ASSET_MANAGER_MAX_TEXTURE_ATLASES];
    u32             current_atlas_count;
}texture_atlas_registry_t;

typedef struct asset_manager
{
    bool8                           is_initialized;
    memory_arena_t                  manager_arena;

    // TODO(Sleepster): Hash table for hashing asset filenames with thier associated asset file
    // Ex: "player.png" -> "/run_tree/res/main_asset_file.wad"
    // or even beter "player.png" -> index 0 of the asset_file array
    asset_manager_asset_file_data_t asset_files[ASSET_MANAGER_MAX_ASSET_FILES];
    HashTable_t(s32)                asset_name_to_file;
    u32                             loaded_file_count;

    asset_slot_t                   *asset_load_queue[256];
    asset_slot_t                   *asset_unload_queue[256];

    texture_atlas_registry_t        atlas_registry;

    // TODO(Sleepster): Replace this zone allocator thing. Not great for more than one thread... 
    zone_allocator_t               *asset_allocator;
    asset_catalog_t                 asset_catalogs[AT_Count];
    asset_catalog_t                *texture_catalog;
    asset_catalog_t                *shader_catalog;

    vulkan_render_context_t        *render_context;
}asset_manager_t;

void  s_asset_manager_init(asset_manager_t *asset_manager);
bool8 s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath);
asset_handle_t s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name);


texture_atlas_t* s_texture_atlas_create(asset_manager_t *asset_manager, u32 size, u32 channel_count, u32 format, u32 initial_subtexture_count);
void s_texture_atlas_add_texture(texture_atlas_t *atlas, asset_handle_t *texture_handle);
void s_texture_atlas_pack_added_textures(vulkan_render_context_t *render_context, texture_atlas_t *atlas);

#endif // S_ASSET_MANAGER_H

