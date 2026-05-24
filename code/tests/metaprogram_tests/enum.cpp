/* ========================================================================
   $File: metaprogram_test_enum.cpp $
   $Date: May 21 2026 03:43 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

enum 
{
    SOME_DELCARED_ENUM_TYPE
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
