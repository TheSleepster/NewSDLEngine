/* ========================================================================
   $File: test_new_metaprogram.cpp $
   $Date: May 21 2026 10:14 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
TEST(basic_macro)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_basic_macro.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(complex_macro)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_complex_macro.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_typedef)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_simple_typedef.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_structures)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_basic_structure.cpp"));
    Assert(sys_wait_for_process(process));
}


TEST(complex_structures)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_complex_structures.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(enums)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_enum.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(function_decls)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_declared_functions.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(macroed_function_decls)
{
    void *process = sys_create_process(STR("../build/new_metaprogram"), STR("--filename=tests/metaprogram_tests/metaprogram_test_casey_style_external_function.cpp"));
    Assert(sys_wait_for_process(process));
}

