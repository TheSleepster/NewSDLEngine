/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: December 09 2025 01:37 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_asset_manager.h>
#include <stb/stb_image.h>

#include <p_platform_data.h>

void 
s_asset_manager_init(asset_manager_t *asset_manager)
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

    zone_allocator_t *zone          = task_data->zone;
    asset_slot_t     *slot          = task_data->asset_slot;

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
                bitmap->decompressed_data.count = strlen((char *)bitmap->decompressed_data.data);
                bitmap->format = BMF_RGBA32;
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
