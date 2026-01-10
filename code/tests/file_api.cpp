/* ========================================================================
   $File: file_api.cpp $
   $Date: December 25 2025 01:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include <stdio.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>
#include <c_file_api.h>
#include <c_string.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>

#define MATH_IMPLEMENTATION
#include <c_math.h>

int
main(void)
{
    string_t file_data0 = c_file_read_entirety(STR("c_file_api.d"));
    printf("%s\n", C_STR(file_data0));
}
