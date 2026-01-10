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

#include <stb/stb_image.h>

#include <asset_file_packer/asset_file_packer.h>

#include <r_vulkan.h>

// ===============================
// ========== TEXTURES ===========
// ===============================

bitmap_t
s_asset_bitmap_create(asset_manager_t *asset_manager, 
                      asset_slot_t    *asset_slot, 
                      u32              width,
                      u32              height, 
                      u32              channels,
                      bitmap_format_t  format)
{
    bitmap_t result  = {};
    result.width     = width;
    result.height    = height;
    result.channels  = channels;
    result.format    = format;

    u32   pixel_count = width * height * channels;
    byte *pixel_data  = c_za_push_array(asset_manager->asset_allocator, byte, pixel_count, ZA_TAG_STATIC);

    result.pixels = {
        .data  = pixel_data,
        .count = pixel_count
    };

    return(result);
}

bitmap_t
s_asset_bitmap_init(string_t pixels, s32 width, s32 height, s32 channels, u32 format)
{
    bitmap_t result = {};
    result.width    = width;
    result.height   = height;
    result.channels = channels;
    result.format   = format;
    result.pixels   = pixels;

    return(result);
}

texture2D_t 
s_asset_texture_create(asset_manager_t *asset_manager, asset_slot_t *slot)
{
    texture2D_t result = {};
    string_t asset_data = slot->package_entry->entry_data;

    s32 width;
    s32 height;
    s32 channels;

    byte *pixel_data  = stbi_load_from_memory(asset_data.data, asset_data.count, &width, &height, &channels, 4);
    u32   pixel_count = (u32)(width * height * channels);
    string_t pixels = {
        .data  = pixel_data,
        .count = pixel_count 
    };
    Assert(pixel_data != null);
    Assert(pixel_count > 0);

    result.bitmap = s_asset_bitmap_init(pixels, width, height, channels, BMF_RGBA32);
    return(result);
}

shader_t
s_asset_shader_create(asset_manager_t *asset_manager, asset_slot_t *slot)
{
    shader_t result;
    return(result);
}

