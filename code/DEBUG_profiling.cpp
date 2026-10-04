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
#include <c_heap_allocator.h>

#include <DEBUG_profiling.h>

internal_api void
DEBUG_state_create(void)
{
    debug_state = (DEBUG_state_t*)c_alloc(sizeof(DEBUG_state_t), ALLOCATOR_TAG_DEBUG);
}

void
DEBUG_output_record_data(void)
{
#if 0
    printf("========== DEBUG RECORDS ===========\n");
    for(u32 record_index = 0;
        record_index < 3;
        ++record_index)
    {
        DEBUG_cycle_record_t *record = debug_state->record + record_index;
        printf("Timer: '%s'...\n", record->name);
        printf("\tTime MS: '%lu'...\n", record->total_cycles);

        record->total_cycles = 0;
        record->hit_count    = 0;
    }
    printf("====================================\n");
#endif
}
