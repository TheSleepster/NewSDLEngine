/* ========================================================================
   $File: metaprogram_test_basic_structure.cpp $
   $Date: May 21 2026 12:39 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// C++ variant
struct item
{
    int item_member;
    int item_member_one;

    struct  other_item_group {
        item *subitem;
        char *name;
    };
    // TODO: Not found...
    int item_member_one;
};

static void
item()
{
}