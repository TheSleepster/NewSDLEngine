/*f========================================================================
   $File: asset_file_packer.cpp $
   $Date: December 09 2025 08:25 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <asset_file_packer/asset_file_packer.h>

#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_globals.h>
#include <c_log.h>
#include <c_string.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_dynarray.h>
#include <c_zone_allocator.h>
#include <c_threadpool.h>
#include <c_memory_arena.h>
#include <p_platform_data.h>

#include <s_asset_manager.h>
#include <r_asset_texture.h>
#include <r_asset_dynamic_render_font.h>

#include <c_globals.cpp>
#include <c_zone_allocator.cpp>
#include <c_memory_arena.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_file_api.cpp>
#include <c_hash_table.cpp>
#include <c_file_watcher.cpp>
#include <p_platform_data.cpp>

global_variable packer_state_t packer_state;

internal_api void
asset_packer_write_file(void)
{
    u32 offset = sizeof(asset_file_header_t);
    for(u32 packer_entry_index = 0;
        packer_entry_index < packer_state.next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = &packer_state.entries[packer_entry_index];

        entry->data_offset_from_start_of_file = offset;
        offset += entry->entry_data.count;
    }

    const u32 FILE_MAGIC_VALUE = ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' ');
    const u32 FILE_VERSION     = ASSET_FILE_VERSION;
    const u32 FLAGS_DWORD      = 100;
    const u32 OFFSET_TO_TOC    = offset;

    c_string_builder_append_value(&packer_state.header, (void*)&FILE_MAGIC_VALUE, sizeof(u32));
    c_string_builder_append_value(&packer_state.header, (void*)&FILE_VERSION,     sizeof(u32));
    c_string_builder_append_value(&packer_state.header, (void*)&FLAGS_DWORD,      sizeof(u32));
    c_string_builder_append_value(&packer_state.header, (void*)&OFFSET_TO_TOC,    sizeof(u32));

    c_string_builder_write_to_file(&packer_state.asset_file_handle, &packer_state.header);

    for(u32 packer_entry_index = 0;
        packer_entry_index < packer_state.next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = packer_state.entries + packer_entry_index;
        bool8 success = c_file_write_string(&packer_state.asset_file_handle, entry->entry_data);
        if(!success)
        {
            log_error("Failed to write entry: '%d'(%s) to the file...\n", packer_entry_index, entry->name);
        }
    }

    asset_file_table_of_contents_t table_of_contents = {};
    table_of_contents.magic_value = ASSET_FILE_MAGIC_VALUE('t', 'o', 'c', 'd');
    table_of_contents.entry_count = packer_state.next_entry_to_write;

    c_string_builder_append_value(&packer_state.table_of_contents, &table_of_contents, sizeof(asset_file_table_of_contents_t));

    /* THE ENTRIES ARE CONSTRUCTED HERE:
     *
     * Entries should be read like so...
     * - 4-byte value to indiciate the filename's length...
     * - The file's name (zero terminated...)
     * - 4-byte integer to indicate the filepath's length...
     * - The file's path (again, zero terminated...)
     * - 4-byte integer to indicate the entry's data length...
     * - 4-byte integer to indicate the entry's ID....
     * - 4-byte integer to indicate the entry's ID in the file...
     * - 4-byte integer to indicate the entry's asset type...
     * - 8-byte integer to indicate the entry's data offset...
     */    

    s8 null_term = 0;
    for(u32 packer_entry_index = 0;
        packer_entry_index < packer_state.next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = &packer_state.entries[packer_entry_index];

        c_string_builder_append_value(&packer_state.table_of_contents, &entry->name.count,                     sizeof(u32));
        c_string_builder_append_value(&packer_state.table_of_contents,  entry->name.data,                      entry->name.count);
        c_string_builder_append_value(&packer_state.table_of_contents, &null_term,                             sizeof(s8));

        c_string_builder_append_value(&packer_state.table_of_contents, &entry->filepath.count,                 sizeof(u32));
        c_string_builder_append_value(&packer_state.table_of_contents,  entry->filepath.data,                  entry->filepath.count);
        c_string_builder_append_value(&packer_state.table_of_contents, &null_term,                             sizeof(s8));

        c_string_builder_append_value(&packer_state.table_of_contents, &entry->entry_data.count,               sizeof(u32));
        c_string_builder_append_value(&packer_state.table_of_contents, &entry->ID,                             sizeof(u32));
        c_string_builder_append_value(&packer_state.table_of_contents, &entry->file_ID,                        sizeof(u32));
        c_string_builder_append_value(&packer_state.table_of_contents, &entry->type,                           sizeof(asset_type_t));
        c_string_builder_append_value(&packer_state.table_of_contents, &entry->data_offset_from_start_of_file, sizeof(u64));
    }

    c_string_builder_write_to_file(&packer_state.asset_file_handle, &packer_state.table_of_contents);
    packer_state.entry_count = packer_state.next_entry_to_write;
}

