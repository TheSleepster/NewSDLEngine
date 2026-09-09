#if !defined(TEST_MANAGER_H)
/* ========================================================================
   $File: test_manager.h $
   $Date: September 05 2026 10:12 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define TEST_MANAGER_H

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION 
#define DYNARRAY_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#include <c_base.h>
#include <c_types.h>
#include <c_program_flag_handler.h>
#include <c_dynarray.h>

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
# if OS_WINDOWS
#   define TEST_SECTION_BEGIN __attribute__((used, section("test_table$A")))
#   define TEST_SECTION       __attribute__((used, section("test_table$M")))
#   define TEST_SECTION_END   __attribute__((used, section("test_table$Z")))
# else
#   define TEST_SECTION       __attribute__((used, section("test_table")))
# endif // OS_WINDOWS
#else 
# define TEST_SECTION
# define TEST_SECTION_BEGIN
# define TEST_SECTION_END
#endif // COMPILER_CLANG || COMPILER_GCC

#define TEST(name)                                                        \
    static void name(void);                                               \
    static const test_entry_t _reg_##name TEST_SECTION = {                \
        name, #name, __FILE__                                             \
    };                                                                    \
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

#if OS_WINDOWS

TEST_SECTION_BEGIN const test_entry_t test_table_begin = { (test_func_t*)1, "sentinel_begin", "" };
TEST_SECTION_END   const test_entry_t test_table_end   = { (test_func_t*)1, "sentinel_end", "" };

#define TEST_TABLE_BEGIN (&test_table_begin + 1)
#define TEST_TABLE_END   (&test_table_end)

#else

extern const test_entry_t __start_test_table[];
extern const test_entry_t __stop_test_table[];

#define TEST_TABLE_BEGIN __start_test_table
#define TEST_TABLE_END   __stop_test_table
#endif // OS_WINDOWS

#define TEST_API static

TEST_API bool8
test_manager_run_test(const test_entry_t *entry)
{
    bool8 result = false;

#if 0
    pid_t PID = fork();
    if(PID == 0)
    {    
        entry->function();
        _exit(0);
    }
    else if(PID > 0)
    {
        s32 exit_status = 0;
        waitpid(PID, &exit_status, 0);
        if(WIFEXITED(exit_status))
        {
            result = (WEXITSTATUS(exit_status) == 0);
        }
        else if(WIFSIGNALED(exit_status))
        {
            s32 signal = WTERMSIG(exit_status);

            log_info("CHILD CRASHED: signal %d (%s)\n",
                     signal,
                     strsignal(signal));
        }
    }
#endif
    entry->function();
    result = true;

    return(result);
}

TEST_API test_results_t
test_manager_run_tests(void)
{
    test_results_t result = {};

    u32 test_count = 0;
    for(const test_entry_t *entry = TEST_TABLE_BEGIN;
        entry < TEST_TABLE_END;
        ++entry)
    {
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

        ++test_count;
    }

    result.tests_total = test_count;
    return(result);
}

#endif // TEST_MANAGER_H

