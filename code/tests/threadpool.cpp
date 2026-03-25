/* ========================================================================
   $File: threadpool.cpp $
   $Date: December 04 2025 05:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define MATH_IMPLEMENTATION

#include <c_intrinsics.h>
#include <c_base.h>
#include <c_types.h>
#include <c_math.h>

#include <c_threadpool.h>
#include <c_threadpool.cpp>

#include <stdlib.h>
#include <stdio.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_global_context.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

struct test_data
{
    threadpool_t *threadpool;
    u32           thread_id;
    u32           parent_id;
    bool8         jobs_produced;
};

void 
test_callback(void *user_data)
{
    test_data *data = (test_data*)user_data;
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
        final_matrix = mat4_scale(final_matrix, vec3(mul_index, mul_index, mul_index));
        (void)final_matrix;
    }

    printf("Finished work at index: '%d'...\n", data->thread_id);
}

int
main(void)
{
    u64 perf_count_freq = SDL_GetPerformanceFrequency();

    threadpool_t *threadpool = Alloc(threadpool_t);
    c_threadpool_init(threadpool);

    for(u32 index = 0;
        index < 100000;
        ++index)
    {
        for(u32 job_index = 0;
            job_index < 10000;
            ++job_index)
        {
            test_data *data = (test_data*)malloc(sizeof(test_data));
            data->thread_id     = job_index;
            data->threadpool    = threadpool;
            data->jobs_produced = false;
            data->parent_id     = job_index;
            c_threadpool_add_task(threadpool, data, &test_callback, TPTP_High);
        }
        u64 last_tsc = SDL_GetPerformanceCounter();
        c_threadpool_flush_task_queues(threadpool);

        u64 current_tsc = SDL_GetPerformanceCounter();
        u64 delta_tsc   = current_tsc - last_tsc;
        last_tsc        = current_tsc;

        float32 delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
        log_info("FINISHED... Time taken: '%.2f' seconds...\n", delta_time);
    }

    return(0);
}
