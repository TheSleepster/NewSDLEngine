/* ========================================================================
   $File: c_globals.cpp $
   $Date: December 06 2025 09:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_global_context.h>
#include <c_threadpool.h>
#include <c_math.h>

vec2_t g_window_size = {};
bool8 g_running      = false;

void
c_global_context_init()
{
    Assert(!gc);

    gc = c_arena_bootstrap_allocate_struct(global_context_t, persistent_arena, MB(100));
    gc->transient_arena  = c_arena_create(MB(200));
    gc->simulation_arena = c_arena_create(MB(200));
    Assert(gc != null);

    // TODO(Sleepster): why the hell is this an undefined reference????
    gc->is_initialized = true;

    gc->tick_rate    = 1.0 / 60.0;
    gc->tick_rate_ms = gc->tick_rate * 1000;
}

void
c_global_context_reset_transient_arena()
{
    c_arena_reset(&gc->transient_arena);
}

void
c_global_context_reset_persistent_arena()
{
    c_arena_reset(&gc->persistent_arena);
}

void
c_global_context_reset_simulation_arena()
{
    c_arena_reset(&gc->simulation_arena);
}
