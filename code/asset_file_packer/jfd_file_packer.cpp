/* ========================================================================
   $File: new_file_packer.cpp $
   $Date: January 09 2026 11:20 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <stdlib.h>

#define HASH_TABLE_IMPLEMENTATION
#define MATH_IMPLEMENTATION
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
#include <c_hash_table.h>
#include <p_platform_data.h>

#include <s_asset_manager.h>

#include <c_globals.cpp>
#include <c_zone_allocator.cpp>
#include <c_memory_arena.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <p_platform_data.cpp>

#include "jfd_asset_file.h"

#if 0
constexpr const char *commands[] = {
    "--resource_dir",
    "--output_filename",
    "--output_dir"
};
#endif

typedef struct file_packer_state
{
    string_t         resource_dir;
    string_t         output_dir;
    string_t         output_filename;
    file_t           output_file;

    memory_arena_t   builder_arena;
    memory_arena_t   packages_arena;

    string_builder_t builder;
    string_builder_t header_builder;

    asset_entry_info_t asset_entries[MAX_ENTRIES];
    u32                asset_next_entry_to_write;
}file_packer_state_t;

global_variable file_packer_state_t packer_state;

internal_api void
parse_arguments(s32 arg_count, char **args)
{
   for(s32 arg_index = 1;
        arg_index < arg_count;
        ++arg_index)
    {
        string_t argument = STR(args[arg_index]);
        const string_t command_appendment = STR("--");
        string_t test_arg = argument;
        test_arg.count = 2;

        if(c_string_compare(test_arg, command_appendment))
        {
            log_warning("command currently have no function...\n");
        }
        else
        {
            log_error("You must append your command with '--'...\n");
        }
    }
}

internal_api
VISIT_FILES(gather_all_asset_file_entries)
{
    string_t filename = visit_file_data->filename;
    string_t filepath = visit_file_data->fullname;
    string_t file_ext = c_string_get_file_ext_from_path(filename);

    string_t filename_no_ext = filename;
    filename_no_ext.count   -= file_ext.count;

    asset_type_t type = AT_Invalid;
    if(c_string_compare(file_ext,      STR(".ttf"))) type = AT_Font;
    else if(c_string_compare(file_ext, STR(".wav"))) type = AT_Sound;
    else if(c_string_compare(file_ext, STR(".png"))) type = AT_Bitmap;
    else if(c_string_compare(file_ext, STR(".spv"))) type = AT_Shader;

    if(type != AT_Invalid)
    {
        log_info("Adding asset entry: '%s'...\n", C_STR(filename));

        asset_entry_info *entry = packer_state.asset_entries + packer_state.asset_next_entry_to_write++;
        ZeroStruct(*entry);

        entry->filename   = c_string_make_copy(&packer_state.packages_arena, filename_no_ext);
        entry->fullpath   = c_string_make_copy(&packer_state.packages_arena, filepath);
        entry->asset_data = c_file_read_entirety(filepath, &packer_state.packages_arena);
        entry->type       = type;
        if(entry->asset_data.data == null)
        {
            log_error("Error reading file: '%s'...\n", C_STR(entry->filename));
        }
    }
    else
    {
        log_warning("Asset of name: '%s' is a type we cannot recognize... skipping...\n", C_STR(filename));
    }
}

int
main(int arg_count, char **args)
{
    ZeroStruct(packer_state);
    gc_setup();

    packer_state.builder_arena  = c_arena_create(GB(1));
    packer_state.packages_arena = c_arena_create(GB(6));
    c_string_builder_init(&packer_state.builder, MB(500));
    c_string_builder_init(&packer_state.header_builder, GB(6));

    if(arg_count < 2)
    {
        byte *buffer = c_arena_push_array(&packer_state.builder_arena, byte, 1024);
        Expect(buffer != null, "We have failed to allocate the buffer for our current working dir...\n");
        sys_directory_get_current_working_dir(buffer ,1024);

        packer_state.resource_dir    = STR((char*)buffer);
        packer_state.output_dir      = STR((char*)buffer);
        packer_state.output_filename = STR("asset_data.jfd");

        log_info("No args passed... building in this directory...\n");
    }
    else
    {
        parse_arguments(arg_count, args);
    }
    log_info("Packing assets from high level directory: '%s' into file: '%s'...\n", C_STR(packer_state.resource_dir), C_STR(packer_state.output_filename));

    string_t output_fullpath = c_string_concat(&packer_state.builder_arena, packer_state.output_dir, STR("/"));
    output_fullpath = c_string_concat(&packer_state.builder_arena, output_fullpath, packer_state.output_filename);

    packer_state.output_file = c_file_open(output_fullpath, true);
    if(packer_state.output_file.handle == INVALID_FILE_HANDLE)
    {
        log_fatal("Could not create file: '%s'... Exiting...\n", packer_state.output_filename);
        exit(-1);
    }

    visit_file_data_t visit_info = c_directory_create_visit_data(gather_all_asset_file_entries, true, null);
    c_directory_visit(packer_state.resource_dir, &visit_info);
    if(packer_state.asset_next_entry_to_write > 0)
    {
        // NOTE(Sleepster): Build header 
        jfd_file_header_t header = {};
        header.magic_value = ASSET_FILE_HEADER_MAGIC;
        header.version     = ASSET_FILE_VERSION;
        header.flags       = 0;
        header.entry_count = packer_state.asset_next_entry_to_write;

        c_string_builder_append_value(&packer_state.header_builder, (void*)&header, sizeof(header));
        c_string_builder_flush_to_file(&packer_state.output_file, &packer_state.header_builder);

        // NOTE(Sleepster): Write out asset blocks 
        for(u32 asset_entry_index = 0;
            asset_entry_index < packer_state.asset_next_entry_to_write;
            ++asset_entry_index)
        {
            asset_entry_info_t *asset_info = packer_state.asset_entries + asset_entry_index;
            Assert(asset_info->asset_data.data != null);
            Assert(asset_info->asset_data.count > 0);

            Assert(asset_info->filename.data != null);
            Assert(asset_info->filename.count > 0);

            Assert(asset_info->fullpath.data != null);
            Assert(asset_info->fullpath.count > 0);

            Assert(asset_info->type != AT_Invalid);

            // TODO(Sleepster): filenames should be null terminated...
            jfd_chunk_data chunk_data = {};
            chunk_data.chunk_header.magic_value      = ASSET_FILE_CHUNK_MAGIC;
            chunk_data.chunk_header.total_entry_size = sizeof(jfd_package_chunk_header_t) + (asset_info->asset_data.count + asset_info->filename.count);
            chunk_data.chunk_header.asset_type       = asset_info->type;
            chunk_data.chunk_header.filename_size    = asset_info->filename.count;
            chunk_data.chunk_header.entry_data_size = asset_info->asset_data.count;

            chunk_data.filename_data    = asset_info->filename.data;
            chunk_data.asset_entry_data = asset_info->asset_data.data;

            c_string_builder_append_value(&packer_state.builder, (void*)&chunk_data.chunk_header, sizeof(jfd_package_chunk_header_t));
            c_string_builder_append_data(&packer_state.builder, asset_info->filename);
            c_string_builder_append_data(&packer_state.builder, asset_info->asset_data);
        }
        c_string_builder_flush_to_file(&packer_state.output_file, &packer_state.builder);
    }
    else
    {
        log_info("Exiting. There are no assets to pack...\n");
        exit(0);
    }

    log_info("Finished packing!...\n");
    getchar();
    return(0);
}


