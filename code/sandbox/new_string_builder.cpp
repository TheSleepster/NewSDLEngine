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

int
main(void)
{
    string_builder_t builder;
    c_string_builder_init(&builder, KB(300));

    file_t test_file = c_file_open(STR("../../code/tests/test.txt"), true);

    string_t file_data = c_file_read_entirety(STR("../../code/tests/string_builder.cpp"));
    c_string_builder_append_data(&builder, file_data);
    c_string_builder_flush_to_file(&test_file, &builder);

    string_t file_data1 = c_file_read_entirety(STR("../deps/stb/stb_image.h"));
    c_string_builder_append_data(&builder, file_data1);
    c_string_builder_dump_to_file(&test_file, &builder);

    getchar();
}
