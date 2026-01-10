/* ========================================================================
   $File: new_string_builder.cpp $
   $Date: January 10 2026 07:06 am $
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

int
main(void)
{
    new_string_builder_t builder;
    c_new_string_builder_init(&builder, MB(1000));

    string_t file_data = c_file_read_entirety(STR("new_string_builder.cpp"));
    c_new_string_builder_append_data(&builder, file_data);

    file_t test_file = c_file_open(STR("test.txt"), true);
    c_new_string_builder_dump_to_file(&test_file, &builder);
}
