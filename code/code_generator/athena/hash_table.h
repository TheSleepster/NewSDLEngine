#if !defined(HASH_TABLE_H)
/* ========================================================================
   $File: hash_table.h $
   $Date: June 09 2026 11:41 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define HASH_TABLE_H
#include <c_base.h>
#include <c_types.h>
#include <c_string.h>

// EXPERIMENTAL
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

template <typename T>
struct hash_element_t
{
    T   item;
    u64 raw_key_hash;
};

template <typename T>
struct hash_table_t
{
    hash_element_t<T>          *items;
    //dynarray_t<u64>             occupied_indices;
    u32                         max_entries;

    void                       *allocator;
    hash_table_allocate_impl_t *allocate_fn;
    hash_table_free_impl_t     *free_fn;
};

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

u64
hash_table_combine_hashes(u64 A, u64 B)
{
    u64 result = A * 31 + B;
    return(result);
}

template <typename T>
hash_table_t<T>
hash_table_create(u32                         max_entries, 
                  void                       *allocator = null, 
                  hash_table_allocate_impl_t *allocate  = hash_table_default_allocate,
                  hash_table_free_impl_t     *free      = hash_table_default_free)
{
    hash_table_t<T> result = {};

    result.max_entries      = max_entries;
    result.allocator        = allocator;
    result.allocate_fn      = allocate;
    result.free_fn          = free;
    result.items            = (hash_element_t<T>*)result.allocate_fn(result.allocator, sizeof(hash_element_t<T>) * max_entries);

    return(result);
}

template <typename T> 
void
hash_table_destroy(hash_table_t<T> *table)
{
    table->free(table->allocator, table->items);
    table->free(table->allocator, table->occupied_indices);
}

template <typename T>
hash_element_t<T>*
hash_table_get_hash_element(hash_table_t<T> *table, u64 index, u64 key_hash)
{
    hash_element_t<T> *result = null;

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

template <typename T>
void
hash_table_add_element(hash_table_t<T> *table, T new_element, string_t key)
{
    Assert(table->items);
    Assert(table->max_entries > 0);

    u64 key_hash = hash_table_hash_key(key);
    u64 index    = key_hash % table->max_entries;

    hash_element_t<T> *element = hash_table_get_hash_element(table, index, key_hash);

    // NOTE(Sleepster): Copies the element 
    element->item         = new_element;
    element->raw_key_hash = key_hash;

    //dynarray_add(&table->occupied_indices, index);
}

template <typename T>
T
hash_table_get_element(hash_table_t<T> *table, string_t key)
{
    Assert(table->items);
    Assert(table->max_entries > 0);

    T result;

    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = hash_table_get_hash_element(table, index, key_hash);
    result = element->item;

    return(result);
}

template <typename T>
T*
hash_table_get_element_ptr(hash_table_t<T> *table, string_t key)
{
    T *result = null;
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = hash_table_get_hash_element(table, index, key_hash);
    result = &element->item;

    return(result);
}

template <typename T>
hash_element_t<T>*
hash_table_get_hash_element_block(hash_table_t<T> *table, string_t key)
{
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = hash_table_get_hash_element(table, index, key_hash);

    return(element);
}

template <typename T>
T
hash_table_get_element_at_index(hash_table_t<T> *table, u64 index)
{
    T element = table->elements[index].item;
    return(element);
}

template <typename T>
T*
hash_table_get_element_ptr_at_index(hash_table_t<T> *table, u64 index)
{
    T *element = &table->items[index].item;
    return(element);
}

template <typename T>
void
hash_table_clear_element_item(hash_table_t<T> *table, string_t key)
{
    u64 key_hash = hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = hash_table_get_hash_element(table, index, key_hash);
    ZeroStruct(element->item);
}
// EXPERIMENTAL


#endif // HASH_TABLE_H

