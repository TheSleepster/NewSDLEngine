/* ========================================================================
   $File: metaprogram_test_complex_structures.cpp $
   $Date: May 21 2026 07:55 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define MAX_INTERNAL_MEMBERS        128
#define MAX_PAREN_INTERNAL_MEMBERS (128)

#define internal static

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

enum data_types 
{
    DATA_TYPE_TEST_ENUM_THING = 1 << 31,
};

struct test_element_data 
{
    u32 oranges;
    u32 internal_data[MAX_INTERNAL_MEMBERS];
    u32 more_internal_data[MAX_PAREN_INTERNAL_MEMBERS];
    struct internal_members {
        u32 apples;
    };

    union {
        u32 banannas;
        u32 grapes;
        u32 tomatos;
    };
};
