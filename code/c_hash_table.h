#if !defined(C_HASH_TABLE_H)
/* ========================================================================
   $File: c_hash_table.h $
   $Date: December 08 2025 08:08 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_HASH_TABLE_H
#include <c_types.h>
#include <c_string.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>


typedef struct zone_allocator zone_allocator_t;

// NOTE(Sleepster): The key is simply a string because at the end of the day a string is just a byte array anyway... 
typedef struct hash_table_entry
{
    string_t key;
    void    *value;
}hash_table_entry_t;

typedef struct hash_table
{
    hash_table_entry_t *entries;
    u32                 max_entries;

    usize               value_size;
    u32                 entry_counter;
}hash_table_t;

////////////////////
// API DEFINITIONS
////////////////////
const u64 default_fnv_hash_value = 14695981039346656037ULL;

       u64          c_fnv_hash_value(u8 *data, usize element_size, u64 hash_value);
       hash_table_t c_hash_table_create_ma(memory_arena_t *arena, u32 max_entries, usize value_size);
       hash_table_t c_hash_table_create_za(zone_allocator_t *zone, u32 max_entries, usize value_size, za_allocation_tag_t tag);
inline hash_table_t c_hash_table_create_(void *memory, u32 max_entries, usize value_size);
       void         c_hash_insert_kv_pair_(hash_table_t *table, string_t key, void *value, usize value_size);
       void*        c_hash_get_value(hash_table_t *table, string_t key);

// NOTE(Sleepster): This feels stupid...
       void*        c_hash_get_value_from_raw_index(hash_table_t *table, s32 index);

#define c_hash_table_create(memory, max_entries, data_type) c_hash_table_create_(memory, max_entries, sizeof(data_type))
#define c_hash_insert_kv_pair(table, key, value)            c_hash_insert_kv_pair_(table, key, (void*)value, sizeof(value))

#endif // C_HASH_TABLE_H

