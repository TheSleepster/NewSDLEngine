/* ========================================================================
   $File: new_hash_table.cpp $
   $Date: January 06 2026 06:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>

#include <c_dynarray.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

#include <c_new_hash_table.h>

struct thing
{
    u32 ID;
    u32 item;
    u32 value0;
    u32 value1;
    u32 value2;
    u32 value3;
};

int
main()
{
    HashTable(thing, 4096) table;
}
