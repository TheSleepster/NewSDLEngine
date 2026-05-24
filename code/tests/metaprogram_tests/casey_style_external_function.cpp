/* ========================================================================
   $File: metaprogram_test_casey_style_external_function.cpp $
   $Date: May 21 2026 10:39 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define internal_api static

#define C_HASH_TABLE_ALLOCATE_IMPL(name)  void *name(void *allocator, u32 allocation_size, u32 allocation_flags)
typedef C_HASH_TABLE_ALLOCATE_IMPL(hash_table_allocate_impl_t);

struct hash_table_data_t
{
    hash_table_allocate_impl_t *function;
};

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(asset_manager_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

