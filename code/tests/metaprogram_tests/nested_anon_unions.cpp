#define c_hash_table_get_value_ptr(hash_table_ptr, key) ({                                                  \
    Expect((hash_table_ptr)->header.debug_id == HASH_TABLE_DEBUG_ID,                                        \
           "Hash table is invalid... the debug_id doesn't match...\n");                                     \
                                                                                                            \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                                                 \
                                                                                                            \
    u64 index = c_hash_table_value_from_key((key).data, (key).count, (hash_table_ptr)->header.max_entries); \
    Assert(index > 0);                                                                                      \
                                                                                                            \
    table_type_t *result = (hash_table_ptr)->data + index;                                                  \
                                                                                                            \
    result;                                                                                                 \
})

#define c_hash_table_init(hash_table_ptr, entry_count, ...) do {                                                                                                    \
    Expect((hash_table_ptr) != null, "Hash table address is invalid...\n");                                                                                         \
    ZeroStruct(*(hash_table_ptr));                                                                                                                                  \
    hash_table_header_t *header = &(hash_table_ptr)->header;                                                                                                        \
    Expect(header, "Header is invalid...\n");                                                                                                                       \
                                                                                                                                                                    \
    header->max_entries = entry_count;                                                                                                                              \
                                                                                                                                                                    \
    (hash_table_ptr)->allocator   = GET_HASH_ALLOC(0, ##__VA_ARGS__, null);                                                                                         \
    (hash_table_ptr)->allocate_fn = GET_HASH_ALLOC_FN(0, ##__VA_ARGS__,                                                                                             \
                                                      c_hash_table_default_alloc_impl,                                                                              \
                                                      c_hash_table_default_alloc_impl);                                                                             \
                                                                                                                                                                    \
    (hash_table_ptr)->free_fn = GET_HASH_FREE_FN(0, ##__VA_ARGS__,                                                                                                  \
                                                 c_hash_table_default_free_impl,                                                                                    \
                                                 c_hash_table_default_free_impl,                                                                                    \
                                                 c_hash_table_default_free_impl);                                                                                   \
    Expect((hash_table_ptr)->allocate_fn, "Hash table alloc function pointer is null...\n");                                                                        \
                                                                                                                                                                    \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                                                                                                         \
    typedef TypeOf(*((hash_table_ptr)->keys)) key_type_t;                                                                                                           \
                                                                                                                                                                    \
    (hash_table_ptr)->data   = (table_type_t*)(hash_table_ptr)->allocate_fn((hash_table_ptr)->allocator,  sizeof(table_type_t) * header->max_entries, HTAF_Static); \
    (hash_table_ptr)->keys   = (key_type_t *) (hash_table_ptr)->allocate_fn((hash_table_ptr)->allocator,  sizeof(key_type_t)   * header->max_entries, HTAF_Static); \
    (hash_table_ptr)->hashes = (u64*)         (hash_table_ptr)->allocate_fn((hash_table_ptr)->allocator,  sizeof(u64)          * header->max_entries, HTAF_Static); \
                                                                                                                                                                    \
    Expect((hash_table_ptr)->data != null, "Data pointer for hash table is invalid...\n");                                                                          \
    Expect((hash_table_ptr)->keys != null, "Keys pointer for hash table is invalid...\n");                                                                          \
                                                                                                                                                                    \
    (hash_table_ptr)->header.debug_id = HASH_TABLE_DEBUG_ID;                                                                                                        \
}while(0)

static const char *g_device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const int *g_input_stance[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const string_t *g_members[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

typedef struct input_controller
{
    bool8             is_valid;
    bool8             is_analog;
    controller_type_t type;
    union {
        keyboard_controller_data_t keyboard;
        gamepad_controller_data_t  gamepad;
    };
}input_controller_t;

typedef struct test_structure {
    union {
        int internal_to_union;

        struct {
            int internal_to_structure_inside_union;
        };

        struct {
            int other_internal_to_structure_inside_union[2];
        };
    };
}test_structure_t;
