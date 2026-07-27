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

struct test_structure {
    int apples;
    int oranges;
    int bananas;

    void apples_test_func(char *name);
};

#define ATHENA_IMPLMENTATION
#include "generated_test.h"

int
main(int argc, char **argv)
{
    const type_info_t *info = type_info("test_structure");
    if(info)
    {
        const type_info_struct_t *structure = (const type_info_struct_t *)info;
        printf("%d members\n", structure->member_count);

        for(u32 member_index = 0;
            member_index < structure->member_count;
            ++member_index)
        {
            const type_info_member_t *member = structure->members + member_index;
            printf("Member name: '%s'...\n", member->member_name);
        }

        const type_info_member_t *function = athena_get_member_info(info, "apples_test_func");
        if(function)
        {
            const type_info_procedure_t *test_proc_data = (const type_info_procedure_t *)function->type_info;
            int x = 0;
        }
    }

    return(0);
}