internal_api void
asset_file_add_entry(string_t     filename,
                     string_t     filepath,
                     string_t     file_data,
                     asset_type_t type)
{
    // NOTE(Sleepster): zeroth entry is null 
    asset_package_entry_t *entry = packer_state.entries + packer_state.next_entry_to_write;
    u64 ID = c_fnv_hash_value(filename.data, filename.count, default_fnv_hash_value);

    entry->name       = c_string_make_copy(&packer_state.packer_arena, filename);
    entry->filepath   = c_string_make_copy(&packer_state.packer_arena, filepath);
    entry->entry_data = c_string_make_copy(&packer_state.packer_arena, file_data);
    entry->ID         = (ID % MANAGER_HASH_TABLE_SIZE + MANAGER_HASH_TABLE_SIZE) % MANAGER_HASH_TABLE_SIZE;
    entry->file_ID    = packer_state.next_entry_to_write;
    entry->type       = type;

    packer_state.next_entry_to_write++;
}

VISIT_FILES(get_resource_dir_files)
{
    string_t filename = visit_file_data->filename;
    string_t filepath = visit_file_data->fullname;
    string_t file_ext = c_string_get_file_ext_from_path(filename);

    string_t filename_no_ext = filename;
    filename_no_ext.count   -= file_ext.count;

    asset_type_t type = AT_NONE;
    if(c_string_compare(file_ext, STR(".ttf")))
    {
        type = AT_FONT;
    }
    else if(c_string_compare(file_ext, STR(".wav")))
    {
        type = AT_SOUND;
    }
    else if(c_string_compare(file_ext, STR(".png")))
    {
        type = AT_BITMAP;
    }
    else if(c_string_compare(file_ext, STR(".glsl")))
    {
        type = AT_SHADER;
    }
    
    if(type != AT_NONE)
    {
        string_t data = c_file_read(filepath, READ_ENTIRE_FILE, 0);
        asset_file_add_entry(filename_no_ext, filepath, data, type);

        log_info("Adding asset: '%s' to the built file...\n", filename.data);
    }
    else
    {
        log_warning("Could not add asset file named: '%s'... Asset's type was unknown...\n", filename.data);
    }
}

int
main(int argc, char **argv)
{
    gc_setup();
    packer_state.packer_arena      = c_arena_create(GB(16));

    packer_state.packed_file_name  = STR("asset_data");
    packer_state.resource_dir_path = STR("../res");
    packer_state.output_dir        = STR("../res");
    packer_state.file_extension    = STR(".wad");

    c_string_builder_init(&packer_state.header,            MB(500));
    c_string_builder_init(&packer_state.data,              MB(500));
    c_string_builder_init(&packer_state.table_of_contents, MB(500));

    packer_state.output_dir = c_string_concat(&packer_state.packer_arena, packer_state.output_dir, STR("/"));

    string_t full_packed_file_name = c_string_concat(&packer_state.packer_arena, packer_state.output_dir, packer_state.packed_file_name);
             full_packed_file_name = c_string_concat(&packer_state.packer_arena, full_packed_file_name,   packer_state.file_extension);

    packer_state.asset_file_handle = c_file_open(full_packed_file_name, true);
    Assert(packer_state.asset_file_handle.handle != INVALID_FILE_HANDLE);

    log_info("Packing assets to file: '%s'...\n", full_packed_file_name.data);

    visit_file_data_t visit_data = c_directory_create_visit_data(get_resource_dir_files, true, null);
    c_directory_visit(packer_state.resource_dir_path, &visit_data);

    asset_packer_write_file();
    log_info("Assets packed successfully...\n");

    getchar();
}
