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
    int item_member_two;
}item_t;
#endif

#if 0
struct item;

// C++ varient
struct item
{
    int item_member;
    int item_member_one;
    int item_member_two;
};

item *item(char *name) {
    item result = malloc(sizeof(item));
    return(result);
}

item *item(void *information) {
    item result = malloc(sizeof(item));
    return(result);
}
#endif
#if 0
#define Dynarray_t(type) \
    struct { \
        type *array; \
        u32   count; \
    }

struct item {
public:
    int testint;
    int testint2;
    int testint3;
    int testint4;
    int testint5;

    Dynarray_t(int) test_array;
    Dynarray_t(int) test_array2;

    float test_after_array;
};

typedef struct item {
    int test_int_blah;
    struct {
        int internal_test_int;
    };

    int other_test_int;
}item_t;

typedef struct asset_handle
{
    bool32             is_valid;
    asset_type_t       type;
    s32                owner_asset_file_index;

    subtexture_data_t *subtexture_data;
    asset_slot_t      *slot;

    // TODO(Sleepster): 
    // Not happy about these.. These should ONLY be used to signal loading or unloading of an asset's data
    asset_manager_t   *asset_manager;
    asset_catalog_t   *catalog;
    union {
        texture2D_t           *texture;
        shader_t              *shader;
        material_data_t       *material_info;
        dynamic_render_font_t *dynamic_render_font;
    };
}asset_handle_t;
#endif

#define TypeOf(type) __typeof__(type)
#define DynArray_t(type) TypeOf((type*)null)

struct test_structure {
    int pre_test_int;
    DynArray_t(int) integers;
    int post_test_int;
};

struct test2 {
    alignas(64) int test;
};