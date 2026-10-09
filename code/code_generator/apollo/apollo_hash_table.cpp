/* ========================================================================
   $File: apollo_hash_table.cpp $
   $Date: October 08 2026 09:56 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_dynarray.h>

/* NOTE(Sleepster): 
    I normally hate OOP but in this case I have no choice, We'll just have to live with it ig.
    C++ doesn't really allow us to do without it here that easily.
*/

u64
fnv1a_hash(void *data, u32 count)
{
    u64 result = 0;
    local_persist const u64 base_hash = 0xcbf29ce484222325ULL;
    local_persist const u64 FNV_prime = 0x100000001b3ULL;

    // TODO(Sleepster): 4 wide SIMD?? 
    u64 current_hash = base_hash;
    for(u32 key_index = 0;
        key_index < count;
        ++key_index)
    {
        byte key_data = ((byte*)data)[key_index];
        current_hash  = current_hash ^ key_data;
        current_hash  = current_hash * FNV_prime;
    }

    result = current_hash;
    return(result);
}

constexpr u32 MAX_HASH_PEEK_AHEADS = 5;

#define APOLLO_HASH_TABLE_HEAP_ALLOCATE(name) void *(name)(void *allocator, s64 size, void *user_data)
#define APOLLO_HASH_TABLE_HEAP_FREE(name)     void  (name)(void *allocator, void *memory, void *user_data)

typedef APOLLO_HASH_TABLE_HEAP_ALLOCATE(apollo_hash_table_allocate_t);
typedef APOLLO_HASH_TABLE_HEAP_FREE(apollo_hash_table_free_t);

APOLLO_HASH_TABLE_HEAP_ALLOCATE(apollo_hash_table_default_arena_allocator)
{
    (void)user_data;

    void *result = null;
    memory_arena_t *arena = (memory_arena_t*)allocator;

    result = c_arena_push_size(arena, size);

    return(result);
}

// NOTE(Sleepster): Declare this for the key type,
// We're already so deep on the C++ crap, no half measures I suppose
template <typename KeyType>
struct hash_traits_t;

template <>
struct hash_traits_t<string_t>
{
    static u64 
    get_hash_value(const string_t &key)
    {
        u64 result = fnv1a_hash(key.data, key.count);
        return(result);
    }

    static bool8 
    hashes_equal(const string_t &A, const string_t &B)
    {
        bool8 result = false;
        if(A.count == B.count)
        {
            if(memcmp(A.data, B.data, A.count) == 0)
            {
                result = true;
            }
        }

        return(result);
    }
};

template <>
struct hash_traits_t<u64>
{
    static u64 
    get_hash_value(const u64 &key)
    {
        u64 result = 0;
        string_t key_string = {
            .data  = (byte*)&key,
            .count = sizeof(u64)
        };
        result = c_hash_table_hash_key(key_string);

        return(result);
    }

    static bool8 
    hashes_equal(const u64 &A, const u64 &B)
    {
        bool8 result = false;

        u64 keyA = get_hash_value(A);
        u64 keyB = get_hash_value(B);
        if(keyA == keyB)
        {
            result = true;
        }

        return(result);
    }
};

// O(1 * num_buckets)
template <typename ValueType>
struct apollo_hash_element_t
{
    ValueType value;
    u64       raw_hash;
};

template <typename ValueType>
struct apollo_hash_bucket_t
{
    array_t<apollo_hash_element_t<ValueType>> elements;
    apollo_hash_bucket_t<ValueType>          *next_bucket;
};

template <typename                     KeyType, 
          typename                     ValueType,
          s32                          MaxCapacity,
          apollo_hash_table_allocate_t AllocateFn = apollo_hash_table_default_arena_allocator,
          apollo_hash_table_free_t     FreeFn     = (apollo_hash_table_free_t*)null>
struct apollo_hash_table_t
{
    static constexpr s32 bucket_capacity = MaxCapacity;
    apollo_hash_bucket_t<ValueType> first_bucket;
    void *allocator;

    dynarray_t<apollo_hash_element_t<ValueType>*> used_elements;

    // NOTE(Sleepster): FUNCTIONS 
    void
    hash_bucket_init(apollo_hash_bucket_t<ValueType> *bucket)
    {
        bucket->elements.items = (apollo_hash_element_t<ValueType>*)AllocateFn(allocator, sizeof(apollo_hash_element_t<ValueType>) * bucket_capacity, null);
        bucket->elements.count = bucket_capacity;
        bucket->next_bucket = null;
    }

    apollo_hash_bucket_t<ValueType>*
    append_new_bucket(void)
    {
        apollo_hash_bucket_t<ValueType> *result = (apollo_hash_bucket_t<ValueType>*)AllocateFn(allocator, sizeof(apollo_hash_bucket_t<ValueType>), null);
        hash_bucket_init(result);

        apollo_hash_bucket_t<ValueType> *last_bucket = &first_bucket;
        while(last_bucket->next_bucket != null)
        {
            last_bucket = last_bucket->next_bucket;
        }

        last_bucket->next_bucket = result;
        return(result);
    }

    void 
    init(void *passed_allocator)
    {
        allocator = passed_allocator;
        hash_bucket_init(&first_bucket);
    }

    apollo_hash_element_t<ValueType>*
    get_hash_element(KeyType &key)
    {
        apollo_hash_element_t<ValueType> *result = null;
        u64 key_hash = hash_traits_t<KeyType>::get_hash_value(key);

        u64 hash_index = key_hash % bucket_capacity;

        apollo_hash_bucket_t<ValueType> *current_bucket = &first_bucket;
        apollo_hash_element_t<ValueType> *found = &current_bucket->elements[hash_index]; 
        if(found->raw_hash != 0 && found->raw_hash != key_hash)
        {
            while(current_bucket)
            {
                u32 look_ahead = 0;
                while(look_ahead < MAX_HASH_PEEK_AHEADS)
                {
                    found = &current_bucket->elements[hash_index]; 
                    if(found->raw_hash == key_hash)
                    {
                        result = found;
                        goto return_value;
                    }

                    hash_index = (hash_index + 1) % bucket_capacity;
                    ++look_ahead;
                }
                current_bucket = current_bucket->next_bucket;
            }
        }
        else
        {
            result = found;
        }

return_value:
        if(!result)
        {
            current_bucket = append_new_bucket();
            result = &current_bucket->elements[hash_index]; 
        }

        Assert(result);
        return(result);
    }

    ValueType*
    get_element(KeyType &key)
    {
        ValueType *result = null;

        apollo_hash_element_t<ValueType> *element = get_hash_element(key);
        if(element->raw_hash != 0)
        {
            result = &element->value;
        }

        return(result);
    }

    void 
    add_element(ValueType &value, KeyType &key)
    {
        apollo_hash_element_t<ValueType> *element = get_hash_element(key);

        u64 key_hash = hash_traits_t<KeyType>::get_hash_value(key);
        element->raw_hash = key_hash;
        element->value    = value;

        c_dynarray_add(&used_elements, &element);
    }
};
