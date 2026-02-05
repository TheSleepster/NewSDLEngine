#if !defined(GENERATED_TEST_H)
/* ========================================================================
   $File: GENERATED_test.h $
   $Date: January 25 2026 10:44 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST_H
#include <c_types.h>

typedef struct test_thing
{
    const u32             element0[64];
    DynArray_t(string_t)  element1;
    HashTable_t(s32)      element2;
    const volatile u32    element3;

    // NOTE(Sleepster): We need to make it so that test_structure_t is a member of 
    // test_thing_t
    struct test_structure {
        u32 test_element;
        u32 other_test_element;

        // NOTE(Sleepster): Same thing here, make it so that other_thing_t is a mmember
        // of test_structure_t
        struct other_thing {
            s32 element_inside_other_struct0;
            s32 element_inside_other_struct1;
        }other_thing;
    }test_structure;
}test_thing_t;

#endif // GENERATED_TEST_H

