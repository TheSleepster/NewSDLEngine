/* ========================================================================
   $File: apollo.cpp $
   $Date: October 06 2026 03:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "test_manager.h"

TEST(basic_structure)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/basic_structure.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(complex_structure)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/complex_structure.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_template)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/basic_template.cpp"));
    Assert(sys_wait_for_process(process));
}


TEST(basic_template_array)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/basic_template_array.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_function)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/basic_function.cpp"));
    Assert(sys_wait_for_process(process));
}

TEST(basic_template_function)
{
    void *process = sys_create_process(STR("../build/Linux/Debug/apollo"), STR("tests/apollo_tests/basic_template_function.cpp"));
    Assert(sys_wait_for_process(process));
}

int
main(void)
{
    c_global_context_init();
    test_manager_run_tests();

    return(0);
}
