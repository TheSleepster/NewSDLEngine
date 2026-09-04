/* ========================================================================
   $File: c_threadpool.cpp $
   $Date: March 26 2026 04:40 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
// TODO(Sleepster): 
// - [X] Work stealing
// - [X] parallel_for should allow threads to instead add work to themselves in parallel
// - [X] Work Order Fences 
// - [X] A manner with which to run work in batches and get the results of that completed work back from the threadpool
#include <SDL3/SDL.h>

#include <c_base.h>
#include <c_global_context.h>
#include <c_intrinsics.h>
#include <c_math.h>

#include <p_platform_data.h>

#include <c_memory_arena.h>
#include <c_threadpool.h>

internal_api bool8 c_thread_steal_work_order(worker_thread_t *theif_thread);
PLATFORM_THREAD_PROC(thread_proc_entry);

/*
=============
c_threadpool_init
=============
*/

void
c_threadpool_init(threadpool_t *threadpool, u32 max_threads, u32 thread_allocator_size, bool8 start_instantly, bool8 allow_stealing)
{
    ZeroStruct(*threadpool);
    threadpool->work_avaliable_semaphore = sys_semaphore_create(0, max_threads);
    threadpool->allow_stealing           = allow_stealing;

    for(u32 thread_index = 0;
        thread_index < max_threads;
        ++thread_index)
    {
        worker_thread_t *thread = threadpool->workers + thread_index;
        ZeroStruct(*thread);

        thread_allocator_t *allocator = &thread->allocator;
        allocator->buffer  = (byte*)sys_allocate_memory(null, thread_allocator_size);
        allocator->size    = thread_allocator_size;

        thread->is_started = false;
        thread->threadpool = threadpool;
        thread->thread_id  = thread_index;

        // TODO(Sleepster): Windows might complain about thread_proc_entry returning an 'int' and not a 'DWORD'... But we'll see. 
        thread->handle = sys_thread_create(thread_proc_entry, thread, true);

        WriteBarrier;
        AtomicIncrement(&threadpool->thread_count);
    }

    if(start_instantly)
    {
        c_threadpool_start(threadpool);
    }
}

/*
=============
c_threadpool_start
=============
*/

void
c_threadpool_start(threadpool_t *threadpool)
{
    for(s32 thread_index = 0;
        thread_index < threadpool->thread_count;
        ++thread_index)
    {
        worker_thread_t *thread = threadpool->workers + thread_index;
        AtomicStore32(&thread->is_started, 1);
    }

    sys_semaphore_release(&threadpool->work_avaliable_semaphore, threadpool->thread_count);
}

/*
=============
c_threadpool_flush_work_orders
=============
*/

void
c_threadpool_flush_work_orders(threadpool_t *threadpool)
{
    worker_thread_t this_thread = {};
    this_thread.is_started = true;
    this_thread.threadpool = threadpool;
    while(threadpool->threads_flushed != threadpool->thread_count)
    {
        if(threadpool->allow_stealing)
        {
            c_thread_steal_work_order(&this_thread);
        }
    }
}

/*
=============
c_threadpool_wait_on_fence
=============
*/

void
c_threadpool_wait_on_fence(threadpool_t *threadpool, work_completion_fence_t *fence)
{
    worker_thread_t this_thread = {};
    this_thread.is_started = true;
    this_thread.threadpool = threadpool;
    while(fence->pending > 0)
    {
        if(threadpool->allow_stealing)
        {
            c_thread_steal_work_order(&this_thread);
        }
    }
}

/*
=============
c_threadpool_pop_work_order
=============
*/

internal_api bool8 
thread_pop_work_order(worker_thread_t *thread)
{
    bool8 result = true;

    u32 current_head = AtomicLoad(&thread->work_avaliable.head);
    u32 current_tail = AtomicLoad(&thread->work_avaliable.tail);

    if(current_head != current_tail)
    {
        u32 last_head = (current_head - 1) % MAX_WORK_ORDERS;
        u32 next_entry_todo = AtomicCompareExchange32(&thread->work_avaliable.head,
                                                      last_head,
                                                      current_head);
        if(next_entry_todo == current_head)
        {
            work_order_t *work_order = thread->work_avaliable.work_orders + last_head;
            work_order->function(work_order->data);
            if(work_order->fence)
            {
                AtomicDecrement(&work_order->fence->pending);
            }

            ReadWriteBarrier;
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

/*
=============
c_threadpool_steal_work_order
=============
*/

internal_api bool8
c_thread_steal_work_order(worker_thread_t *theif_thread)
{
    bool8 result = true;

    u32 target_thread_index      = 0;
    u32 highest_work_order_count = 0;

    u32 current_thread_count = (u32)(AtomicLoad32(&theif_thread->threadpool->thread_count));
    for(u32 thread_index = 0;
        thread_index < current_thread_count;
        ++thread_index)
    {
        if(thread_index == theif_thread->thread_id) continue;

        worker_thread_t *target_thread = theif_thread->threadpool->workers + thread_index;

        u32 current_head = AtomicLoad32(&target_thread->work_avaliable.head);
        u32 current_tail = AtomicLoad32(&target_thread->work_avaliable.tail);

        u32 work_orders_left = Max(current_head - current_tail, 0); 
        if(work_orders_left > highest_work_order_count)
        {
            highest_work_order_count = work_orders_left;
            target_thread_index      = thread_index;
        }
    }
    worker_thread_t *target_thread = theif_thread->threadpool->workers + target_thread_index;
    Assert(target_thread);

    u32 tail_index = AtomicLoad(&target_thread->work_avaliable.tail);
    u32 head_index = AtomicLoad(&target_thread->work_avaliable.head);
    if(tail_index != head_index)
    {
        u32 next_tail_index = (tail_index + 1) % MAX_WORK_ORDERS;
        u32 next_entry_todo = AtomicCompareExchange32(&target_thread->work_avaliable.tail, 
                                                      next_tail_index,
                                                      tail_index);
        if(next_entry_todo == tail_index)
        {
            work_order_t *work_order = target_thread->work_avaliable.work_orders + next_entry_todo;
            work_order->function(work_order->data);
            if(work_order->fence)
            {
                AtomicDecrement(&work_order->fence->pending);
            }

            ReadWriteBarrier;
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

/*
=============
thread_proc_entry
=============
*/

PLATFORM_THREAD_PROC(thread_proc_entry)
{
    worker_thread_t *thread = (worker_thread_t *)user_data;
    for(;;)
    {
        bool32 is_started = AtomicLoad32(&thread->is_started);
        if(is_started)
        {
            if(!thread_pop_work_order(thread))
            {
                if(thread->threadpool->allow_stealing)
                {
                    if(!c_thread_steal_work_order(thread))
                    {
                        goto sleep;
                    }
                }
                else
                {
                    goto sleep;
                }
            }
        }
        else if(thread->should_exit)
        {
            break;
        }
        else
        {
sleep:
            AtomicIncrement(&thread->threadpool->threads_flushed);
            sys_semaphore_wait(&thread->threadpool->work_avaliable_semaphore, 0);

            ReadWriteBarrier;
            // NOTE(Sleepster): When the thread wakes up, reset it's state.
            AtomicStore32(&thread->allocator.used, 0);
            AtomicDecrement(&thread->threadpool->threads_flushed);

            s32 threads_flushed = AtomicLoad32(&thread->threadpool->threads_flushed);
            Assert(threads_flushed >= 0);
        }
    }
    
    return(0);
}

