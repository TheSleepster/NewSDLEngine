/* ========================================================================
   $File: c_hash_table.cpp $
   $Date: December 08 2025 08:09 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_hash_table.h>

u64 
c_fnv_hash_value(u8 *data, usize element_size, u64 hash_value)
{
    u64 fnv_prime  = 1099511628211ULL;
    u64 new_value  = hash_value;
        
    for(u32 byte_index = 0;
        byte_index < element_size;
        ++byte_index)
    {
        new_value = (new_value ^ data[byte_index]) * fnv_prime;
    }

    return(new_value);
}

hash_table_t
c_hash_table_create_ma(memory_arena_t *arena, u32 max_entries, usize value_size)
{
    hash_table_t result;
    
    void *memory = c_arena_push_size(arena, max_entries * value_size);
    result       = c_hash_table_create_(memory, max_entries, value_size);

    return(result);
}

hash_table_t
c_hash_table_create_za(zone_allocator_t *zone, u32 max_entries, usize value_size, za_allocation_tag_t tag)
{
    hash_table_t result;
    
    void *memory = c_za_alloc(zone, max_entries * value_size, tag);
    result       = c_hash_table_create_(memory, max_entries, value_size);

    return(result);
}

inline hash_table_t
c_hash_table_create_(void *memory, u32 max_entries, usize value_size)
{
    hash_table_t result;
    result.entries       = (hash_table_entry_t *)memory;
    result.max_entries   = max_entries;
    result.value_size    = value_size;
    result.entry_counter = 0;

    return(result);
}

inline u64
c_hash_create_key_index(hash_table_t *table, void *key, usize key_size)
{
    u64 result = 0;
    u64 new_hash_value = c_fnv_hash_value((u8*)key, key_size, default_fnv_hash_value);

    result = (new_hash_value % table->max_entries + table->max_entries) % table->max_entries;
    return(result);
}

void
c_hash_insert_kv_pair_(hash_table_t *table, string_t key, void *value, usize value_size)
{
    u64 hash_index = c_hash_create_key_index(table, key.data, key.count);
    Assert(hash_index >= 0);

    hash_table_entry_t *entry = &table->entries[hash_index];
    Assert(entry);
    
    if(entry->key.data !=  null)
    {
        if(memcmp(entry->key.data, key.data, key.count) == 0)
        {
            entry->value = value;
            log_warning("hash table value at index: '%d' has been updated...\n");

            return;
        }
    }
    entry->key   = key;
    entry->value = value;

    table->entry_counter += 1;
}

void*
c_hash_get_value(hash_table_t *table, string_t key)
{
    void *result = null;
    
    u64 hash_index = c_hash_create_key_index(table, key.data, key.count);
    Assert(hash_index >= 0);

    hash_table_entry_t *entry = &table->entries[hash_index];
    if(entry->key.data != null)
    {
        if(memcmp(entry->key.data, key.data, key.count) == 0)
        {
            result = entry->value;
        }
    }
    return(result);
}

void*
c_hash_get_value_from_raw_index(hash_table_t *table, s32 index)
{
    hash_table_entry_t *entry = &table->entries[index];

    return(entry->value);
}

void
c_hash_clear_value_at_index(hash_table_t *table, s32 index)
{
    Assert(index >= 0);
    Assert(table->max_entries >= (u32)index);

    hash_table_entry_t *entry = &table->entries[index];
    entry->value = null;
}

void
c_hash_clear_index(hash_table_t *table, s32 index)
{
    Assert(index >= 0);
    Assert(table->max_entries >= (u32)index);
    
    hash_table_entry_t *entry = &table->entries[index];
    entry->value = null;
    entry->key   = STR("");
}

void
c_hash_clear_table_entries(hash_table_t *table)
{
    for(u32 hash_index = 0;
        hash_index < table->max_entries;
        ++hash_index)
    {
        hash_table_entry_t *entry = &table->entries[hash_index];
        entry->value = null;
        entry->key   = STR("");
    }
}
