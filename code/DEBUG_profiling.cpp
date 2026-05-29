/* ========================================================================
   $File: DEBUG_profiling.cpp $
   $Date: May 29 2026 01:18 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_synchronization.h>
#include <c_intrinsics.h>

constexpr u32 MAX_FRAME_HISTORY    = 576;
constexpr u32 MAX_TIMERS_PER_FRAME = 100;

struct DEBUG_timed_block
{
    u64 begin_cycle_count;
    u64 end_cycle_count;
    u64 delta_cycle_count;

    u32 core_ID;
    u32 timer_ID;

     DEBUG_timed_block(u32 timer_ID);
    ~DEBUG_timed_block();
};

DEBUG_timed_block::DEBUG_timed_block(u32 timer_index)
{
    begin_cycle_count = rdtscp(&core_ID);
    timer_ID          = timer_index;
}

DEBUG_timed_block::~DEBUG_timed_block()
{
    end_cycle_count   = rdtsc();
    delta_cycle_count = AtomicSubtract32(&end_cycle_count, begin_cycle_count);
}

struct DEBUG_state_t
{
    DEBUG_timed_block timers[MAX_FRAME_HISTORY][MAX_TIMERS_PER_FRAME];
    u32               current_frame_index;
    u32               timer_count;
};
