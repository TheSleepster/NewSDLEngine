#if !defined(C_GLOBALS_H)
/* ========================================================================
   $File: c_globals.h $
   $Date: December 06 2025 09:43 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_GLOBALS_H
#include <c_types.h>
#include <c_base.h>
#include <c_memory_arena.h>

typedef struct vec2 vec2_t;

extern vec2_t g_window_size;
extern bool8 g_running; 

constexpr float32 gcv_tick_rate = 1.0f / 60.0f;

typedef struct global_context
{
    // NOTE(Sleepster): Persistent allocations... Use sparingly... 
    memory_arena_t context_arena;
    // NOTE(Sleepster): Resets with each call to gc_reset_temporary_data() 
    memory_arena_t temporary_arena;

    bool8  running;
}global_context_t;

void gc_setup();
void gc_reset_temporary_data();
void gc_reset_context_arena();

extern global_context_t *global_context;

#endif // C_GLOBALS_H

