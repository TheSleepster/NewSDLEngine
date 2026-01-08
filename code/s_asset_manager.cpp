/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: January 06 2026 11:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_hash_table.h>

#include <asset_file_packer/asset_file_packer.h>

#include <r_vulkan.h>

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(asset_manager_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

// TODO(Sleepster): Generate default assets 
void
s_asset_manager_init(asset_manager_t *asset_manager)
{
    Assert(asset_manager->is_initialized == false);

    asset_manager->manager_arena = c_arena_create(MB(100));
    for(u32 catalog_index = 0;
        catalog_index < ASSET_MANAAGER_CATALOG_COUNT;
        ++catalog_index)
    {
        asset_catalog *catalog = asset_manager->asset_catalogs + catalog_index;
        catalog->asset_manager = asset_manager;
        c_hash_table_init(&catalog->asset_lookup, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_manager->manager_arena, 
                           asset_manager_hash_arena_allocate);
    }

    // NOTE(Sleepster): Hard coded because it won't really matter anyway since there will be like only 4 of them in the future; 
    asset_manager->asset_catalogs[0].catalog_type = AT_Bitmap;
    asset_manager->asset_catalogs[1].catalog_type = AT_Shader;

    asset_manager->is_initialized = true;
}

bool8
s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath)
{
    Assert(asset_manager->is_initialized);
    Assert(asset_manager->loaded_file_count + 1 <= ASSET_MANAGER_MAX_ASSET_FILES);

    bool8 result = true;
    asset_manager_asset_file_data_t *asset_file = null;
    for(u32 file_index = 0;
        file_index < ASSET_MANAGER_MAX_ASSET_FILES;
        ++file_index)
    {
        asset_manager_asset_file_data_t *found = asset_manager->asset_files + file_index;
        if(found->is_initialized == false)
        {
            asset_file = found;
            break;
        }
    }
    Assert(asset_file);
    Assert(!asset_file->is_initialized);

    asset_file->asset_allocator = c_za_create(GB(1.0f));
    asset_file->init_arena      = c_arena_create(MB(500));
    asset_file->is_initialized  = true;

    asset_file->file_info = c_file_open(filepath, false);
    if(asset_file->file_info.handle != INVALID_FILE_HANDLE)
    {
        asset_file_header_t            *header            = null;
        asset_file_table_of_contents_t *table_of_contents = null;

        //NOTE(Sleepster): Get the asset file header 
        string_t header_data = c_file_read(&asset_file->file_info, 
                                            sizeof(asset_file_header_t), 
                                           &asset_file->init_arena);

        header = (asset_file_header_t*)header_data.data;
        Expect(header->magic_value == ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' '), 
               "Asset file: '%s' does not have the value magic number 'WAD '...\n", C_STR(asset_file->file_info.file_name));
        
        //NOTE(Sleepster): Get the asset file's TOC using the offset given by the header 
        string_t table_of_contents_data = c_file_read_from_offset(&asset_file->file_info, 
                                                                  sizeof(asset_file_table_of_contents_t), 
                                                                  header->offset_to_table_of_contents, 
                                                                  &asset_file->init_arena);
        table_of_contents = (asset_file_table_of_contents_t*)table_of_contents_data.data;
        Expect(table_of_contents->magic_value == ASSET_FILE_MAGIC_VALUE('t', 'o', 'c', 'd'), 
               "Asset file: '%s' does not have the valid TOC magic value of 'tocd'...\n", C_STR(asset_file->file_info.file_name));

        Expect(table_of_contents->entry_count > 0, "Asset file: '%s' has an entry count of zero...\n", C_STR(asset_file->file_info.file_name));
        asset_file->package_entries_offset = header->offset_to_table_of_contents + sizeof(asset_file_table_of_contents_t);
        asset_file->package_entry_count    = table_of_contents->entry_count;
        asset_file->header_data            = header;
        asset_file->table_of_contents      = table_of_contents;

        c_hash_table_init(&asset_file->entry_hash, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_file->init_arena, 
                           asset_manager_hash_arena_allocate);

        asset_file->package_entries = c_arena_push_array(&asset_file->init_arena, 
                                                          asset_file_package_entry_t, 
                                                          asset_file->package_entry_count);
        //NOTE(Sleepster): Build the asset file's package database 
        Assert(asset_file->package_entry_count > 0);
        Assert(asset_file->package_entries);

        for(u32 entry_index = 0;
            entry_index < asset_file->package_entry_count;
            ++entry_index)
        {
            asset_file_package_entry_t *entry = asset_file->package_entries + entry_index;
            c_hash_table_insert_pair(&asset_file->entry_hash, entry->name, (s32)entry_index);
        }
    }
    else
    {
        result = false;
    }

    return(result);
}
