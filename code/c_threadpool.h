#if !defined(C_THREADPOOL_H)
/* ========================================================================
   $File: c_threadpool.h $
   $Date: March 26 2026 04:41 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_THREADPOOL_H

#include <c_base.h>
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
    u32             thread_count;

    alignas(CACHE_LINE) volatile u32 threads_flushed;
    alignas(CACHE_LINE) volatile u32 next_worker_index;
};

/*===========================================
  ============= FUNCATION API ===============
  ===========================================*/

void c_threadpool_init(threadpool_t *threadpool, u32 max_threads, u32 thread_allocator_size, bool8 start_instantly);
void c_threadpool_start(threadpool_t *threadpool);
void c_threadpool_flush_work_orders(threadpool_t *threadpool);
void c_threadpool_wait_on_fence(threadpool_t *threadpool, work_completion_fence_t *fence);

template <typename LambdaType>
void c_threadpool_push_work_order(threadpool_t *threadpool, LambdaType lambda, work_completion_fence_t *fence);

/*===========================================
  ================= MACROS ==================
  ===========================================*/

#define parallel_for_FIFO(threadpool, iterator, max_iterations, work_completed_fence_ptr, lambda) \
    for(u32 iterator = max_iterations; iterator > 0; --iterator)  \
        c_threadpool_push_work_order(threadpool, lambda, work_completed_fence_ptr)                  

// NOTE(Sleepster): This is by default first in last out. 
#define parallel_for(threadpool, iterator, max_iterations, work_completed_fence_ptr, lambda) \
    for(u32 iterator = 0; iterator < max_iterations; ++iterator)  \
        c_threadpool_push_work_order(threadpool, lambda, work_completed_fence_ptr)                  

#endif // C_THREADPOOL_H

