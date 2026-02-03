#if !defined(GENERATED_TEST_H)
/* ========================================================================
   $File: GENERATED_test.h $
   $Date: January 25 2026 10:44 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST_H
#include <c_types.h>

enum thing_t
{
    THING_1,
    THING_2,
    THING_3,
    THING_4,
};

typedef enum thing_stuff
{
    THING_STUFF_1,
    THING_STUFF_2,
    THING_STUFF_3,
    THING_STUFF_4,
}thing_stuff_t;

typedef struct test_thing
{
    const u32             element0[64];
    DynArray_t(string_t)  element1;
    HashTable_t(s32)      element2;
    const volatile u32    element3;

    struct test_structure {
        u32 test_element;
        u32 other_test_element;

        struct other_thing {
            s32 element_inside_other_struct0;
            s32 element_inside_other_struct1;
        }other_thing_t;
    }test_structure_t;
}test_thing_t;

typedef struct test_object
{
    u32 test_object_element0;
    u32 test_object_element1;
    union {
        u32 test_object_element2;
        u32 test_object_element4;
    };
};

#endif // GENERATED_TEST_H

