#if !defined(C_NEW_HASH_TABLE_H)
/* ========================================================================
   $File: c_new_hash_table.h $
   $Date: January 06 2026 06:26 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_NEW_HASH_TABLE_H
#include <c_types.h>
#include <c_base.h>
#include <c_string.h>

#define HASH_TABLE_DEBUG_ID (0xC0FFEEUL)

typedef struct hash_table_header
{
    u32       max_entries;
    u32       data_type_size;
    u32       flags;
    u32       current_entry_count;
    u32       debug_id;
    u32       __reserved0;
    u32       __reserved1;
    u32       __reserved2;
}hash_table_header_t;
StaticAssert(sizeof(hash_table_header_t) % 16 == 0, "Hash table header must be 16 byte aligned...\n");

// NOTE(Sleepster): Allocator functions 
#define C_HASH_TABLE_ALLOCATE_IMPL(name)  void *name(void *allocator, u32 allocation_size, u32 allocation_flags)
typedef C_HASH_TABLE_ALLOCATE_IMPL(c_hash_table_allocate_fn);

#define C_HASH_TABLE_FREE_IMPL(name) void name(void *allocator, void *data)
typedef C_HASH_TABLE_FREE_IMPL(c_hash_table_free_fn);

#ifdef NEW_HASH_TABLE_IMPLEMENTATION
# define HASH_API inline
#else
# define HASH_API extern
#endif // NEW_HASH_TABLE_IMPLEMENTATION
       
HASH_API u64 c_new_hash_table_value_from_key(byte *key, u32 key_size, u32 max_table_entries);

#ifdef NEW_HASH_TABLE_IMPLEMENTATION
/* NOTE(Sleepster):
 *
 * These are here so that we can have some sort of way to override the hash tables's allocator 
 * without worrying about the lack of a default system crippling this library
 */

HASH_API
C_HASH_TABLE_ALLOCATE_IMPL(c_new_hash_table_default_alloc_impl)
{
    void *result = null;
    result = AllocSize(allocation_size);

    return(result);
}

HASH_API
C_HASH_TABLE_FREE_IMPL(c_new_hash_table_default_free_impl)
{
    free(data);
}

HASH_API u64
c_new_hash_table_value_from_key(byte *key, u32 key_size, u32 max_table_entries)
{
    u64 result = 0;

    local_persist const u64 base_hash = 0xcbf29ce484222325ULL;
    local_persist const u64 FNV_prime = 0x100000001b3ULL;
    // TODO(Sleepster): 4 wide SIMD?? 
    u64 current_hash = base_hash;
    for(u32 key_index = 0;
        key_index < key_size;
        ++key_index)
    {
        byte key_data = key[key_index];
        current_hash = current_hash ^ key_data;
        current_hash = current_hash * FNV_prime;
    }

    result = current_hash % max_table_entries;
    return(result);
}
#endif // NEW_HASH_TABLE_IMPLEMENTATION
       

/* NOTE(Sleepster): The default allocator is just malloc / free pairing, 
 * so it doesn't actually care about the allocation flags. 
 * This is a "Bring your own flags" thing for your convienence.
 */

#define HashTable(stored_type, key_type)   \
struct {                                   \
    hash_table_header_t header;            \
    stored_type        *data;              \
    key_type           *keys;              \
                                           \
    void                     *allocator;   \
    c_hash_table_allocate_fn *allocate_fn; \
    c_hash_table_free_fn     *free_fn;     \
} 

#define _GET_SECOND_ARG(A, B, ...) B
#define _GET_THIRD_ARG(A, B, C, ...) C
#define _GET_FOURTH_ARG(A, B, C, D, ...) D

/* NOTE(Sleepster): We shift the getters by one. Since we will pad the calls with a dummy '0' at the start,
 * the "First" user argument is now effectively the "Second" argument to the macro.
 */
#define GET_HASH_ALLOC(...)     _GET_SECOND_ARG(__VA_ARGS__)
#define GET_HASH_ALLOC_FN(...)  _GET_THIRD_ARG(__VA_ARGS__)
#define GET_HASH_FREE_FN(...)   _GET_FOURTH_ARG(__VA_ARGS__)

/* NOTE(Sleepster): We pass '0' as a dummy first argument since the "comma swallowing" of the 
 * preprocessor only works AFTER the ## operator:
 *
 * If __VA_ARGS__ is empty: 0, ##__VA_ARGS__ -> 0 (comma swallowed)
 * If __VA_ARGS__ is full:  0, ##__VA_ARGS__ -> 0, Arg1, ...
 */
