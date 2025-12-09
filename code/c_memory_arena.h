#if !defined(C_MEMORY_ARENA_H)
/* ========================================================================
   $File: c_memory_arena.h $
   $Date: December 07 2025 03:40 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_MEMORY_ARENA_H
#include <c_base.h>
#include <c_types.h>

struct memory_arena_footer_t
{
    byte *last_base;
    u64   last_used;
    u64   last_block_size;
};

struct memory_arena_t
{
    bool32 is_initialized;
    byte  *base;
    u64    used;
    u64    block_size;

    u32    block_counter;
    u32    scratch_arena_count;
};

struct scratch_arena_t
{
    memory_arena_t *parent;
    u8             *base;
    u64             used;
};

/*===========================================
  ============ STANDARD ARENAS  =============
  ===========================================*/
#define c_arena_push_struct(arena, type)                                 (type*)(c_arena_push_size(arena, sizeof(type)))
#define c_arena_push_array(arena, type, count)                           (type*)(c_arena_push_size(arena, sizeof(type) * count))
#define c_arena_bootstrap_allocate_struct(type, member, allocation_size) (type*)(c_arena_bootstrap_allocate_struct_(sizeof(type), IntFromPtr(OffsetOf(type, member)), allocation_size))

memory_arena_t c_arena_create(u64 block_size);
void*          c_arena_push_size(memory_arena_t *arena, u64 push_size);
byte*          c_arena_bootstrap_allocate_struct_(u32 structure_size, u32 offset_to_arena, u64 block_size);
void           c_arena_clear_block(memory_arena_t *arena);
void           c_arena_free_last_block(memory_arena_t *arena);
void           c_arena_reset(memory_arena_t *arena);

/*===========================================
  ============= SCRATCH ARENAS  =============
  ===========================================*/
inline scratch_arena_t c_arena_begin_temporary_memeory(memory_arena_t *arena);
inline void            c_arena_end_temporary_memory(scratch_arena_t *scratch_arena);

#endif // C_MEMORY_ARENA_H

