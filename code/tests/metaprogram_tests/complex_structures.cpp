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
    int    *oranges;
    int     internal_data[128];
    float   internal_after_array = 4;
    struct orchard_data {
        int apples;
        int  plums;
    };

    union citrus {
        double limes;
        double lemons;
    };
}test_element_typedeffed_t;

enum data_types 
{
    DATA_TYPE_NEGATIVE_ENUM_VALUE = 1 << 30,
    DATA_TYPE_TEST_ENUM_VALUED    = 1 << 31,
    DATA_TYPE_TEST_ENUM_THING     = 1 << 32,
};

struct test_element_data 
{
    int oranges;
    int internal_data[MAX_INTERNAL_MEMBERS];
    int more_internal_data[MAX_PAREN_INTERNAL_MEMBERS];
    float floating_point_test_item;
    struct internal_members {
        int apples;
    };

    union {
        int banannas;
        int grapes;
        int tomatos;
    };
};
