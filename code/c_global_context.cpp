/* ========================================================================
   $File: c_globals.cpp $
   $Date: December 06 2025 09:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_global_context.h>
#include <c_heap_allocator.h>
#include <c_threadpool.h>
#include <c_math.h>

vec2_t g_window_size = {};
bool8 g_running      = false;

ENGINE_API void
c_global_context_init(void)
{
    Assert(!gc);

    void *DEBUG_base_address = (void*)TB(2);
    c_memory_allocator_init(DEBUG_base_address, GB(16));

    gc = (global_context_t*)c_alloc(sizeof(global_context_t), ALLOCATOR_TAG_STATIC);
    gc->persistent_arena = c_arena_create(MB(300), ALLOCATOR_TAG_STATIC);
    gc->temp_arena       = c_arena_create(MB(50),  ALLOCATOR_TAG_STATIC);
    gc->transient_arena  = c_arena_create(MB(200), ALLOCATOR_TAG_STATIC);
    gc->simulation_arena = c_arena_create(MB(200), ALLOCATOR_TAG_ENGINE);
    Assert(gc != null);

    // TODO(Sleepster): why the hell is this an undefined reference????
    gc->is_initialized = true;

    gc->tick_rate    = 1.0 / 60.0;
    gc->tick_rate_ms = gc->tick_rate * 1000;
}

ENGINE_API void
c_global_context_reset_transient_arena(void)
{
    c_arena_reset(&gc->transient_arena);
}

ENGINE_API void
c_global_context_reset_temp_arena(void)
{
    c_arena_reset(&gc->temp_arena);
}

ENGINE_API void
c_global_context_reset_persistent_arena(void)
{
    c_arena_reset(&gc->persistent_arena);
}

ENGINE_API void
c_global_context_reset_simulation_arena(void)
{
    c_arena_reset(&gc->simulation_arena);
}
