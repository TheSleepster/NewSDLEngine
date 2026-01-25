#if !defined(GENERATED_TEST_H)
/* ========================================================================
   $File: GENERATED_test.h $
   $Date: January 25 2026 10:44 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST_H
#include <c_types.h>

struct test_element_data 
{
    u32 oranges;
    u32 internal_data[128];
    struct internal_members {
        u32 apples;
    };

    union {
        u32 banannas;
        u32 limes;
        u32 lemons;
    };
};

union test_union
{
    bool8 test_bool;
    u8    test_byte;
};

typedef struct test_element_typedeffed 
{
    u32 oranges;
    u32 internal_data[128];
    struct internal_members {
        u32 apples;
    };

    union citrus {
        u32 limes;
        u32 lemons;
    };
}test_element_typedeffed_t;

typedef union test_union_typedeffed
{
    bool8 test_bool;
    u8    test_byte;
}test_union_typedeffed_t;

#endif // GENERATED_TEST_H

