/* ========================================================================
   $File: metaprogram_test_enum.cpp $
   $Date: May 21 2026 03:43 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
enum chained_values {
    TEST_VAL  = 1 << 1,
    TEST_VAL2 = 1 << 2,
    TEST_VAL3 = 1 << 3,
    TEST_VAL4 = TEST_VAL|TEST_VAL2|TEST_VAL3,
};

enum test_enum_type_t 
{
    SOME_OTHER_DECLARED_TYPE       = (1 << 31),
    SOME_OTHER_OTHER_DECLARED_TYPE = 1,
};

typedef enum some_other_typedeffed_item 
{
    WOW_THIS_IS_TYPEDEFFED = (1 << 31),
}some_other_typedeffed_item_t;


typedef enum some_other_typedeffed_item_without_a_comma
{
    WOW_THIS_IS_TYPEDEFFED = (1 << 31)
}some_other_typedeffed_item_without_a_comma_t;

enum 
{
    SOME_DELCARED_ENUM_TYPE
};