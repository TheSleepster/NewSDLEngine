/* ========================================================================
   $File: metaprogram_test_basic_structure.cpp $
   $Date: May 21 2026 12:39 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// C++ variant
struct item_t 
{
    int item_member;
    int item_member_one;
};

// C Variant
typedef struct other_item
{
    int item_typedeffed_member;
    int item_typedeffed_member_one;
}other_item_t;

// C++ forward declare
struct item_t;

// C style forward declare 
typedef struct other_item other_item_t;
