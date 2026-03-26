/* =======================================================================
   $File: new_threadpool.cpp $
   $Date: February 08 2026 07:29 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
// TODO(Sleepster): 
// - [X] Work stealing
// - [X] parallel_for should allow threads to instead add work to themselves in parallel
// - [X] Work Order Fences 
// - [X] A manner with which to run work in batches and get the results of that completed work back from the threadpool
#include <SDL3/SDL.h>

#define MATH_IMPLEMENTATION
#include <c_base.h>
#include <c_intrinsics.h>
#include <c_math.h>

#include <c_global_context.cpp>
#include <c_string.cpp>

#include <p_platform_data.h>
#include <c_memory_arena.h>

#include <sys_linux.cpp>
#include <c_memory_arena.cpp>
#include <c_threadpool.cpp>

int
main(void) 
{
    c_global_context_init();

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    threadpool_t *threadpool = (threadpool_t*)AllocSize(Align(sizeof(threadpool_t), 64));

    u32 thread_count = sys_get_thread_count() - 1;
    c_threadpool_init(threadpool, thread_count, MB(200), false);

    work_completion_fence_t matrix_fence = {};
    parallel_for_FIFO(threadpool, work_index, MAX_WORK_ORDERS, &matrix_fence, [=]() {
        vec3_t rotation_vector = vec3(1.0, 0.3, 0.0);
        mat4_t matrix          = mat4_identity();
        mat4_t translation_mat = mat4_make_translation(vec3(100, 100, 0));
        mat4_t rotation_matrix = mat4_rotate(matrix, rotation_vector, 10.8f);

        mat4_t final_matrix = mat4_multiply(matrix, mat4_multiply(translation_mat, rotation_matrix));
        for(u32 mul_index = 0;
            mul_index < 10000;
            ++mul_index)
        {
            final_matrix = mat4_multiply(final_matrix, mat4_multiply(translation_mat, rotation_matrix));
            final_matrix = mat4_scale(final_matrix, vec3(work_index, work_index, work_index));
            (void)final_matrix;
        }

        printf("Finished work at index: '%d'...\n", work_index);
    });

    c_threadpool_start(threadpool);
    u64 last_tsc = SDL_GetPerformanceCounter();
#if 0
    threadpool_flush_work_orders(threadpool);
#else
    c_threadpool_wait_on_fence(threadpool, &matrix_fence);
#endif

    u64 current_tsc = SDL_GetPerformanceCounter();
    u64 delta_tsc   = current_tsc - last_tsc;
    last_tsc        = current_tsc;

    float32 delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    log_info("FINISHED... Time taken: '%.6f' seconds...\n", delta_time);

    return(0);
}
