#if !defined(C_THREADPOOL_H)
/* ========================================================================
   $File: c_threadpool.h $
   $Date: March 26 2026 04:41 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_THREADPOOL_H

#include <c_base.h>
#include <c_intrinsics.h>
#include <c_types.h>
#include <p_platform_data.h>

#define MAX_WORK_ORDERS  (10000)
#define MAX_THREAD_COUNT (42)
#define CACHE_LINE       (64)

// NOTE(Sleepster): 
// By default this threadpool uses LIFO queueing due to the Chase-Lev style 
// queueing and work queue stealing

typedef void work_order_fn(void *data);

struct work_completion_fence_t
{
    volatile u32 pending;
};

struct work_order_t
{
    void                    *data;
    work_order_fn           *function;
    work_completion_fence_t *fence;
    u64                      __padding;
};

struct work_list_t 
{
    work_order_t work_orders[MAX_WORK_ORDERS];

    alignas(CACHE_LINE) u32 head;
    alignas(CACHE_LINE) u32 tail;
};

struct thread_allocator_t
{
    byte *buffer;

    alignas(CACHE_LINE) u32          size;
    alignas(CACHE_LINE) volatile u32 used;
};

struct threadpool_t;
struct worker_thread_t
{
    u32                thread_id;
    sys_thread_t       handle;

    work_list_t        work_avaliable;
    thread_allocator_t allocator;
    threadpool_t      *threadpool;

    alignas(CACHE_LINE) volatile u32    total_work_orders;
    alignas(CACHE_LINE) volatile bool32 is_started;
    alignas(CACHE_LINE) volatile bool32 should_exit;

};

struct threadpool_t
{
    sys_semaphore_t work_avaliable_semaphore;
    worker_thread_t workers[MAX_THREAD_COUNT];
    s32             thread_count;

    alignas(CACHE_LINE) volatile s32 threads_flushed;
    alignas(CACHE_LINE) volatile s32 next_worker_index;
};

/*===========================================
  ============= FUNCATION API ===============
  ===========================================*/

void c_threadpool_init(threadpool_t *threadpool, u32 max_threads, u32 thread_allocator_size, bool8 start_instantly);
void c_threadpool_start(threadpool_t *threadpool);
void c_threadpool_flush_work_orders(threadpool_t *threadpool);
void c_threadpool_wait_on_fence(threadpool_t *threadpool, work_completion_fence_t *fence);

template <typename LambdaType>
true_inline void c_threadpool_push_work_order(threadpool_t *threadpool, LambdaType lambda, work_completion_fence_t *fence);

/*===========================================
  ================= MACROS ==================
  ===========================================*/

#define parallel_for_FIFO(threadpool, iterator, max_iterations, work_completed_fence_ptr, lambda) \
    for(u32 iterator = max_iterations; iterator > 0; --iterator) \
        c_threadpool_push_work_order(threadpool, lambda, work_completed_fence_ptr)                  

// NOTE(Sleepster): This is by default first in last out. 
#define parallel_for(threadpool, iterator, max_iterations, work_completed_fence_ptr, lambda) \
    for(u32 iterator = 0; iterator < max_iterations; ++iterator) \
        c_threadpool_push_work_order(threadpool, lambda, work_completed_fence_ptr)                  

// NOTE(Sleepster): 
// C++ is stupid and is an awfully designed language... I would much rather NOT have these here, but I lack any choice in the matter.

/*
=============
invoke
=============
*/

template <typename LambdaType>
internal_api void
invoke(void *lambda_data)
{
    LambdaType *lambda = static_cast<LambdaType*>(lambda_data);
    (*lambda)();
}

/*
=============
c_threadpool_push_work_order
=============
*/

template <typename LambdaType>
internal_api void
c_threadpool_push_work_order(worker_thread_t *thread, LambdaType lambda, work_completion_fence_t *fence)
{
    work_order_t new_work_order = {}; 
    void *data = 0;

    u32 allocation_size = Align(sizeof(LambdaType), 64);

    u32 used_offset = AtomicExchangeAdd32(&thread->allocator.used, allocation_size); 
    if(used_offset <= thread->allocator.size)
    {
        data = thread->allocator.buffer + used_offset;
    }

    Assert(data != null);
    memcpy(data, &lambda, sizeof(LambdaType));

    new_work_order.data     = data;
    new_work_order.function = invoke<LambdaType>;
    new_work_order.fence    = fence;
    for(;;)
    {
        u32 original_next_head_index = thread->work_avaliable.head;
        u32 next_head_index          = (original_next_head_index + 1) % MAX_WORK_ORDERS;

        u32 next_entry = AtomicCompareExchange32(&thread->work_avaliable.head,
                                                 next_head_index,
                                                 original_next_head_index);
        if(next_entry == original_next_head_index)
        {
            thread->work_avaliable.work_orders[next_entry] = new_work_order;
            AtomicIncrement32(&thread->total_work_orders);
            if(thread->is_started)
            {
                sys_semaphore_release(&thread->threadpool->work_avaliable_semaphore, 1);
            }

            ReadWriteBarrier;
            break;
        }
    }
}

/*
=============
c_threadpool_push_work_order
=============
*/

template <typename LambdaType>
true_inline void
c_threadpool_push_work_order(threadpool_t *threadpool, LambdaType lambda, work_completion_fence_t *fence)
{
    worker_thread_t *thread = threadpool->workers + threadpool->next_worker_index;
    threadpool->next_worker_index = (threadpool->next_worker_index + 1) % threadpool->thread_count;
    if(fence)
    {
        AtomicIncrement(&fence->pending);
    }

    c_threadpool_push_work_order(thread, lambda, fence);
}


#endif // C_THREADPOOL_H

