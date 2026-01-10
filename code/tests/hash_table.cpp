/* ========================================================================
   $File: new_hash_table.cpp $
   $Date: January 06 2026 06:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>

#define HASH_TABLE_IMPLEMENTATION
#include <c_hash_table.h>

#include <c_dynarray.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

struct thing
{
    u32 ID;
    u32 item;
    u32 value0;
    u32 value1;
    u32 value2;
    u32 value3;
};

C_HASH_TABLE_ALLOCATE_IMPL(memory_arena_hash_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

C_HASH_TABLE_FREE_IMPL(memory_arena_hash_free)
{
    return;
}

int
main()
{
    HashTable_t(thing) table;
    c_hash_table_init(&table, 4096);

    string_t test0 = STR("Egg");
    string_t test1 = STR("Egg");
    string_t test2 = STR("Ain't nobody got time for that");

    u64 value0 = c_hash_table_value_from_key(test0.data, test0.count, table.header.max_entries);
    u64 value1 = c_hash_table_value_from_key(test1.data, test1.count, table.header.max_entries);
    u64 value2 = c_hash_table_value_from_key(test1.data, test1.count, table.header.max_entries);

    Assert(value0 > 0);
    Assert(value1 > 0);
    Assert(value2 > 0);

    log_info("Value 0 from key: '%s' is '%llu'...\n", C_STR(test0), value0);
    log_info("Value 1 from key: '%s' is '%llu'...\n", C_STR(test1), value1);
    log_info("Value 2 from key: '%s' is '%llu'...\n", C_STR(test2), value2);

    thing test_thing = {
        1, 2, 3, 4, 5 
    };

    c_hash_table_insert_pair(&table, test0, test_thing);
    thing *other_thing      = c_hash_table_get_value_ptr(&table, test0);
    thing  copy_other_thing = c_hash_table_get_value(&table, test0);

    (void)other_thing;
    (void)copy_other_thing;

    c_hash_table_clear_keyed_value(&table, test0);

    memory_arena_t arena = c_arena_create(MB(50));
    HashTable_t(thing) arena_hash;
    c_hash_table_init(&arena_hash, 4096, &arena, memory_arena_hash_allocate, null);

    getchar();
}
