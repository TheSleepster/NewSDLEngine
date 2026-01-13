/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: January 06 2026 11:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stb/stb_image.h>

#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_hash_table.h>
#include <asset_file_packer/jfd_asset_file.h>

#include <r_vulkan_core.h>

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
    string_t asset_data = slot->package_entry->asset_data;

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
s_asset_shader_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    shader_t result;
    result.shader_data = r_vulkan_shader_create(asset_manager->render_context, slot->package_entry->asset_data);
    result.ID          = name_hash;
    return(result);
}

void
s_asset_manager_load_asset_data(asset_manager_t *asset_manager, asset_handle_t *handle, u64 name_hash)
{
    asset_slot_t *slot = handle->slot;
    Assert(slot->slot_state == ASLS_Unloaded || slot->slot_state == ASLS_ShouldReload);
    slot->package_entry->asset_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                                              slot->package_entry->asset_data.count,
                                                              slot->package_entry->data_offset, 
                                                              null, 
                                                              asset_manager->asset_allocator, 
                                                              ZA_TAG_STATIC);
    Assert(slot->package_entry->asset_data.data != null);
    switch(slot->type)
    {
        case AT_Bitmap:
        {
            slot->texture = s_asset_texture_create(asset_manager, slot);
            log_info("Loading texture data for bitmap: '%s'...\n", C_STR(handle->slot->name));
        }break;
        case AT_Shader:
        {
            slot->shader = s_asset_shader_create(asset_manager, slot, name_hash);
            log_info("Loading shader data for: '%s'...\n", C_STR(handle->slot->name));
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
    AtomicIncrement32(&slot->package_generation);
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
    stbi_set_flip_vertically_on_load(0);

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
    c_hash_table_init(&asset_manager->asset_name_to_file, 
                       ASSET_CATALOG_MAX_LOOKUPS, 
                      &asset_manager->manager_arena, 
                       asset_manager_hash_arena_allocate,
                       null);
    // NOTE(Sleepster): Initializing all entries to -1 
    memset(asset_manager->asset_name_to_file.data, -1, sizeof(s32) * ASSET_CATALOG_MAX_LOOKUPS);
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
    
    asset_file->file_info = c_file_open(filepath, false);
    if(asset_file->file_info.handle != INVALID_FILE_HANDLE)
    {
        file_t *file_handle = &asset_file->file_info;

        asset_file->init_arena      = c_arena_create(MB(500));
        asset_file->is_initialized  = true;
        asset_file->ID              = asset_manager->loaded_file_count;

        jfd_file_header_t *header = (jfd_file_header_t*)(c_file_read(file_handle, sizeof(jfd_file_header_t), &asset_file->init_arena).data);
        Assert(header->magic_value == ASSET_FILE_HEADER_MAGIC);
        
        asset_file->package_entries = c_arena_push_array(&asset_file->init_arena, jfd_package_entry_t, header->entry_count);
        c_hash_table_init(&asset_file->entry_hash, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_file->init_arena, 
                           asset_manager_hash_arena_allocate,
                           null);

        u64 current_file_offset = file_handle->current_read_offset;
        for(u32 entry_index = 0;
            entry_index < header->entry_count;
            ++entry_index)
        {
            u64 data_offset = current_file_offset + sizeof(jfd_package_chunk_header_t);
            jfd_package_entry_t *entry = asset_file->package_entries + entry_index;
            entry->entry_header = (jfd_package_chunk_header_t*)(c_file_read_from_offset(file_handle, 
                                                                                        sizeof(jfd_package_chunk_header_t), 
                                                                                        current_file_offset, 
                                                                                       &asset_file->init_arena).data);
            Assert(entry->entry_header->magic_value == ASSET_FILE_CHUNK_MAGIC);
            entry->filename = c_file_read_from_offset(file_handle, 
                                                      entry->entry_header->filename_size, 
                                                      data_offset, 
                                                     &asset_file->init_arena);
            entry->asset_data.count = entry->entry_header->entry_data_size;
            entry->data_offset  = data_offset + entry->filename.count;
            current_file_offset += entry->entry_header->total_entry_size;
             
            c_hash_table_insert_pair(&asset_file->entry_hash, entry->filename, (s32)entry_index);
            c_hash_table_insert_pair(&asset_manager->asset_name_to_file, entry->filename, (s32)asset_file->ID);
            u64 hash_value = c_hash_table_value_from_key(entry->filename.data, entry->filename.count, asset_manager->asset_name_to_file.header.max_entries);

            log_debug("Inserting asset with name: '%s' with a name length of: '%d' into the name_to_file hash with file_index: '%d' hash_value: '%llu'...\n", C_STR(entry->filename), entry->filename.count, asset_file->ID, hash_value);

            asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
            asset_slot_t    *slot    = c_hash_table_get_value_ptr(&catalog->asset_lookup, entry->filename);
            Assert(slot);
            Assert(entry->entry_header->asset_type == catalog->catalog_type);

            ZeroStruct(*slot);
            slot->slot_state       = ASLS_Unloaded;
            slot->type             = (asset_type_t)entry->entry_header->asset_type;
            slot->name             = entry->filename;
            slot->package_entry    = entry;
            slot->ref_counter      = 0;
            slot->owner_asset_file = asset_file->file_info;
        }
        asset_manager->loaded_file_count += 1;
    }
    else
    {
        log_error("Failure to open file: type'%s'... invalid handle...\n", C_STR(filepath));
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

        jfd_package_entry_t *entry = asset_file->package_entries + asset_entry_index;
        Assert(entry->entry_header->asset_type != AT_Invalid && entry->entry_header->asset_type != AT_Count);

        asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
        Assert(catalog);

        result.type = (asset_type_t)entry->entry_header->asset_type;
        result.slot = s_asset_manager_get_asset_slot(catalog, name);
        result.owner_asset_file_index         = file_index;
        result.is_valid                       = true;
        if(result.slot->slot_state == ASLS_Unloaded)
        {
            s_asset_manager_load_asset_data(asset_manager, &result, hash_value);
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
