/* ========================================================================
   $File: metaprogram_test_complex_structures.cpp $
   $Date: May 21 2026 07:55 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

typedef struct test_element_typedeffed 
{
    s32    oranges              = -1;
    u32    internal_data[128];
    float  internal_after_array = 1.0f;
    struct orchard_data {
        u32 apples;
        u32 plums;
    };

    union citrus {
        u32 limes;
        u32 lemons;
    };
}test_element_typedeffed_t;

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
