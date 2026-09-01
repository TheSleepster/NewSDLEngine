/* ========================================================================
   $File: test_manager.cpp $
   $Date: April 24 2026 03:17 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION 
#define DYNARRAY_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#include <c_base.h>
#include <c_types.h>
#include <c_program_flag_handler.h>
#include <c_dynarray.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_global_context.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

#if OS_LINUX

#define TEST_SECTION __attribute__((used, section("test_table")))
#define TEST(name)                                                   \
    static void name(void);                                          \
    static test_entry_t _reg_##name TEST_SECTION = { name, #name, __FILE__ }; \
    static void name(void)

typedef void test_func_t(void);
struct test_entry_t
{
    test_func_t *function;
    const char  *function_name;
    const char  *filename;
};

//
// TEST FILES ARE DECLARED HERE 

#include "test_new_metaprogram.cpp"

// TEST FILES ARE DECLARED HERE 
//

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

bool8
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

        return(result);
    }

    return(result);
}

test_results_t
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

void
test_manager_init(test_manager_t *manager)
{
    manager->entries     = __start_test_table;
    manager->entry_count = (u32)(__stop_test_table - __start_test_table);
}
#endif

int
main(void)
{
    c_global_context_init();

#if OS_LINUX
    test_manager_t manager = {};
    test_manager_init(&manager);

    // NOTE(Sleepster): Should just call "test_manager_add_test" on each of the tests in the test table X-macro and then execute them.
    log_info("Running '%u' test(s)...\n", manager.entry_count);
    test_results_t results = test_manager_run_tests(&manager);
    log_info("Tests finished running...\n\nTotal tests: '%u'\nTests Passed: '%u'\nTests Failed: '%u'\n", 
             results.tests_total, results.tests_passed, results.tests_failed);
#endif
}
