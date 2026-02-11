/* ========================================================================
   $File: new_threadpool.cpp $
   $Date: February 08 2026 07:29 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#define MATH_IMPLEMENTATION
#include <c_base.h>
#include <c_intrinsics.h>
#include <c_math.h>

#include <c_globals.cpp>
#include <c_string.cpp>

#include <p_platform_data.h>
#include <c_memory_arena.h>

#include <sys_linux.cpp>
#include <c_memory_arena.cpp>

// TODO(Sleepster): 
// - [ ] A manner with which to run work in batches and get the results of that completed work back from the threadpool
// - [X] Work stealing
// - [ ] parallel_for should allow threads to instead add work to themselves in parallel

#define MAX_WORK_ORDERS  (10000)
#define MAX_THREAD_COUNT (42)
#define CACHE_LINE       (64)

typedef void work_order_fn(void *data);

struct alignas(CACHE_LINE) work_order_t
{
    void           *data;
    work_order_fn  *function;
};

struct work_list_t 
{
    work_order_t    work_orders[MAX_WORK_ORDERS];
    u32             head;
    u32             tail;
};

struct thread_allocator_t
{
    byte             *buffer;
    u32               size;
    volatile u32      used;
};

struct threadpool_data_t;
struct worker_thread_t
{
    u32                thread_id;
    SDL_Thread        *handle;

    work_list_t        work_avaliable;
    thread_allocator_t allocator;
    volatile u32       total_work_orders;

    volatile bool32    is_started;
    volatile bool32    should_exit;

    threadpool_data_t *threadpool;
};

struct threadpool_data_t
{
    sys_semaphore_t work_avaliable_semaphore;
    sys_semaphore_t work_completed_semaphore;

    worker_thread_t workers[MAX_THREAD_COUNT];
    volatile u32    threads_flushed;

    volatile u32    next_worker_index;
    u32             thread_count;
};

int thread_proc_entry(void *user_data);
bool8 thread_steal_work_order(worker_thread_t *theif_thread);

void
threadpool_init(threadpool_data_t *threadpool, u32 max_threads, u32 thread_allocator_size, bool8 start_instantly)
{
    ZeroStruct(*threadpool);
    threadpool->thread_count = max_threads;
    threadpool->work_avaliable_semaphore = sys_semaphore_create(0, max_threads);
    threadpool->work_completed_semaphore = sys_semaphore_create(0, max_threads);

    for(u32 thread_index = 0;
        thread_index < max_threads;
        ++thread_index)
    {
        worker_thread_t *thread = threadpool->workers + thread_index;
        ZeroStruct(*thread);

        thread_allocator_t *allocator = &thread->allocator;
        allocator->buffer  = AllocArray(byte, thread_allocator_size);
        allocator->size    = thread_allocator_size;
        thread->is_started = start_instantly;
        thread->threadpool = threadpool;

        thread->handle = SDL_CreateThread(thread_proc_entry, null, thread);
    }
}

void
threadpool_start(threadpool_data_t *threadpool)
{
    AtomicStore(&threadpool->threads_flushed, 0);
    for(u32 thread_index = 0;
        thread_index < threadpool->thread_count;
        ++thread_index)
    {
        worker_thread_t *thread = threadpool->workers + thread_index;
        thread->is_started = true;
    }

    sys_semaphore_release(&threadpool->work_avaliable_semaphore, threadpool->thread_count);
}

void
threadpool_flush_work_orders(threadpool_data_t *threadpool)
{
    worker_thread_t this_thread = {};
    this_thread.is_started = true;
    this_thread.threadpool = threadpool;
    while(threadpool->threads_flushed != threadpool->thread_count)
    {
        thread_steal_work_order(&this_thread);
    }
}

template <typename LambdaType>
internal_api void
invoker(void *lambda_data)
{
    LambdaType *lambda = (LambdaType*)lambda_data;
    (*lambda)();
}

template <typename LambdaType>
void
thread_push_work_order(worker_thread_t *thread, LambdaType lambda)
{
    work_order_t new_work_order = {}; 
    void *data = 0;

    u32 allocation_size = Align8(sizeof(LambdaType));

    u32 used_offset = AtomicAdd(&thread->allocator.used, allocation_size); 
    if(used_offset <= thread->allocator.size)
    {
        data = thread->allocator.buffer + thread->allocator.used;
        thread->allocator.used += allocation_size;
    }

    Assert(data != null);
    memcpy(data, &lambda, sizeof(LambdaType));
    new_work_order.data     = data;
    new_work_order.function = invoker<LambdaType>;

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

template <typename LambdaType>
true_inline void
threadpool_push_work_order(threadpool_data_t *threadpool, LambdaType lambda)
{
    worker_thread_t *thread = threadpool->workers + threadpool->next_worker_index;
    threadpool->next_worker_index = (threadpool->next_worker_index + 1) % threadpool->thread_count;

    thread_push_work_order(thread, lambda);
}

bool8 
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

            ReadWriteBarrier;
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

bool8
thread_steal_work_order(worker_thread_t *theif_thread)
{
    bool8 result = true;

    u32 target_thread_index      = 0;
    u32 highest_work_order_count = 0;
    for(u32 thread_index = 0;
        thread_index < theif_thread->threadpool->thread_count;
        ++thread_index)
    {
        worker_thread_t *target_thread = theif_thread->threadpool->workers + thread_index;
        u32 work_orders_left = target_thread->work_avaliable.head - target_thread->work_avaliable.tail; 
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

            ReadWriteBarrier;
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

int
thread_proc_entry(void *user_data)
{
    worker_thread_t *thread = (worker_thread_t *)user_data;
    for(;;)
    {
        if(thread->is_started)
        {
            if(!thread_pop_work_order(thread))
            {
                if(!thread_steal_work_order(thread))
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
        }
    }
    
    return(0);
}

#define parallel_for_FIFO(threadpool, iterator, max_iterations, lambda) \
    for(u32 iterator = max_iterations; iterator > 0; --iterator)  \
        threadpool_push_work_order(threadpool, lambda)                  

#define parallel_for_LIFO(threadpool, iterator, max_iterations, lambda) \
    for(u32 iterator = 0; iterator < max_iterations; ++iterator)  \
        threadpool_push_work_order(threadpool, lambda)                  

int
main(void) 
{
    c_global_context_init();

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    threadpool_data_t *threadpool = Alloc(threadpool_data_t);

    // TODO(Sleepster): sys_get_thread_count()
    u32 thread_count = sys_get_cpu_count() - 1;
    threadpool_init(threadpool, thread_count, MB(200), false);

    parallel_for_FIFO(threadpool, work_index, MAX_WORK_ORDERS, [=]() {
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

    threadpool_start(threadpool);
    u64 last_tsc = SDL_GetPerformanceCounter();
    threadpool_flush_work_orders(threadpool);

    u64 current_tsc = SDL_GetPerformanceCounter();
    u64 delta_tsc   = current_tsc - last_tsc;
    last_tsc        = current_tsc;

    float32 delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    log_info("FINISHED... Time taken: '%.6f' seconds...\n", delta_time);

    return(0);
}
