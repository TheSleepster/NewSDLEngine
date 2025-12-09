#if !defined(R_ASSET_TEXTURE_H)
/* ========================================================================
   $File: r_asset_texture.h $
   $Date: December 09 2025 01:55 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_ASSET_TEXTURE_H
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_file_api.h>

#include <c_math.h>

typedef struct asset_slot    asset_slot_t;
typedef struct asset_handle  asset_handle_t;
typedef struct asset_manager asset_manager_t;

typedef enum bitmap_format
{
    BMF_INVALID = 0,
    BMF_R8      = 1,
    BMF_B8      = 1,
    BMF_G8      = 1,
    BMF_RGB24   = 3,
    BMF_BGR24   = 3,
    BMF_RGBA32  = 4,
    BMF_ABGR32  = 4,
    BMF_COUNT
}bitmap_format_t;

typedef struct bitmap
{
    bitmap_format_t format;
    s32             channels;
    s32             width;
    s32             height;
    s32             stride;
    
    // NOTE(Sleepster): byte arrays essentially. 
    string_t        data;
    string_t        decompressed_data;
}bitmap_t;

typedef enum filter_type
{
    TAAFT_INVALID,
    TAAFT_LINEAR,
    TAAFT_NEAREST,
    TAAFT_COUNT
}filter_type_t;

typedef struct texture_view
{
    bool8         is_valid;
    bool8         is_in_atlas;
    
    u32           viewID;
    u32           GPU_textureID;

    // NOTE(Sleepster): Pointers so that if the uv-location of the texture ever 
    // changes, the changes are immediately reflected for each view created for this
    // texture.
    vec2_t       *uv_min;
    vec2_t       *uv_max;
}texture_view_t;

typedef struct texture2D
{
    texture_view_t  view;
    bitmap_t        bitmap;

    vec2_t          uv_min;
    vec2_t          uv_max;

    bool8           has_AA;
    filter_type_t   filter_type;
    u32             GPU_textureID;
}texture2D_t;

/*=============================================
  ================ TEXTURE API ================
  =============================================*/
texture_view_t  s_asset_texture_view_generate(asset_manager_t *asset_manager, asset_slot_t *valid_slot, texture2D_t *texture_data);
texture2D_t     s_asset_texture_create(asset_manager_t  *asset_manager, zone_allocator_t *zone, s32 width, s32 height, bitmap_format_t format, bool8 has_AA, filter_type_t filtering);
texture2D_t     s_asset_texture_and_view_create(asset_manager_t  *asset_manager, zone_allocator_t *zone, s32 width, s32 height, bitmap_format_t format, bool8 has_AA, filter_type_t filtering);
void            s_asset_texture_load_data(asset_manager_t *asset_manager, asset_handle_t *handle);
asset_handle_t  s_asset_texture_get(asset_manager_t *asset_manager, string_t asset_key);
void            s_asset_texture_destroy_data(asset_manager_t *asset_manager, asset_handle_t handle);
void            s_asset_texture_view_destroy(asset_manager_t *asset_manager, asset_handle_t handle);

texture2D_t    *s_asset_texture_get_from_handle(asset_handle_t handle);

#endif // R_ASSET_TEXTURE_H

