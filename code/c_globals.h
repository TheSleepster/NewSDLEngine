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
#include <c_math.h>
#include <c_string.h>

extern vec2_t g_window_size;
extern bool8 g_running; 

constexpr float32 gcv_tick_rate = 1.0f / 60.0f;

#endif // C_GLOBALS_H

