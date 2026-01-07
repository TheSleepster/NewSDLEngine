#if !defined(S_ASSET_MANAGER_H)
/* ========================================================================
   $File: s_asset_manager.h $
   $Date: December 09 2025 01:33 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ASSET_MANAGER_H
#include <ft2build.h>
#include FT_FREETYPE_H

#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_string.h>
#include <c_hash_table.h>
#include <c_zone_allocator.h>
#include <c_threadpool.h>

#include <r_asset_texture.h>
#include <r_asset_dynamic_render_font.h>

#define MANAGER_HASH_TABLE_SIZE 1024
#define MAX_TEXTURE_VIEWS       1024
#define TEXTURE_ATLAS_SIZE      (2048)

struct render_state_t;
typedef struct asset_file_table_of_contents asset_file_table_of_contents_t;

#if 0
typedef struct render_material
{
    u32           material_ID;
    u32           texture_ID;
    u32           effect_mask;
    float32       emmisive_strength;

    GPU_shader_t *shader;
}render_material_t;
#endif

typedef enum asset_type
{
    AT_NONE,
    AT_BITMAP,
    AT_SHADER,
    AT_FONT,
    AT_SOUND,
    AT_ANIMATION,
    AT_COUNT
}asset_type_t;

// NOTE(Sleepster): Unfortunate naming 
typedef enum asset_slot_state
{
    ASS_INVALID,

    ASS_UNLOADED,
    ASS_QUEUED,
    ASS_LOADED,

    ASS_RELOADING,
    ASS_UNLOADING,

    ASS_COUNT
}asset_slot_state_t;

typedef struct asset_slot
{
    asset_slot_state_t slot_state;
    asset_type_t       asset_type;

    string_t           filename;
    string_t           system_filepath;
    
    s32                asset_id;
    s32                asset_file_id;
    u64                last_used_ts;

    u64                asset_file_data_offset;
    u64                asset_file_data_length;
    union
    {
        texture2D_t           texture;
        //loaded_sound_t        loaded_sound;
        dynamic_render_font_t render_font;
    };
}asset_slot_t;

/* NOTE(Sleepster): New idea: asset_handles. These asset_handles will be
 * used to look into an asset and keep track of the state related to
 * that asset while still allowing the asset's state to remain
 * modifiable. The idea is that we no longer hand out pointers to the
 * texture like before, we instead create "handles" that contains the
 * data needed to actually use the texture but in a read only
 * way. Such that if important data like the texture's UVs change due
 * to the engine merging certain textures into an atlas, we won't have
 * to update the handles. The handles will already contain the right
 * data without updating.
 */

typedef struct asset_handle
{
    bool8         is_valid;
    asset_type_t  type;

    asset_slot_t *asset_slot;
    union
    {
        texture_view_t        *texture;
        dynamic_render_font_t *font;
        //loaded_sound_t      *sound;
        //render_shader_t     *shader;
    };
}asset_handle_t;

typedef struct playing_sound
{
    asset_handle_t        sound_handle;
    asset_handle_t        next_sound_handle;

    float32               play_cursor;
    vec2_t                current_playing_volume;
    vec2_t                target_playing_volume;
    vec2_t                d_volumet;
    float32               pitch_shift; 

    bool8                 is_paused;
    struct playing_sound *next;
}playing_sound_t;

typedef struct atlas_packer
{
    asset_handle_t           texture_atlas;
    asset_slot_t             atlas_slot;
    DynArray(asset_handle_t) textures_to_pack;

    u32                      atlas_cursor_x;
    u32                      atlas_cursor_y;
    u32                      largest_width;
    u32                      largest_height;
}atlas_packer_t;

typedef struct asset_manager
{
    threadpool_t      threadpool;
    memory_arena_t    manager_arena;

    zone_allocator_t *task_allocator;

    string_t          asset_file_data;
    file_t            asset_file_handle;

    struct 
    {
        zone_allocator_t        *texture_allocator;
        u32                      global_view_ID;

        hash_table_t             texture_hash;
        texture_view_t           null_texture;

        atlas_packer_t           primary_packer;
    }texture_catalog;
    struct 
    {
        zone_allocator_t *shader_allocator;

        hash_table_t      shader_hash;
        texture_view_t    null_shader;
    }shader_catalog;
    struct 
    {
        FT_Library        font_lib;
        zone_allocator_t *font_allocator;
    
        hash_table_t      font_hash;
        texture_view_t    null_font;
    }font_catalog;
#if 0 
    struct
    {
        zone_allocator_t *sound_allocator;

        hash_table_t      sound_hash;
        texture_view_t    null_sound;
    }sound_catalog;
#endif
}asset_manager_t;

// void s_asset_manager_init(asset_manager_t *asset_manager, string_t packed_asset_filepath);
// void s_asset_manager_init(asset_manager_t *asset_manager, string_t packed_asset_filepath);
// void c_asset_manager_start_load_task(asset_manager_t *asset_manager, asset_slot_t *asset_slot, zone_allocator_t *zone);
// void s_asset_manager_async_load_asset_data(void *user_data);
//
// void s_asset_packer_init(asset_manager_t *asset_manager, atlas_packer_t *packer, zone_allocator_t *zone);
// void s_asset_packer_add_texture(atlas_packer_t *packer, asset_handle_t texture);
// void s_asset_packer_init(asset_manager_t *asset_manager, atlas_packer_t *packer, zone_allocator_t *zone);
// void s_atlas_packer_pack_textures(asset_manager_t *asset_manager, atlas_packer_t *packer);

#endif // S_ASSET_MANAGER_H

