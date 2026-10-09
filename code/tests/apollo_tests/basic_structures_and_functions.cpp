/* ========================================================================
   $File: basic_structures_and_functions.cpp $
   $Date: October 08 2026 09:19 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// NOTE(Sleepster): Basic because there's no templates and it basically covers
// all the structure, enum, and function stuff.

typedef int s32;
using float32 = float;

constexpr int MAX_ARRAY_SIZE = 40;

namespace test {
enum test_enum
{
    TEST_ENUM_INVALID,
    TEST_ENUM_FIRST,
    TEST_ENUM_SECOND,
};
};

[[test_attrib]]
enum test_enum2
{
    TEST_ENUM2_INVALID = 1,
    TEST_ENUM2_FIRST = 3,
    TEST_ENUM2_SECOND = 1ull << 31ull,
};

struct test_simple_struct_t
{
    s32 member0;
    int member1;
    float32 member2;
    float member3[32];
    float member4[32][32][32];
};

struct test_simple_struct2_t 
{
    struct nested_item {
        union {
            float float_table[32][32];
            int   int_table[MAX_ARRAY_SIZE][MAX_ARRAY_SIZE];
        };
    };

    int after_union;
    void init();
    void more_complex_init(char *name, void *data);

    test_simple_struct2_t();
    ~test_simple_struct2_t();
};

struct test_simple_struct_inheritance_t: test_simple_struct_t
{
    struct inherited_nested_item {
        union {
            float float_table[32][32];
            int   int_table[32][32];
        };
    };

    int after_union;
    void init();
    void more_complex_init(char *name, void *data);

    test_simple_struct_inheritance_t();
    ~test_simple_struct_inheritance_t();
};

void test_function();

void
test_function()
{
}

int
test_function2(char *blah, 
               int data, 
               int blah2, 
               test_simple_struct2_t structure, 
               test_simple_struct2_t *structure_pointer = 0)
{
}
