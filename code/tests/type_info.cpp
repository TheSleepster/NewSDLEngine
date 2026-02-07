/* ========================================================================
   $File: type_info.cpp $
   $Date: February 03 2026 03:30 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <stddef.h>

#include <c_base.h>
#include <c_types.h>
#include <c_math.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

#include <tests/GENERATED_test.h>
#include <meta/GENERATED_test_output.h>

int
main(void)
{
    const type_info_t *type_data = c_meta_get_type_info_by_name(STR("test_structure"));
    for(u32 member_index = 0;
        member_index < type_data->struct_info->member_count;
        ++member_index)
    {
        type_info_member_t *member = type_data->struct_info->members + member_index;
        log_info("Member by name: '%s' found...\n", member->name);
    }

    string_t member_name = STR("other_thing");
    const type_info_member_t *member_data = c_meta_get_member_info(type_data->struct_info, member_name);

    (void)member_data;
}
