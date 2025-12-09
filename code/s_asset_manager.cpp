/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: December 09 2025 01:37 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_asset_manager.h>

void 
s_asset_manager_init(asset_manager_t *asset_manager)
{
    c_threadpool_init(&asset_manager->threadpool);
    asset_manager->manager_arena = c_arena_create(GB(1));

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

void 
s_asset_load_data_from_asset_file_or_path(asset_manager_t    *asset_manager,
                                          string_t           *out_data,
                                          zone_allocator_t   *zone,
                                          asset_slot_t       *slot_data,
                                          za_allocation_tag_t tag,
                                          bool8               is_reloading = false)
{
}
