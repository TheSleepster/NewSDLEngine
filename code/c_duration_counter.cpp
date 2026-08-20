/* ========================================================================
   $File: c_duration_counter.cpp $
   $Date: August 12 2026 10:29 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_duration_counter.h>
#include <c_intrinsics.h>

void
c_duration_counter_init(duration_counter_t *counter, u64 duration_ms, bool8 looped)
{
    ZeroStruct(*counter);
    counter->duration_ms = duration_ms;
    counter->looped      = looped;
}

bool8
c_duration_counter_advance(duration_counter_t *counter, u64 advance_ms)
{
    bool8 ended = false;
    counter->current_elapsed += advance_ms;
    if(counter->current_elapsed >= counter->duration_ms)
    {
        ended = true;
        if(counter->looped == true)
        {
            counter->current_elapsed = 0;
        }
    }

    return(ended);
}

true_inline void
c_duration_counter_reset(duration_counter_t *counter)
{
    counter->current_elapsed = 0;
}
