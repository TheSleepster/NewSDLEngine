/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: December 09 2025 01:37 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stb/stb_image.h>

#include <s_asset_manager.h>
#include <c_hash_table.h>
#include <p_platform_data.h>
#include <s_renderer.h>

#include <asset_file_packer/asset_file_packer.h>

#define TEXTURE_ATLAS_SIZE (2048)

void
s_asset_manager_read_asset_file_entries(asset_manager_t                *asset_manager,
                                        string_t                        entry_data,
                                        asset_file_table_of_contents_t *table_of_contents)
{
    byte *first_entry = entry_data.data;
    for(u32 entry_index = 0;
        entry_index < table_of_contents->entry_count;
        ++entry_index)
    {
        asset_package_entry_t entry = {};

        entry.name.count     = *first_entry;
        first_entry         += sizeof(u32);

        entry.name.data      = first_entry;
        first_entry         += entry.name.count + 1;

        entry.filepath.count = *first_entry;
        first_entry         += sizeof(u32);

        entry.filepath.data  = first_entry;
        first_entry         += entry.filepath.count + 1;

        entry.entry_data.count = *((u32*)first_entry);
        first_entry           += sizeof(u32);

        entry.ID      = *((u32*)first_entry);
        first_entry  += sizeof(u32);

        entry.file_ID = *((u32*)first_entry);
        first_entry  += sizeof(u32);

        entry.type    = *((asset_type_t*)first_entry);
        first_entry  += sizeof(asset_type_t);

        entry.data_offset_from_start_of_file = *((u64*)first_entry);
        first_entry  += sizeof(u64);

        asset_slot_t *slot_data    = c_arena_push_struct(&asset_manager->manager_arena, asset_slot_t);
        slot_data->asset_type      = entry.type;
        slot_data->slot_state      = ASS_UNLOADED;
        slot_data->asset_id        = entry.ID;
        slot_data->asset_file_id   = entry.file_ID;
        slot_data->filename        = c_string_make_copy(&asset_manager->manager_arena, entry.name);
        slot_data->system_filepath = c_string_make_copy(&asset_manager->manager_arena, entry.filepath);

        slot_data->asset_file_data_offset = entry.data_offset_from_start_of_file;
        slot_data->asset_file_data_length = entry.entry_data.count;
        switch(slot_data->asset_type)
        {
            case AT_BITMAP:
            {
                c_hash_insert_pair(&asset_manager->texture_catalog.texture_hash, slot_data->filename, slot_data);
            }break;
            case AT_SHADER:
            {
                c_hash_insert_pair(&asset_manager->shader_catalog.shader_hash,  slot_data->filename, slot_data);
            }break;
            case AT_FONT:
            {
                c_hash_insert_pair(&asset_manager->font_catalog.font_hash,      slot_data->filename, slot_data);
            }break;
            case AT_SOUND:
            {
                //c_hash_insert_kv_pair(&asset_manager->sound_catalog.sound_hash,    slot_data->filename, slot_data);
            }break;
            default:
            {
                InvalidCodePath;
            }break;
        }
        log_info("ASSET: '%s' READ FROM FILE...\n", slot_data->filename.data);
    }
}

