/* ========================================================================
   $File: test_athena.cpp $
   $Date: May 21 2026 10:14 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
TEST(basic_macro)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/basic_macro.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(complex_macro)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/complex_macro.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_typedef)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/simple_typedef.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_structures)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/basic_structure.cpp"));
    Assert(sys_wait_for_process(process));
}


TEST(complex_structures)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/complex_structures.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(enums)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/enum.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(function_decls)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/declared_functions.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(macroed_function_decls)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/casey_style_external_function.cpp"));
    Assert(sys_wait_for_process(process));
}