#define c_new_hash_table_init(hash_table_ptr, entry_count, ...) do {                                                                                    \
    Expect((hash_table_ptr) != null, "Hash table address is invalid...\n");                                                                             \
    ZeroStruct(*(hash_table_ptr));                                                                                                                      \
    hash_table_header_t *header = &(hash_table_ptr)->header;                                                                                            \
    Expect(header, "Header is invalid...\n");                                                                                                           \
                                                                                                                                                        \
    header->max_entries    = entry_count;                                                                                                               \
    header->data_type_size = sizeof(*((hash_table_ptr)->data));                                                                                         \
    Expect(header->data_type_size > 0,                                                                                                                  \
           "Failure to get the correct size of the hash table's type...\n");                                                                            \
                                                                                                                                                        \
    (hash_table_ptr)->allocator   = GET_HASH_ALLOC(0, ##__VA_ARGS__, NULL);                                                                             \
    (hash_table_ptr)->allocate_fn = GET_HASH_ALLOC_FN(0, ##__VA_ARGS__, c_new_hash_table_default_alloc_impl, NULL);                                     \
    (hash_table_ptr)->free_fn     = GET_HASH_FREE_FN(0, ##__VA_ARGS__, NULL, c_new_hash_table_default_free_impl, NULL);                                 \
    Expect((hash_table_ptr)->allocate_fn, "Hash table alloc function pointer is null...\n");                                                            \
    Expect((hash_table_ptr)->free_fn, "Hash table free function pointer is null...\n");                                                                 \
                                                                                                                                                        \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                                                                                             \
    typedef TypeOf(*((hash_table_ptr)->keys)) key_type_t;                                                                                               \
                                                                                                                                                        \
    (hash_table_ptr)->data = (table_type_t*)(hash_table_ptr)->allocate_fn((hash_table_ptr)->allocator,  sizeof(table_type_t) * header->max_entries, 0); \
    (hash_table_ptr)->keys = (key_type_t *) (hash_table_ptr)->allocate_fn((hash_table_ptr)->allocator,  sizeof(key_type_t)   * header->max_entries, 0); \
                                                                                                                                                        \
    Expect((hash_table_ptr)->data != null, "Data pointer for hash table is invalid...\n");                                                              \
    Expect((hash_table_ptr)->keys != null, "Keys pointer for hash table is invalid...\n");                                                              \
                                                                                                                                                        \
    (hash_table_ptr)->header.debug_id = HASH_TABLE_DEBUG_ID;                                                                                            \
}while(0)

// TODO(Sleepster): Copy key? 
#define c_new_hash_table_insert_pair(hash_table_ptr, value, key) do {                                                   \
    Expect((hash_table_ptr)->header.debug_id == HASH_TABLE_DEBUG_ID,                                                    \
           "Hash table is invalid... the debug_id doesn't match...\n");                                                 \
                                                                                                                        \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                                                             \
    typedef TypeOf(*((hash_table_ptr)->keys)) key_type_t;                                                               \
                                                                                                                        \
    key_type_t   *key_array   = (hash_table_ptr)->keys;                                                                 \
    table_type_t *value_array = (hash_table_ptr)->data;                                                                 \
                                                                                                                        \
    StaticAssert(TypesSame(*value_array, value), "Value types within the table are not the same...\n");                 \
    StaticAssert(TypesSame(*key_array,   key),   "Key types within the table are not the same...\n");                   \
                                                                                                                        \
    u64 index = c_new_hash_table_value_from_key((byte*)&key, sizeof(key_type_t), (hash_table_ptr)->header.max_entries); \
    Assert(index > 0);                                                                                                  \
    key_array[index]   = key;                                                                                           \
    value_array[index] = value;                                                                                         \
}while(0)

#define c_new_hash_table_get_value_ptr(hash_table_ptr, key) ({                                                          \
    Expect((hash_table_ptr)->header.debug_id == HASH_TABLE_DEBUG_ID,                                                    \
           "Hash table is invalid... the debug_id doesn't match...\n");                                                 \
                                                                                                                        \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                                                             \
    typedef TypeOf(*((hash_table_ptr)->keys)) key_type_t;                                                               \
                                                                                                                        \
                                                                                                                        \
    u64 index = c_new_hash_table_value_from_key((byte*)&key, sizeof(key_type_t), (hash_table_ptr)->header.max_entries); \
    Assert(index > 0);                                                                                                  \
                                                                                                                        \
    table_type_t *result = (hash_table_ptr)->data + index;                                                              \
    result;                                                                                                             \
})

#define c_new_hash_table_get_value(hash_table_ptr, key) ({                      \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                     \
    table_type_t result = *c_new_hash_table_get_value_ptr(hash_table_ptr, key); \
                                                                                \
    result;                                                                     \
})

#define c_new_hash_table_clear_keyed_value(hash_table_ptr, key) do {            \
    typedef TypeOf(*((hash_table_ptr)->data)) table_type_t;                     \
    table_type_t *result = c_new_hash_table_get_value_ptr(hash_table_ptr, key); \
    ZeroStruct(*result);                                                        \
}while(0)

#endif // C_NEW_HASH_TABLE_H

