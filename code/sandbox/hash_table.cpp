/* ========================================================================
   $File: hash_table.cpp $
   $Date: June 08 2026 12:17 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <stdlib.h>

typedef void *hash_table_allocate_impl_t(void *allocator, u32 allocation_size);
typedef void  hash_table_free_impl_t(void *allocator, void *memory);

void*
hash_table_default_allocate(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);

    return(result);
}

void
hash_table_default_free(void *allocator, void *memory)
{
    free(memory);
}

template <typename Type>
struct hash_element_t
{
    Type     item;
    u64      raw_key_hash;
};

template <typename Type>
struct hash_table_t
{
    hash_element_t<Type>       *items;
    u32                        *occupied_indices;
    u32                         max_entries;

    void                       *allocator;
    hash_table_allocate_impl_t *allocate_fn;
    hash_table_free_impl_t     *free_fn;
};

template <typename Type>
hash_table_t<Type>
hash_table_create(u32                         max_entries, 
                  void                       *allocator = null, 
                  hash_table_allocate_impl_t *allocate  = hash_table_default_allocate,
                  hash_table_free_impl_t     *free      = hash_table_default_free)
{
    hash_table_t<Type> result = {};

    result.max_entries      = max_entries;
    result.allocator        = allocator;
    result.allocate_fn      = allocate;
    result.free_fn          = free;
    result.items            = (hash_element_t<Type>*)result.allocate_fn(result.allocator, sizeof(hash_element_t<Type>) * max_entries);
    result.occupied_indices = (u32*)result.allocate_fn(result.allocator, sizeof(u32) * max_entries);

    return(result);
}

template <typename Type> 
void
hash_table_destroy(hash_table_t<Type> *table)
{
    free(table->items);
    free(table->occupied_indices);
}

u64
hash_table_hash_key(string_t key)
{
    u64 result = 0;

    local_persist const u64 base_hash = 0xcbf29ce484222325ULL;
    local_persist const u64 FNV_prime = 0x100000001b3ULL;

    // TODO(Sleepster): 4 wide SIMD?? 
    u64 current_hash = base_hash;
    for(u32 key_index = 0;
        key_index < key.count;
        ++key_index)
    {
        byte key_data = key.data[key_index];
        current_hash  = current_hash ^ key_data;
        current_hash  = current_hash * FNV_prime;
    }

    result = current_hash;
    return(result);
}

template <typename Type>
hash_element_t<Type>*
hash_table_get_hash_element(hash_table_t<Type> *table, u64 index, u64 key_hash)
{
    hash_element_t<Type> *result = null;

    // NOTE(Sleepster): 
    // Check the index, if there is a collision and the raw key_hashes do not match we move 
    // forward 1 index at a time until we find an a hash that is either 0 or matches our key_hash.
    result = table->items + index;
    while(result->raw_key_hash != 0 && result->raw_key_hash != key_hash)
    {
        index = (index + 1) % table->max_entries;
        result = table->items + index;
    }

    return(result);
}

template <typename Type>
void
hash_table_add_element(hash_table_t<Type> *table, Type *new_element, string_t key)
{
    Assert(table->items);
    Assert(table->occupied_indices);
    Assert(table->max_entries > 0);

    u64 key_hash = hash_table_hash_key(key);
    u64 index    = key_hash % table->max_entries;

    hash_element_t<Type> *element = hash_table_get_hash_element(table, index, key_hash);

    // NOTE(Sleepster): Copies the element 
    element->item         = *new_element;
    element->raw_key_hash =  key_hash;

    // TODO(Sleepster): add the index to the occupied_indices array of the hash table...
}

template <typename Type>
Type
hash_table_get_element(hash_table_t<Type> *table, string_t key)
{
    Assert(table->items);
    Assert(table->occupied_indices);
    Assert(table->max_entries > 0);

    Type result;

    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<Type> *element = hash_table_get_hash_element(table, index, key_hash);
    result = element->item;

    return(result);
}

template <typename Type>
Type*
hash_table_get_element_ptr(hash_table_t<Type> *table, string_t key)
{
    Type *result = null;
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<Type> *element = hash_table_get_hash_element(table, index, key_hash);
    result = &element->item;

    return(result);
}

template <typename Type>
hash_element_t<Type>*
hash_table_get_hash_element_block(hash_table_t<Type> *table, string_t key)
{
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<Type> *element = hash_table_get_hash_element(table, index, key_hash);

    return(element);
}

template <typename Type>
void
hash_table_clear_element_item(hash_table_t<Type> *table, string_t key)
{
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<Type> *element = hash_table_get_hash_element(table, index, key_hash);
    ZeroStruct(element->item);
}

int
main(void)
{
    hash_table_t<u32> table = hash_table_create<u32>(4096);

    u32 test_element = 938;
    hash_table_add_element(&table, &test_element, STR("double"));

    u32 new_element = hash_table_get_element(&table, STR("hash_table_data_t"));
    Assert(new_element != test_element);

    printf("test_element: '%d'...\n", test_element);
    printf("new_element: '%d'...\n", new_element);
}
