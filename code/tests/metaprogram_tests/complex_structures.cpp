/* ========================================================================
   $File: metaprogram_test_complex_structures.cpp $
   $Date: May 21 2026 07:55 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
constexpr u32 MAX_INTERNAL_MEMBERS = 128;
constexpr u32 MINIMUM_MEMBERS      = MAX_INTERNAL_MEMBERS / 4;

// Nested macros are unaccounted for...
#define MAX_PAREN_INTERNAL_MEMBERS (128)
#define internal static

template <typename T>
void
some_template_function(hash_table_t<T> item);

template <typename T>
void
other_template_function(hash_table_t<T> item)
{
    item.data = 0;
    fdjkslfkjldsfds
    fsdhjklfds;kfds
    ;fsdfsdjkfdskfsd
    fhjfdfjkldslfkdsjklfsd
}

struct test_element_data 
{
     test_element_data();
    ~test_element_data();

    int oranges = (1 << 24);
    int internal_data[MINIMUM_MEMBERS];
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

    void brick_the_tool(char *data = NULL)
    {
        return;
    }
};

typedef struct test_element_typedeffed 
{
    int    *oranges;
    int     internal_data[MAX_INTERNAL_MEMBERS];
    int     internal_array2[MAX_PAREN_INTERNAL_MEMBERS];
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

internal inline void*
test_allocator_lambda(void *allocator = NULL, size_t allocation_size = 1000);

internal inline void*
test_read_file(char *filename = "../test_manager.cpp")
{
    void *file_data = null;
    file_data = fread(filename, "rb"); 

    return(file_data);
}
