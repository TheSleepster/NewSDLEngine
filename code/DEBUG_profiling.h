#if !defined(DEBUG_PROFILING_H)
/* ========================================================================
   $File: DEBUG_profiling.h $
   $Date: October 03 2026 11:55 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define DEBUG_PROFILING_H
#include <c_base.h>
#include <c_types.h>
#include <c_synchronization.h>
#include <c_intrinsics.h>
#include <c_heap_allocator.h>

constexpr u32 MAX_FRAME_HISTORY    = 100;
constexpr u32 MAX_TIMERS_PER_FRAME = 100;

struct DEBUG_timed_block_t
{
    u64 begin_cycle_count;
    u64 end_cycle_count;
    u64 delta_cycle_count;

    const char *name;

    u32 coreID;
    u32 timerID;

     DEBUG_timed_block_t(u32 timer_ID, const char *name);
    ~DEBUG_timed_block_t();
};

struct DEBUG_cycle_record_t
{
    u64 total_cycles;
    u64 hit_count;

    const char *name;
};

struct DEBUG_state_t
{
    DEBUG_cycle_record_t record[MAX_TIMERS_PER_FRAME];
    u32                  timer_count;
};

global DEBUG_state_t *debug_state;

DEBUG_timed_block_t::
DEBUG_timed_block_t(u32 timer_index, const char *name)
{
    begin_cycle_count = SDL_GetTicks();
    timerID           = timer_index;
    this->name        = name;
}

DEBUG_timed_block_t::
~DEBUG_timed_block_t()
{
    end_cycle_count = SDL_GetTicks();
    delta_cycle_count = end_cycle_count - begin_cycle_count;

    DEBUG_cycle_record_t *record = &debug_state->record[this->timerID];
    record->total_cycles = this->delta_cycle_count;
    record->hit_count    = 1;
    record->name         = this->name;
}

#define DEBUG_TIMED_BLOCK(name) \
constexpr u32 timer_index = __COUNTER__; \
DEBUG_timed_block_t timed_block##__FUNCTION__ = DEBUG_timed_block_t(timer_index, #name);

internal_api void DEBUG_state_create(void);
void DEBUG_output_record_data(void);

#endif // DEBUG_PROFILING_H

