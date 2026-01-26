#if !defined(GENERATED_TEST_H)
/* ========================================================================
   $File: GENERATED_test.h $
   $Date: January 25 2026 10:44 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST_H
#include <c_types.h>

struct test_thing_t
{
    u32   element0;
    char *element1[128];
    const u32 element_array;

    DynArray_t(u32) dynamic_elements;

    struct test_nested_struct
    {
        const volatile u32 nested_element[128];
    };

    const s32 thingy_after_nested_element;
};

#endif // GENERATED_TEST_H

