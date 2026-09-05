#if !defined(TEST_MANAGER_H)
/* ========================================================================
   $File: test_manager.h $
   $Date: September 05 2026 10:12 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION 
#define DYNARRAY_IMPLEMENTATION
#ifdef OS_LINUX

#include <stdio.h>
#include <stdlib.h>

#include <c_base.h>
#include <c_types.h>
#include <c_program_flag_handler.h>
#include <c_dynarray.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#define TEST_API static

#define TEST_MANAGER_H
#define TEST_SECTION __attribute__((used, section("test_table")))
#define TEST(name)                                                   \
    static void name(void);                                          \
    static test_entry_t _reg_##name TEST_SECTION = { name, #name, __FILE__ }; \
    static void name(void)

#define TEST_FAILURE(cond, msg, ...) \
if(!(cond)) {                   \
    log_fatal(msg, __VA_ARGS__) \
    return(-1)                  \
}                               \

#define SUBCASE(msg, ...)

typedef void test_func_t(void);
struct test_entry_t
{
    test_func_t *function;
    const char  *function_name;
    const char  *filename;
};

extern test_entry_t __start_test_table[];
extern test_entry_t __stop_test_table[];

struct test_results_t
{
    u32 tests_total;
    u32 tests_passed;
    u32 tests_failed;
};

struct test_manager_t
{
    test_entry_t *entries;
    u32           entry_count;
};

TEST_API bool8
test_manager_run_test(test_entry_t *entry)
{
    bool8 result = false;

    pid_t PID = fork();
    if(PID == 0)
    {
        entry->function();
        exit(0);
    }
    else if(PID > 0)
    {
        s32 exit_status = 0;

        waitpid(PID, &exit_status, 0);
        if(WIFEXITED(exit_status))
        {
            result = (WEXITSTATUS(exit_status) == 0);
        }
    }

    return(result);
}

TEST_API test_results_t
test_manager_run_tests(test_manager_t *manager)
{
    test_results_t result = {};

    u32 test_count = manager->entry_count;
    result.tests_total = test_count;

    for(u32 test_index = 0;
        test_index < test_count;
        ++test_index)
    {
        test_entry_t *entry = manager->entries + test_index;
        log_debug("\n[RUNNING]: '%s'...\n", entry->function_name);

        bool8 success = test_manager_run_test(entry);
        if(success)
        {
            log_info("\n[ PASSED ]\n");
            ++result.tests_passed;
        }
        else
        {
            log_warning("\n[ FAILURE ]\n");
            ++result.tests_failed;
        }
    }

    return(result);
}

TEST_API void
test_manager_init(test_manager_t *manager)
{
    manager->entries     = __start_test_table;
    manager->entry_count = (u32)(__stop_test_table - __start_test_table);
}
#endif

#endif // TEST_MANAGER_H

