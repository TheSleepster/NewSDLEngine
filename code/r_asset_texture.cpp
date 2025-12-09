/* ========================================================================
   $File: r_asset_texture.cpp $
   $Date: December 09 2025 02:32 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#if !defined(STB_IMAGE_IMPLEMENTATION)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#endif
#include <stb/stb_image.h>

#include <s_asset_manager.h>
#include <r_asset_texture.h>

bitmap_t
s_asset_bitmap_create(zone_allocator_t *zone, s32 width, s32 height, bitmap_format_t format)
{
    bitmap_t result   = {};
    result.width      = width;
    result.height     = height;
    result.channels   = format;
    result.stride     = 8 * format;
    result.format     = format;
    result.data.count = width * height * format;
    result.data.data  = c_za_alloc(zone, result.data.count, ZA_TAG_TEXTURE);

    return(result);
}

texture_view_t
s_asset_texture_view_generate(asset_manager_t *asset_manager, asset_slot_t *valid_slot, texture2D_t *texture_data)
{
    texture_view_t new_view = {}; 
    new_view.is_valid       = true;
    new_view.GPU_textureID  = 0; 
    new_view.uv_min         = &texture_data->uv_min;
    new_view.uv_max         = &texture_data->uv_max;

    return(new_view);
}

texture2D_t
s_asset_texture_create(asset_manager_t  *asset_manager,
                       zone_allocator_t *zone,
                       s32               width,
                       s32               height,
                       bitmap_format_t   format,
                       bool8             has_AA,
                       filter_type_t     filtering)
{
    texture2D_t result = {};
    result.bitmap      = s_asset_bitmap_create(zone, width, height, format);
    result.uv_min      = vec2(0.0, 0.0);
    result.uv_min      = vec2((float32)width, (float32)height);
    result.has_AA      = has_AA;
    result.filter_type = filtering;

    return(result);
}

texture2D_t
s_asset_texture_and_view_create(asset_manager_t  *asset_manager,
                                zone_allocator_t *zone,
                                s32               width,
                                s32               height,
                                bitmap_format_t   format,
                                bool8             has_AA,
                                filter_type_t     filtering)
{
    texture2D_t result = s_asset_texture_create(asset_manager, zone, width, height, format, has_AA, filtering);
    result.view        = s_asset_texture_view_generate(asset_manager, null, &result);

    return(result);
}

// TODO(Sleepster): Make this actually work 
void
s_asset_texture_load_data(asset_manager_t *asset_manager, asset_handle_t *handle)
{
    
    
    Assert(handle->type == AT_BITMAP);
    asset_slot_t *slot_data = handle->asset_slot;
    
    if(slot_data->slot_state == ASS_UNLOADED || slot_data->slot_state == ASS_RELOADING)
    {
#if 0
        s_asset_load_data_from_asset_file_or_path(asset_manager,
                                                  &handle->asset_slot->texture.bitmap.data, 
                                                  asset_manager->texture_catalog.texture_allocator,
                                                  slot_data,
                                                  ZA_TAG_CACHE,
                                                  slot_data->slot_state == ASS_RELOADING ? true : false);
        at_atlas_handler_update_entry(asset_manager, &asset_manager->texture_catalog.primary_handler, *handle);
#endif
    }

    // TODO(Sleepster): As of right now, this condition will never be met if you 
    // call this function a single time. It will just stop here, never actually loading the data
    // and adding it to the atlas handler's array. Fix this IMMEDIATELY.
    if(c_string_is_valid(slot_data->texture.bitmap.data) &&
       slot_data->slot_state == ASS_LOADED)
    {
        stbi_set_flip_vertically_on_load(1);
        u8 *data = stbi_load_from_memory(slot_data->texture.bitmap.data.data,
                                         slot_data->texture.bitmap.data.count,
                                        &slot_data->texture.bitmap.width,
                                        &slot_data->texture.bitmap.height,
                                        &slot_data->texture.bitmap.channels,
                                         BMF_RGBA32);

        s32 data_length = strlen((char *)data);
        slot_data->texture.bitmap.decompressed_data.data  = data;
        slot_data->texture.bitmap.decompressed_data.count = data_length;
        slot_data->texture.bitmap.format = BMF_RGBA32;
        slot_data->texture.bitmap.stride = 32;
        if(!handle->texture.is_in_atlas)
        {
            //at_atlas_handler_add_texture(asset_manager, &asset_manager->texture_catalog.primary_handler, *handle);
        }
        //r_texture_make_gpu(&slot_data->texture, false, TAAFT_NEAREST);
    }
    else
    {
        // TODO(Sleepster): Set the default texture stuff 
    }
}

asset_handle_t
s_asset_texture_get(asset_manager_t *asset_manager, string_t asset_key)
{
    asset_handle_t result = {};

    asset_slot_t *valid_slot = (asset_slot_t *)c_hash_get_value(&asset_manager->texture_catalog.texture_hash, asset_key);
    if(valid_slot)
    {
        result.is_valid               = true;
        result.type                   = AT_BITMAP;
        result.asset_slot             = valid_slot;

        texture2D_t *texture_data = &valid_slot->texture;
        if(texture_data->view.is_valid)
        {
            // valid view
            result.texture = texture_data->view;
        }
        else
        {
            // generate a new view 
            texture_view_t new_view = s_asset_texture_view_generate(asset_manager, valid_slot, texture_data);
            texture_data->view = new_view;
            result.texture     = new_view;
        }

        s_asset_texture_load_data(asset_manager, &result);
    }
    else
    {
        log_warning("Invalid texture key: '%s', could not find a texture with that name in our packaging system...\n", asset_key.data);
        result.is_valid = false;
        result.texture  = asset_manager->texture_catalog.null_texture;
    }

    return(result);
}

texture2D_t* 
s_asset_texture_get_from_handle(asset_handle_t handle)
{
    texture2D_t *result;
    result = &handle.asset_slot->texture;

    return(result);
}

void
s_asset_texture_destroy_data(asset_manager_t *asset_manager, asset_handle_t handle)
{
    bitmap_t *bitmap_data = &handle.asset_slot->texture.bitmap;

    c_za_DEBUG_validate_block_list(asset_manager->texture_catalog.texture_allocator);
    c_za_free(asset_manager->texture_catalog.texture_allocator, bitmap_data->data.data);

    free(handle.asset_slot->texture.bitmap.decompressed_data.data);
    handle.asset_slot->texture.bitmap.decompressed_data.data = null;
    handle.asset_slot->texture.bitmap.decompressed_data.count = 0;

    handle.asset_slot->texture.bitmap.data.count = 0;
    handle.asset_slot->texture.bitmap.data.data  = null;

    bitmap_data->data.count = 0;
    handle.asset_slot->slot_state = ASS_UNLOADED;
}

void
s_asset_texture_view_destroy(asset_manager_t *asset_manager, asset_handle_t handle)
{
    handle.texture.is_valid = false;
    memset(&handle.texture, 0, sizeof(texture_view_t));
}
