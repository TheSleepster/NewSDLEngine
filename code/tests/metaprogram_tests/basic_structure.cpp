/* =======================================================================>=
   $File: metaprogram_test_basic_structure.cpp $
   $Date: May 21 2026 12:39 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#if 0
// C variant
typedef struct item
{
    int item_member;
    int item_member_one;

    // I suppose this isn't so much a c style decl vs c++ style decl issue, more so
    // an issue with how we parse members such as this...
    struct {
        item *subitem;
        char *name;
    }other_item_group;

    // TODO: Not found in this version...
    int item_member_one;
}item_t;
#endif

// C++ varient
struct item
{
    int item_member;
    int item_member_one;

    struct other_item_group {
        item *subitem;
        char *name;
    };

    item *get_item(char *name);

    // TODO: Not found in this version...
    int item_member_one;
};