void
s_asset_manager_load_asset_data(asset_manager_t *asset_manager, asset_handle_t *handle)
{
    asset_slot_t *slot = handle->slot;
    Assert(slot->slot_state == ASLS_Unloaded || slot->slot_state == ASLS_ShouldReload);
    slot->package_entry->entry_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                                               slot->package_entry->entry_data.count,
                                                               slot->package_entry->data_offset_from_start_of_file, 
                                                               null, 
                                                               asset_manager->asset_allocator, 
                                                               ZA_TAG_STATIC);
    Assert(slot->package_entry->entry_data.data != null);
    switch(slot->type)
    {
        case AT_Bitmap:
        {
            slot->texture = s_asset_texture_create(asset_manager, slot);
        }break;
        case AT_Shader:
        {
            //slot->shader = s_asset_shader_create(asset_manager, slot);
            log_warning("Not loading shader... not currently supported...\n");
        }break;
        case AT_Font:
        {
            log_warning("Not loading font... not currently supported...\n");
        }break;
        case AT_Sound:
        {
            log_warning("Not loading sound... not currently supported...\n");
        }break;
    }
    slot->slot_state = ASLS_Loaded;
}

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(asset_manager_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

// ===============================
// ========== ASSET MANAGER ======
// ===============================

// TODO(Sleepster): Generate default assets 
void
s_asset_manager_init(asset_manager_t *asset_manager)
{
    Assert(asset_manager->is_initialized == false);
    stbi_set_flip_vertically_on_load(1);

    asset_manager->manager_arena = c_arena_create(MB(100));
    for(u32 catalog_index = 1;
        catalog_index < AT_Count;
        ++catalog_index)
    {
        asset_catalog *catalog = asset_manager->asset_catalogs + catalog_index;
        catalog->asset_manager = asset_manager;
        c_hash_table_init(&catalog->asset_lookup, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_manager->manager_arena, 
                           asset_manager_hash_arena_allocate,
                           null);
        catalog->catalog_type = (asset_type_t)(catalog_index);

        Assert(catalog->catalog_type < AT_Count);
        Assert(catalog->catalog_type > AT_Invalid);
    }
    c_threadpool_init(&asset_manager->worker_pool);
    c_hash_table_init(&asset_manager->asset_name_to_file, 
                       ASSET_CATALOG_MAX_LOOKUPS, 
                      &asset_manager->manager_arena, 
                       asset_manager_hash_arena_allocate,
                       null);
    // NOTE(Sleepster): Initializing all entries to -1 
    memset(asset_manager->asset_name_to_file.data, -1, sizeof(s32) * ASSET_CATALOG_MAX_LOOKUPS);
        
    asset_manager->is_initialized = true;
}

internal_api void
s_asset_manager_parse_asset_package_file(asset_manager_t *asset_manager, asset_manager_asset_file_data_t *file_data)
{
    u64 first_entry_offset = file_data->header_data->offset_to_table_of_contents + sizeof(asset_file_table_of_contents_t);
    string_t package_entries = c_file_read_from_offset(&file_data->file_info, 
                                                        file_data->file_info.file_size - first_entry_offset,
                                                        first_entry_offset,
                                                       &file_data->init_arena);
    for(u32 entry_index = 0;
        entry_index < file_data->package_entry_count;
        ++entry_index)
    {
        asset_file_package_entry_t package_entry = {};

        package_entry.name.count = *package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.name.data = package_entries.data;
        package_entries.data += package_entry.name.count + 1;

        package_entry.filepath.count = *package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.filepath.data  = package_entries.data;
        package_entries.data += package_entry.filepath.count + 1;

        package_entry.entry_data.count = *package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.ID = *package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.file_ID = *package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.type = (asset_type_t)*package_entries.data;
        package_entries.data += sizeof(u32);

        package_entry.data_offset_from_start_of_file = *package_entries.data;
        package_entries.data += sizeof(u64);

        file_data->package_entries[entry_index] = package_entry;
    }
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

    asset_file->init_arena      = c_arena_create(MB(500));
    asset_file->is_initialized  = true;
    asset_file->ID              = asset_manager->loaded_file_count;

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
                           asset_manager_hash_arena_allocate,
                           null);
        asset_file->package_entries = c_arena_push_array(&asset_file->init_arena, 
                                                          asset_file_package_entry_t, 
                                                          asset_file->package_entry_count);
        //NOTE(Sleepster): Build the asset file's package database 
        Assert(asset_file->package_entry_count > 0);
        Assert(asset_file->package_entries);
        
        s_asset_manager_parse_asset_package_file(asset_manager, asset_file);

        for(u32 entry_index = 0;
            entry_index < asset_file->package_entry_count;
            ++entry_index)
        {
            asset_file_package_entry_t *entry = asset_file->package_entries + entry_index;
            Assert(entry->type != AT_Invalid);

            c_hash_table_insert_pair(&asset_file->entry_hash, entry->name, (s32)entry_index);
            c_hash_table_insert_pair(&asset_manager->asset_name_to_file, entry->name, (s32)asset_file->ID);
            u64 hash_value = c_hash_table_value_from_key(entry->name.data, entry->name.count, asset_manager->asset_name_to_file.header.max_entries);
            log_debug("Inserting asset with name: '%s' with a name length of: '%d' into the name_to_file hash with file_index: '%d' hash_value: '%llu'...\n", C_STR(entry->name), entry->name.count, asset_file->ID, hash_value);

            asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->type;
            asset_slot_t    *slot    = c_hash_table_get_value_ptr(&catalog->asset_lookup, entry->name);
            Assert(slot);
            Assert(entry->type == catalog->catalog_type);

            ZeroStruct(*slot);
            slot->slot_state       = ASLS_Unloaded;
            slot->type             = entry->type;
            slot->name             = entry->name;
            slot->package_entry    = entry;
            slot->ref_counter      = 0;
            slot->owner_asset_file = asset_file->file_info;
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

internal_api asset_slot_t *
s_asset_manager_get_asset_slot(asset_catalog_t *catalog, string_t name)
{
    asset_slot *result = null;
    result = c_hash_table_get_value_ptr(&catalog->asset_lookup, name);
    if(result == null)
    {
        log_error("Failure to fetch asset '%s' from this catalog...\n", C_STR(name));
    }
    else
    {
        AtomicIncrement32(&result->ref_counter);
    }

    return(result);
}

asset_handle_t
s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name)
{
    asset_handle_t result;

    u64 hash_value = c_hash_table_value_from_key(name.data, name.count, asset_manager->asset_name_to_file.header.max_entries);
    log_info("hash index for: '%s' is '%llu'...\n", C_STR(name), hash_value);
    s32 file_index = c_hash_table_get_value(&asset_manager->asset_name_to_file, name);
    if(file_index != -1)
    {
        asset_manager_asset_file_data_t *asset_file = asset_manager->asset_files + file_index;
        s32 asset_entry_index = c_hash_table_get_value(&asset_file->entry_hash, name);
        // NOTE(Sleepster): This just SHOULD NOT be possible...
        Assert(asset_entry_index != -1);

        asset_file_package_entry_t *entry = asset_file->package_entries + asset_entry_index;
        Assert(entry->type != AT_Invalid && entry->type != AT_Count);

        asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->type;
        Assert(catalog);

        result.type = entry->type;
        result.slot = s_asset_manager_get_asset_slot(catalog, name);
        result.owner_asset_file_index         = file_index;
        result.is_valid                       = true;

        if(result.slot->slot_state == ASLS_Unloaded)
        {
            s_asset_manager_load_asset_data(asset_manager, &result);
        }

        Assert(result.slot->slot_state != ASLS_Invalid);
    }
    else
    {
        log_error("Asset by name of: '%s' cannot be found in the asset file database...\n",
                  C_STR(name));
    }

    return(result);
}
