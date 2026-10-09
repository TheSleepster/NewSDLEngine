/* ========================================================================
   $File: basic_structure.cpp $
   $Date: October 06 2026 02:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

typedef int s32;

struct test_t
{
    int   first_argument = 1;
    int   second_argument = 1 << 30;
    float third_argument;

    s32   typedef_argument;
    s32   array[32];

    void init(int apples);
};

void init(int apples);
void init(float apples);

struct test_pointer_t
{
    int   first_argument;
    float second_argument;
    int  *third_argument;

    test_pointer_t *next_structure;
    test_t          nested_test;
};

typedef struct c_test
{
    int   first_argument;
    int   second_argument;
    float third_argument;
}c_test_t;

enum test_enum
{
    TEST_ENUM_FIRST,
    TEST_ENUM_SECOND,
    TEST_ENUM_THIRD,
    TEST_ENUM_FOURTH,
};

enum class test_enum_class
{
    TEST_ENUM_CLASS_FIRST,
    TEST_ENUM_CLASS_SECOND,
    TEST_ENUM_CLASS_THIRD,
    TEST_ENUM_CLASS_FOURTH,
};