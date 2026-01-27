#if !defined(GENERATED_TEST2_H)
/* ========================================================================
   $File: GENERATED_test2.h $
   $Date: January 25 2026 06:19 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST2_H
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
        u32 grapes;
        u32 tomatos;
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
    u32 internal_after_array;
    struct orchard_data {
        u32 apples;
        u32 plums;
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


#endif // GENERATED_TEST2_H

