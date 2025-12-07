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

#include <stdio.h>

int
main(void)
{
    memory_arena_t arena = c_arena_create(MB(500));

    s32 *first_int = c_arena_push_struct(&arena, s32);
    s32 *next_int  = c_arena_push_struct(&arena, s32);
}
