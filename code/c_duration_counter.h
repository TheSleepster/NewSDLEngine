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

void  c_duration_counter_init(duration_counter_t *counter, u64 duration_ms, bool8 looped);
bool8 c_duration_counter_advance(duration_counter_t *counter, u64 advance_ms);
true_inline void c_duration_counter_reset(duration_counter_t *counter);

#endif // C_DURATION_COUNTER_H

