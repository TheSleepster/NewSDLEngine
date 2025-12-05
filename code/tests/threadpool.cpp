/* ========================================================================
   $File: threadpool.cpp $
   $Date: December 04 2025 05:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_intrinsics.h>
#include <c_base.h>
#include <c_types.h>

#include <c_threadpool.h>
#include <c_threadpool.cpp>

#include <stdlib.h>
#include <stdio.h>

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
    printf("thread: '%d', parent_id: '%d'\n", data->thread_id, data->parent_id);

    if(!data->jobs_produced)
    {
        for(u32 thread_index = 0;
            thread_index < 12;
            ++thread_index)
        {
            test_data *new_data = (test_data*)malloc(sizeof(test_data));
            new_data->thread_id     = thread_index + data->thread_id;
            new_data->parent_id     = data->thread_id;
            new_data->threadpool    = data->threadpool;
            new_data->jobs_produced = true;
            c_threadpool_add_task(new_data->threadpool, new_data, &test_callback, TPTP_High);
        }
    }
}

int
main(void)
{
    threadpool_t *threadpool = Alloc(threadpool_t);
    c_threadpool_init(threadpool);

    for(u32 thread_index = 0;
        thread_index < 12;
        ++thread_index)
    {
        test_data *data = (test_data*)malloc(sizeof(test_data));
        printf("alloced: '%d'...\n", thread_index);
        data->thread_id     = thread_index;
        data->threadpool    = threadpool;
        data->jobs_produced = false;
        data->parent_id     = thread_index;
        c_threadpool_add_task(threadpool, data, &test_callback, TPTP_High);
    }
    c_threadpool_flush_task_queues(threadpool);

    return(0);
}