void 
s_asset_manager_init(asset_manager_t *asset_manager, string_t packed_asset_filepath)
{
    c_threadpool_init(&asset_manager->threadpool);
    asset_manager->manager_arena  = c_arena_create(GB(1));
    asset_manager->task_allocator = c_za_create(MB(10));

    asset_manager->texture_catalog.texture_allocator = c_za_create(MB(500));
    asset_manager->texture_catalog.texture_hash      = c_hash_table_create_ma(&asset_manager->manager_arena, 
                                                                              MANAGER_HASH_TABLE_SIZE, 
                                                                              sizeof(asset_slot_t));

    asset_manager->shader_catalog.shader_allocator = c_za_create(MB(500));
    asset_manager->shader_catalog.shader_hash      = c_hash_table_create_ma(&asset_manager->manager_arena, 
                                                                              MANAGER_HASH_TABLE_SIZE, 
                                                                              sizeof(asset_slot_t));

    asset_manager->font_catalog.font_allocator = c_za_create(MB(500));
    asset_manager->font_catalog.font_hash      = c_hash_table_create_ma(&asset_manager->manager_arena, 
                                                                              MANAGER_HASH_TABLE_SIZE, 
                                                                              sizeof(asset_slot_t));
    stbi_set_flip_vertically_on_load(1);
    FT_Error error = FT_Init_FreeType(&asset_manager->font_catalog.font_lib);
    if(error != 0)
    {
        Expect(false, "Failure to init the freetype lib...\n");
    }

    // NOTE(Sleepster): Read asset file data
    {
        asset_manager->asset_file_handle = c_file_open(packed_asset_filepath, false);
        asset_manager->asset_file_data   = c_file_read(packed_asset_filepath, sizeof(asset_file_header_t), 0);

        asset_file_header_t *header = (asset_file_header_t *)asset_manager->asset_file_data.data;
        Assert(header->magic_value == ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' '));

        string_t table_data = c_file_read(packed_asset_filepath, sizeof(asset_file_table_of_contents_t), header->offset_to_table_of_contents);
        asset_file_table_of_contents_t *table_of_contents = (asset_file_table_of_contents_t *)table_data.data; 
        Assert(table_of_contents->magic_value == ASSET_FILE_MAGIC_VALUE('t', 'o', 'c', 'd'));

        u32 first_entry_offset = header->offset_to_table_of_contents + sizeof(asset_file_table_of_contents_t);
        string_t entry_data    = c_file_read(packed_asset_filepath, READ_TO_END, first_entry_offset);

        s_asset_manager_read_asset_file_entries(asset_manager, entry_data, table_of_contents);
    }

    s_asset_packer_init(asset_manager, &asset_manager->texture_catalog.primary_packer, asset_manager->texture_catalog.texture_allocator);
}

struct asset_load_task_data_t
{
    zone_allocator_t *zone;
    asset_slot_t     *asset_slot;
};

void
c_asset_manager_start_load_task(asset_manager_t *asset_manager, asset_slot_t *asset_slot, zone_allocator_t *zone)
{
    if(sys_mutex_lock(&asset_manager->task_allocator->mutex, true))
    {
        asset_load_task_data_t *load_task = c_za_push_struct(asset_manager->task_allocator, asset_load_task_data_t, ZA_TAG_CACHE);
        load_task->zone          = zone;
        load_task->asset_slot    = asset_slot;
    
        c_threadpool_add_task(&asset_manager->threadpool, load_task, s_asset_manager_async_load_asset_data, TPTP_Low);
        sys_mutex_unlock(&asset_manager->task_allocator->mutex);
    }
}

void
s_asset_manager_async_load_asset_data(void *user_data)
{
    asset_load_task_data_t *task_data = (asset_load_task_data_t *)user_data;

    zone_allocator_t *zone = task_data->zone;
    asset_slot_t     *slot = task_data->asset_slot;

    string_t filepath  = slot->system_filepath;
    if(sys_mutex_lock(&zone->mutex, true))
    {
        switch(slot->asset_type)
        {
            case AT_BITMAP:
            {
                bitmap_t *bitmap = &slot->texture.bitmap;
                bitmap->data = c_file_read_za(zone, filepath, READ_ENTIRE_FILE, 0, ZA_TAG_STATIC);
                bitmap->decompressed_data.data = stbi_load_from_memory(bitmap->data.data,
                                                                       bitmap->data.count,
                                                                       &bitmap->width,
                                                                       &bitmap->height,
                                                                       &bitmap->channels,
                                                                       BMF_RGBA32);
                bitmap->format = (bitmap_format_t)bitmap->channels;
                bitmap->decompressed_data.count = bitmap->format * (bitmap->width * bitmap->height);
                bitmap->stride = 32;
            }break;
            case AT_FONT:
            {
                dynamic_render_font_t *font = &slot->render_font;
                font->loaded_data = c_file_read_za(zone, filepath, READ_ENTIRE_FILE, 0, ZA_TAG_STATIC);
            }break;
            case AT_SOUND:
            {
            }break;
        }
        slot->slot_state = ASS_LOADED;
        sys_mutex_unlock(&zone->mutex);
    }
}

