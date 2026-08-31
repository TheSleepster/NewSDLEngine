#if !defined(C_HASH_TABLE_H)
/* ========================================================================
   $File: c_hash_table.h $
   $Date: January 06 2026 06:26 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define C_HASH_TABLE_H
#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_dynarray.h>

#ifdef HASH_TABLE_IMPLEMENTATION
#define HASH_API 
#else
#define HASH_API extern
#endif

#ifdef HASH_TABLE_IMPLEMENTATION
HASH_API u64
c_hash_table_hash_key(string_t key)
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

HASH_API u64
c_hash_table_combine_hashes(u64 A, u64 B)
{
    u64 result = A * 31 + B;
    return(result);
}

HASH_API void*
c_hash_table_default_allocate(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    return(result);
}

HASH_API void
c_hash_table_default_free(void *allocator, void *memory)
{
    free(memory);
}
#endif

HASH_API u64   c_hash_table_hash_key(string_t key);
HASH_API u64   c_hash_table_combine_hashes(u64 A, u64 B);
HASH_API void *c_hash_table_default_allocate(void *allocator, u32 allocation_size);
HASH_API void  c_hash_table_default_free(void *allocator, void *memory);

typedef void *hash_table_allocate_impl_t(void *allocator, u32 allocation_size);
typedef void  hash_table_free_impl_t(void *allocator, void *memory);

template <typename T>
struct hash_element_t
{
    T   item;
    u64 raw_key_hash;
};

template <typename T>
struct hash_table_t
{
    // TODO(Sleepster): Make this an array_t? Also, perhaps make this expandable like bucket arrays.
    hash_element_t<T>             *items;

    dynarray_t<hash_element_t<T>*> used_entries;
    u32                            max_entries;

    void                          *allocator;
    hash_table_allocate_impl_t    *allocate_fn;
    hash_table_free_impl_t        *free_fn;

    T &operator[](u32 index);
    T *operator+(u32 index);
};

template <typename T>
hash_table_t<T>
c_hash_table_create(u32                         max_entries, 
                    void                       *allocator = null, 
                    hash_table_allocate_impl_t *allocate  = c_hash_table_default_allocate,
                    hash_table_free_impl_t     *free      = c_hash_table_default_free)
{
    hash_table_t<T> result = {};

    result.max_entries      = max_entries;
    result.allocator        = allocator;
    result.allocate_fn      = allocate;
    result.free_fn          = free;
    result.items            = (hash_element_t<T>*)malloc(sizeof(hash_element_t<T>) * max_entries);

    Assert(result.items);
    memset(result.items, 0, sizeof(hash_element_t<T>) * max_entries);

    return(result);
}

template <typename T> 
void
c_hash_table_destroy(hash_table_t<T> *table)
{
    table->free(table->allocator, table->items);
    dynarray_free(table->used_entries);
}

template <typename T>
hash_element_t<T>*
c_hash_table_get_hash_element(hash_table_t<T> *table, u64 index, u64 key_hash)
{
    hash_element_t<T> *result = null;

    // NOTE(Sleepster): 
    // Check the index, if there is a collision and the raw key_hashes do not match we move 
    // forward 1 index at a time until we find an a hash that is either 0 or matches our key_hash.
    result = table->items + index;
    while(result->raw_key_hash != 0 && key_hash != result->raw_key_hash)
    {
        index = (index + 1) % table->max_entries;
        result = table->items + index;
    }

    return(result);
}

template <typename T>
void
c_hash_table_add_element(hash_table_t<T> *table, T *new_element, string_t key)
{
    Assert(table->items);
    Assert(table->max_entries > 0);

    u64 key_hash = c_hash_table_hash_key(key);
    u64 index    = key_hash % table->max_entries;

    hash_element_t<T> *element = c_hash_table_get_hash_element(table, index, key_hash);
    if(element->raw_key_hash == 0)
    {
        c_dynarray_add(&table->used_entries, &element);
    }

    // NOTE(Sleepster): Copies the element 
    element->item         = *new_element;
    element->raw_key_hash =  key_hash;

}

template <typename T>
T
c_hash_table_get_element(hash_table_t<T> *table, string_t key)
{
    Assert(table->items);
    Assert(table->max_entries > 0);

    T result;

    u64 key_hash = c_hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = c_hash_table_get_hash_element(table, index, key_hash);
    result = element->item;

    return(result);
}

template <typename T>
T*
c_hash_table_get_element_ptr(hash_table_t<T> *table, string_t key)
{
    T *result = null;
    u64 key_hash = c_hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = c_hash_table_get_hash_element(table, index, key_hash);
    result = &element->item;

    return(result);
}

template <typename T>
hash_element_t<T>*
c_hash_table_get_hash_element_block(hash_table_t<T> *table, string_t key)
{
    u64 key_hash = c_hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = c_hash_table_get_hash_element(table, index, key_hash);

    return(element);
}

template <typename T>
T
c_hash_table_get_element_at_index(hash_table_t<T> *table, u64 index)
{
    T element = table->elements[index].item;
    return(element);
}

template <typename T>
T*
c_hash_table_get_element_ptr_at_index(hash_table_t<T> *table, u64 index)
{
    T *element = &table->items[index].item;
    return(element);
}

template <typename T>
void
c_hash_table_clear_element_item(hash_table_t<T> *table, string_t key)
{
    u64 key_hash = c_hash_table_hash_key(key);
    u64 index    = (key_hash % table->max_entries);

    hash_element_t<T> *element = c_hash_table_get_hash_element(table, index, key_hash);
    ZeroStruct(element->item);

    c_dynarray_remove(&table->used_entries, index);
}

template <typename T>
u64
c_hash_table_value_from_key(hash_table_t<T> *table, string_t string)
{
    u64 result = 0;
    result = c_hash_table_hash_key(string);
    result = result % table->max_entries;

    return(result);
}

template <typename T>
T&
hash_table_t<T>::operator[](u32 index)
{
    Expect(index < this->max_entries, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->max_entries);
    return(this->items[index].item);
}

template <typename T>
T*
hash_table_t<T>::operator+(u32 index)
{
    Expect(index < this->max_entries, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->max_entries);
    return(&(this->items + index)->item);
}

#endif // C_HASH_TABLE_H

