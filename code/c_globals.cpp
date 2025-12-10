/* ========================================================================
   $File: c_globals.cpp $
   $Date: December 06 2025 09:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_globals.h>
#include <c_math.h>

vec2_t g_window_size = {};
bool8 g_running      = false;

global_context_t *global_context = nullptr;

void
gc_setup()
{
    global_context = c_arena_bootstrap_allocate_struct(global_context_t, context_arena, MB(100));
    Assert(global_context != null);
    
    global_context->temporary_arena = c_arena_create(MB(200));
}

void
gc_reset_temporary_data()
{
    c_arena_reset(&global_context->temporary_arena);
}

void
gc_reset_context_arena()
{
    c_arena_reset(&global_context->context_arena);
}