void
s_asset_packer_init(asset_manager_t *asset_manager, atlas_packer_t *packer, zone_allocator_t *zone)
{
    ZeroStruct(*packer);

    packer->textures_to_pack = c_dynarray_create(asset_handle_t);
    packer->texture_atlas.asset_slot = &packer->atlas_slot;
    packer->texture_atlas.asset_slot->texture = s_asset_texture_and_view_create(asset_manager, 
                                                                                zone, 
                                                                                TEXTURE_ATLAS_SIZE, 
                                                                                TEXTURE_ATLAS_SIZE, 
                                                                                BMF_RGBA32, 
                                                                                false, 
                                                                                TAAFT_NEAREST);
    packer->atlas_slot.texture.image     = sg_alloc_image();
    packer->atlas_slot.texture.view.view = sg_alloc_view();
    r_texture_upload(&packer->texture_atlas.asset_slot->texture, true, false, TAAFT_NEAREST);
}

void
s_asset_packer_add_texture(atlas_packer_t *packer, asset_handle_t texture)
{
    Assert(texture.asset_slot->slot_state == ASS_LOADED);
    Assert(texture.type == AT_BITMAP);

    c_dynarray_push(packer->textures_to_pack, texture);
}

// TODO(Sleepster): In this function, when we update the texture's view data and image data
// we're creating a huge leak because we're allocating the textures and views using sokol, but never
// freeing them. So when we reassign them, the original view is just lost to the abyss... 
// This is obviously not great... TOO BAD!
void
s_atlas_packer_pack_textures(asset_manager_t *asset_manager, atlas_packer_t *packer)
{
    bitmap_t *atlas_bitmap = &packer->atlas_slot.texture.bitmap;
    c_dynarray_for(packer->textures_to_pack, texture_index) 
    {
        asset_handle_t *handle = packer->textures_to_pack + texture_index;
        if(handle->texture->is_in_atlas || (handle->asset_slot->slot_state != ASS_LOADED)) 
        {
            continue;
        }

        bitmap_t *bitmap_data = &handle->asset_slot->texture.bitmap;
        if(bitmap_data->decompressed_data.data != null)
        {
            byte *src_data = bitmap_data->decompressed_data.data;
            byte *dst_data = atlas_bitmap->decompressed_data.data;

            u32 bytes_per_pixel = bitmap_data->format; 
            u32 bytes_per_row   = bitmap_data->width * bytes_per_pixel;
            for(s32 row_index = 0; 
                row_index < bitmap_data->height; 
                ++row_index)
            {
                u32 src_offset = row_index * bitmap_data->width * bytes_per_pixel;
                u32 dst_offset = ((packer->atlas_cursor_y + row_index) * TEXTURE_ATLAS_SIZE + packer->atlas_cursor_x) * bytes_per_pixel;

                memcpy(dst_data + dst_offset, src_data + src_offset, bytes_per_row);
            }
            
            if(bitmap_data->height > (s32)packer->largest_height) 
            {
                packer->largest_height = bitmap_data->height;
            }

            if(bitmap_data->width > (s32)packer->largest_width)
            {
                packer->largest_width = bitmap_data->width;
            }

            texture2D_t *texture = &handle->asset_slot->texture;
            texture->uv_min = vec2(packer->atlas_cursor_x, packer->atlas_cursor_y);
            texture->uv_max = vec2(packer->atlas_cursor_x + bitmap_data->width,
                                   packer->atlas_cursor_y + bitmap_data->height);

            texture->view.is_in_atlas =  true;
            texture->view.view        =  packer->texture_atlas.asset_slot->texture.view.view;
            texture->view.uv_min      = &texture->uv_min;
            texture->view.uv_max      = &texture->uv_max;


            packer->atlas_cursor_x += bitmap_data->width;
            packer->atlas_cursor_y += bitmap_data->height;
            if(packer->atlas_cursor_x > TEXTURE_ATLAS_SIZE)
            {
                packer->atlas_cursor_x  = 0;
                packer->atlas_cursor_y += packer->largest_height;

                packer->largest_height  = 0;
                packer->largest_width   = 0;
            }

            if(packer->atlas_cursor_y > TEXTURE_ATLAS_SIZE)
            {
                log_fatal("Texture Atlas has exceeded maximum size...\n");
            }
        }
        else
        {
            log_warning("Texture is not loaded... cannot add to atlas...\n");
        }
    }

    sg_image_data image_data = {
        .mip_levels[0] = {
            .ptr  = atlas_bitmap->decompressed_data.data,
            .size = atlas_bitmap->decompressed_data.count
        },
    };
    sg_update_image(packer->texture_atlas.asset_slot->texture.image, &image_data);
    c_dynarray_clear(packer->textures_to_pack);
}
