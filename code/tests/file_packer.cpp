/* ========================================================================
   $File: file_packer.cpp $
   $Date: January 09 2026 01:53 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <asset_file_packer/jfd_asset_file.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

internal_api void *
get_chunk_data(byte *iterator, u32 chunk_size)
{
    void *result = null;

    return(result);
}

int
main()
{
    file_t data = c_file_open(STR("asset_data.jfd"), false);
    string_t packed_asset_data = c_file_read_entirety(STR("asset_data.jfd"));
    u64 size = c_file_get_size(&data);

    Assert(packed_asset_data.count == size);
        
    Assert(packed_asset_data.data != null);
    Assert(packed_asset_data.count > 0);

    jfd_file_header_t *file_header = (jfd_file_header_t*)packed_asset_data.data;
    Assert(file_header->magic_value == ASSET_FILE_HEADER_MAGIC);
    u32   bytes_remaining = packed_asset_data.count - sizeof(jfd_file_header_t);

    byte *iterator = packed_asset_data.data + sizeof(jfd_file_header_t); 
    while(bytes_remaining > 0)
    {
        jfd_package_chunk_header_t *chunk_header = (jfd_package_chunk_header_t*)iterator;
        Assert(chunk_header->magic_value == ASSET_FILE_CHUNK_MAGIC);

        char *name = (char*)((byte*)chunk_header + sizeof(jfd_package_chunk_header_t));

        char buffer[4096];
        memcpy(buffer, name, chunk_header->filename_size);
        log_info("File name is: '%s'...\n", buffer);

        //iterator        += chunk_header->total_entry_size;
        //bytes_remaining -= chunk_header->total_entry_size;

        u32 advance = sizeof(jfd_package_chunk_header_t) + (chunk_header->filename_size + chunk_header->entry_data_size);
        iterator += advance;
        bytes_remaining -= advance;
    }
    Assert(bytes_remaining == 0);
    log_info("End of file reached...\n");
    getchar();

    return(0);
}
