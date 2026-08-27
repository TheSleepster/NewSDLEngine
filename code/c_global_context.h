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
#include <c_file_watcher.h>

typedef struct vec2 vec2_t;

struct RHI_context_t;
struct asset_manager_t;
struct input_manager_t;

typedef struct global_context
{
    bool8             is_initialized;
    bool8             running;
    bool8             should_reload;

    threadpool_t      main_threadpool;
    RHI_context_t    *RHI_context;
    asset_manager_t  *asset_manager;
    input_manager_t  *input_manager;
    file_watcher_t    file_watcher;

    // NOTE(Sleepster): Persistent allocations... Use sparingly... 
    memory_arena_t    context_arena;
    // NOTE(Sleepster): Resets with each call to gc_reset_temporary_data() 
    memory_arena_t    temporary_arena;

    float64           tick_rate;
    float64           tick_rate_ms;
}global_context_t;

void c_global_context_init();
void c_global_context_reset_temporary_data();
void c_global_context_reset_context_arena();
void gc_reset_context_arena();

extern thread_local global_context_t *gc;

#endif // C_GLOBALS_H

