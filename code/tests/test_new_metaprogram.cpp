/* ========================================================================
   $File: test_athena.cpp $
   $Date: May 21 2026 10:14 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_string.cpp>
#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_file_watcher.cpp>
#include <c_file_api.cpp>
#include <c_zone_allocator.cpp>
#include <c_global_context.cpp>

#include "test_manager.h"

TEST(basic_macros)
{
    printf("BASIC!!!!!!!!!");
    printf("BASIC!!!!!!!!!");
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/basic_macro.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(complex_macros)
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

TEST(casey_style_declarations)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/casey_style_external_function.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(cpp_structures)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/cpp_class.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(cpp_member_functions)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/member_functions.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(cpp_namespaces)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/namespaces.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(templates)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/templates.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(nested_macros)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/nested_macros.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(overloads)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/overloads.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(cpp_attributes)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/attributes.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(c_style_structures)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/c_style_structures.cpp"));
    Assert(false);
    Assert(sys_wait_for_process(process));
}

TEST(conditional_define)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/conditional_define.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(anon_internal_unions)
{
    void *process = sys_create_process(STR("../build/athena"), STR("--filename=tests/metaprogram_tests/nested_anon_unions.cpp"));
    Assert(sys_wait_for_process(process));
}

int
main(void)
{
    c_global_context_init();

    test_manager_t test_manager = {};
    printf("DIRECT: basic_macros = %p\n", (void *)basic_macros);
    test_manager_init(&test_manager);
    test_manager_run_tests(&test_manager);

    return(0);
}
