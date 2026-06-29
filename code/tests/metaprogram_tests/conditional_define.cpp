#ifdef HASH_TABLE_IMPLEMENTATION
# define HASH_API
#else 
# define HASH_API extern
#endif

#define C_HASH_TABLE_ALLOCATE_IMPL(name)  void *name(void *allocator, u32 allocation_size, u32 allocation_flags)
HASH_API     C_HASH_TABLE_ALLOCATE_IMPL(c_hash_table_default_alloc_impl);

HASH_API void thing()
{
}

HASH_API void other_thing(void *argument)
{
}

//typedef C_HASH_TABLE_ALLOCATE_IMPL(c_hash_table_allocate_fn_t);

//#define C_HASH_TABLE_FREE_IMPL(name) void name(void *allocator, void *data)
//typedef C_HASH_TABLE_FREE_IMPL(c_hash_table_free_fn_t);


//HASH_API     C_HASH_TABLE_FREE_IMPL(c_hash_table_default_free_impl);
