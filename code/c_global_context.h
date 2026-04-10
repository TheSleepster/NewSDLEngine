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
#include <c_threadpool.h>

typedef struct vec2 vec2_t;

struct renderer_state_t;
struct asset_manager_t;

typedef struct input_manager  input_manager_t;

typedef struct global_context
{
    bool8             is_initialized;
    bool8             running;

    threadpool_t      main_threadpool;
    renderer_state_t *renderer_state;
    asset_manager_t  *asset_manager;
    input_manager_t  *input_manager;

    // NOTE(Sleepster): Persistent allocations... Use sparingly... 
    memory_arena_t    context_arena;
    // NOTE(Sleepster): Resets with each call to gc_reset_temporary_data() 
    memory_arena_t    temporary_arena;
}global_context_t;

void c_global_context_init();
void c_global_context_reset_temporary_data();
void c_global_context_reset_context_arena();
void gc_reset_context_arena();

extern global_context_t *global_context;

#endif // C_GLOBALS_H

