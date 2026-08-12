#if !defined(C_DURATION_COUNTER_H)
/* ========================================================================
   $File: c_duration_counter.h $
   $Date: August 12 2026 10:29 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_DURATION_COUNTER_H
#include <string.h>

#include <c_base.h>
#include <c_types.h>

// NOTE(Sleepster): Can't use 'timer_t' because glibc... 
struct duration_counter_t
{
    u64    duration_ms;
    u64    current_elapsed;
    bool32 looped;
};

#endif // C_DURATION_COUNTER_H

