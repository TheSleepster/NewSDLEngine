/* ========================================================================
   $File: memory_arena.cpp $
   $Date: December 07 2025 03:34 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_math.h>

#include <c_memory_arena.h>
#include <c_memory_arena.cpp>

struct really_big_thing_t 
{
    u32            array[1000];
    char          *other_thing;
    memory_arena_t thing_arena;
};

int
main(void)
{
    memory_arena_t arena = c_arena_create(MB(500));

    for(u32 index = 0;
        index < 1000;
        ++index)
    {
        Assert(arena.block_counter == 1);

        scratch_arena_t scratch = c_arena_begin_temporary_memeory(&arena);
        void *data = c_arena_push_size(&arena, MB(400));
        (void)data;
        c_arena_end_temporary_memory(&scratch);
    }

    really_big_thing_t *big_thing = c_arena_bootstrap_allocate_struct(really_big_thing_t, thing_arena, MB(800));
    (void)big_thing;

    return(0);
}
