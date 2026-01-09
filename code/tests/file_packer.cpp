/* ========================================================================
   $File: file_packer.cpp $
   $Date: January 09 2026 01:53 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <asset_file_packer/new_asset_packer.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

int
main()
{
    string_t packed_asset_data = c_file_read_entirety(STR("asset_data.jfd"));
    Assert(packed_asset_data.data != null);
    Assert(packed_asset_data.count > 0);

    jfd_file_header_t *header = (jfd_file_header_t*)packed_asset_data.data;
    Assert(header->magic_value == ASSET_FILE_HEADER_MAGIC);
}
