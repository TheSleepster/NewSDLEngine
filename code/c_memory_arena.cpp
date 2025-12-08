/* ========================================================================
   $File: c_memory_arena.cpp $
   $Date: December 07 2025 03:40 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_memory_arena.h>
#include <string.h>

memory_arena_t
c_arena_create(u64 block_size)
{
    memory_arena_t result = {};

    block_size            = Align16(block_size);
    result.base           = (byte*)malloc(block_size);
    result.used           = 0;
    result.block_size     = block_size;
    result.block_counter += 1;
    result.is_initialized = true;

    return(result);
}

internal inline memory_arena_footer_t*
c_arena_get_footer(memory_arena_t *arena)
{
    memory_arena_footer_t *result;
    result = (memory_arena_footer_t *)((u8*)arena->base + arena->block_size);

    return(result);
}

void*
c_arena_push_size(memory_arena_t *arena, u64 push_size)
{
    Assert(arena->is_initialized);
    void *result = null;

    push_size = Align16(push_size);
    if((arena->used + push_size) >= arena->block_size)
    {
        if(arena->block_size == 0)
        {
            arena->block_size = MB(10);
        }

        memory_arena_footer_t footer;
        footer.last_base       = arena->base;
        footer.last_block_size = arena->block_size;
        footer.last_used       = arena->used;

        push_size += sizeof(memory_arena_footer_t);
        u64 new_block_size = push_size > arena->block_size + sizeof(memory_arena_footer_t) ? push_size : arena->block_size;

        arena->block_size = new_block_size - sizeof(memory_arena_footer_t);
        arena->base       = (byte*)malloc(new_block_size);
        arena->used       = 0;

        arena->block_counter += 1;

        memory_arena_footer_t *arena_footer = c_arena_get_footer(arena); 
        *arena_footer = footer;
    }

    Assert(arena->base != null);
    Assert(arena->used + push_size <= arena->block_size);

    u64 offset_ptr = arena->used + push_size;
    result         = arena->base + offset_ptr;

    arena->used += push_size;
    return(result);
}

byte*
c_arena_bootstrap_allocate_struct_(u32 structure_size, u32 offset_to_arena, u64 block_size)
{
    Assert(structure_size < block_size);
    byte *result = null;

    structure_size = Align16(structure_size);
    memory_arena_t bootstrap = c_arena_create(block_size);
    result                   = (byte*)c_arena_push_size(&bootstrap, structure_size);
    Assert(result);

    *(memory_arena_t*)(result + offset_to_arena) = bootstrap;
    return(result);
}

inline void
c_arena_clear_block(memory_arena_t *arena)
{
    memset(arena->base, 0, arena->used);    
    arena->used = 0;
}

void
c_arena_free_last_block(memory_arena_t *arena)
{
    u8 *block_to_free  = (u8*)arena->base;

    memory_arena_footer_t *footer = c_arena_get_footer(arena);
    arena->base       = footer->last_base;
    arena->used       = footer->last_used;
    arena->block_size = footer->last_block_size;
    arena->block_size = footer->last_block_size;

    free(block_to_free);
    arena->block_counter -= 1;
}

inline void
c_arena_reset(memory_arena_t *arena)
{
    while(arena->block_counter > 1)
    {
        c_arena_free_last_block(arena);
    }
    Assert(arena->base);

    memset(arena->base, 0, arena->used);
    arena->used = 0;
}

inline scratch_arena_t
c_arena_begin_temporary_memeory(memory_arena_t *arena)
{
    scratch_arena_t result;
    result.parent = arena;
    result.used   = arena->used;
    result.base   = (u8*)arena->base + arena->used;

    arena->scratch_arena_count += 1;

    return(result);
}

inline void
c_arena_end_temporary_memory(scratch_arena_t *scratch_arena)
{
    memory_arena_t *parent = scratch_arena->parent;
    Assert(parent->scratch_arena_count > 0);
    while(parent->base != scratch_arena->base)
    {
        c_arena_free_last_block(parent);
    }
    Assert(parent->used >= scratch_arena->used);

    parent->used = scratch_arena->used;
    parent->base = scratch_arena->base;

    parent->scratch_arena_count -= 1;
}
