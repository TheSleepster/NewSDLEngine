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
// - [ ] Work stealing
// - [ ] parallel_for should allow threads to instead add work to themselves in parallel

constexpr u32 MAX_WORK_ORDERS  = 300000;
constexpr u32 MAX_THREAD_COUNT = 36;
constexpr u32 CACHE_LINE       = 64;

struct threadpool_data_t;
typedef void work_order_fn(void *data);

int thread_proc_entry(void *data);

struct thread_data_t
{
    threadpool_data_t *threadpool;
    u32                thread_id;
};

struct alignas(CACHE_LINE) work_order_t
{
    void           *data;
    work_order_fn  *function;
};

struct work_order_list_t 
{
    sys_semaphore_t semaphore;

    work_order_t    work_orders[MAX_WORK_ORDERS];
    volatile u32    next_entry_to_read;
    volatile u32    next_entry_to_write;
};

struct thread_allocator_t
{
    sys_mutex_t       mutex;
    byte             *buffer;
    u32               size;
    volatile u32      used;
};

struct threadpool_thread_t
{
    u32                  thread_id;
    SDL_Thread          *handle;

    work_order_list_t    work_available;
    work_order_list_t    work_completed;
    u32                  work_order_count;
    u32                  work_orders_completed;

    thread_allocator_t   allocator;
    threadpool_thread_t *threads;
    u32                  thread_count;
};

struct threadpool_data_t
{
    threadpool_thread_t threads[MAX_THREAD_COUNT];
    u32                 thread_count;
    volatile u32        next_worker_thread_index;

    u32                 work_order_count;
    volatile u32        work_orders_completed;

    volatile bool32     is_started;
    volatile bool32     should_exit;
};

// TODO(Sleepster): threads should instead add work to themselves in parallel
#define parallel_for(threadpool, iterator_name, iteration_count, lambda)         \
    for(u32 iterator_name = 0; iterator_name < iteration_count; ++iterator_name) \
        push_work_order(threadpool, lambda);

template <typename LambdaType>
internal_api void
invoker(void *lambda_data)
{
    LambdaType *lambda = (LambdaType*)lambda_data;
    (*lambda)();
}

void
append_work_to_thread_list(threadpool_thread_t *thread, work_order_t *work_order)
{
    work_order_list_t *list = &thread->work_available;
    for(;;)
    {
        u32 original_next_entry_to_write = list->next_entry_to_write;
        u32 new_next_entry_to_write      = (original_next_entry_to_write + 1) % MAX_WORK_ORDERS;

        list->work_orders[original_next_entry_to_write] = *work_order;
        if((u32)AtomicCompareExchange(&list->next_entry_to_write, 
                                      new_next_entry_to_write, 
                                      original_next_entry_to_write) == original_next_entry_to_write)
        {
            AtomicIncrement(&thread->work_order_count);
            break;
        }
    }
}

template <typename LambdaType>
void
push_work_order(threadpool_data_t *threadpool, LambdaType lambda)
{
    u32 original_next_worker_index = threadpool->next_worker_thread_index;
    u32 next_worker_index          = (original_next_worker_index + 1) % threadpool->thread_count;

    u32 next_work_index = AtomicCompareExchange32((volatile s32*)&threadpool->next_worker_thread_index,
                                                  next_worker_index,
                                                  original_next_worker_index);
    if(next_work_index == original_next_worker_index)
    {
        threadpool_thread_t *thread = threadpool->threads + next_worker_index;
        work_order_t new_work_order = {}; 
        void *data = 0;

        sys_mutex_lock(&thread->allocator.mutex, true);

        u32 allocation_size = Align8(sizeof(LambdaType));
        if(thread->allocator.used + allocation_size <= thread->allocator.size)
        {
            data = thread->allocator.buffer + thread->allocator.used;
            thread->allocator.used += allocation_size;
        }

        sys_mutex_unlock(&thread->allocator.mutex);

        Assert(data != null);
        memcpy(data, &lambda, sizeof(LambdaType));
        new_work_order.data     = data;
        new_work_order.function = invoker<LambdaType>;

        append_work_to_thread_list(thread, &new_work_order);
        sys_semaphore_release(&thread->work_available.semaphore, 1);
    }
}

bool8 
thread_steal_work_order(threadpool_thread_t *theif_thread)
{
    bool8 result = true;

    u32 current_highest_job_counter = 0;
    threadpool_thread_t *target_thread = null;
    for(u32 thread_index = 0;
        thread_index < theif_thread->thread_count;
        ++thread_index)
    {
        threadpool_thread_t *thread = theif_thread->threads + thread_index;
        u32 work_orders_left = (thread->work_order_count - thread->work_orders_completed);
        if(work_orders_left > current_highest_job_counter)
        {
            current_highest_job_counter = work_orders_left;
            target_thread               = thread;
        }
    }

    // NOTE(Sleepster): Spin until we steal 
    if(target_thread)
    {
        for(;;)
        {
            u32 next_entry_to_read  = AtomicLoad(&target_thread->work_available.next_entry_to_read);
            u32 next_entry_to_write = AtomicLoad(&target_thread->work_available.next_entry_to_write) - 1;
            if(next_entry_to_read != next_entry_to_write)
            {
                u32 new_next_entry_to_write = (next_entry_to_write == 0) ? (MAX_WORK_ORDERS - 1) : (next_entry_to_write - 1);;
                u32 entry_to_steal = AtomicCompareExchange32(&target_thread->work_available.next_entry_to_write,
                                                             new_next_entry_to_write,
                                                             next_entry_to_write);
                if(entry_to_steal == next_entry_to_write)
                {
                    work_order_t *work_order = target_thread->work_available.work_orders + entry_to_steal;
                    Assert(work_order);
                    Assert(work_order->data);
                    Assert(work_order->function);

                    work_order->function(work_order->data);
                    AtomicIncrement(&target_thread->work_orders_completed);
                }
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        result = false;
    }
    
    return(result);
}

bool8 
thread_do_next_work_order(threadpool_thread_t *thread)
{    
    bool8 result = true;

    u32 original_entry_to_read =  thread->work_available.next_entry_to_read;
    u32 new_next_entry_to_read = (original_entry_to_read + 1) % MAX_WORK_ORDERS;

    if(original_entry_to_read != thread->work_available.next_entry_to_write)
    {
        u32 next_entry_to_read = AtomicCompareExchange((volatile s32*)&thread->work_available.next_entry_to_read, 
                                                       new_next_entry_to_read,
                                                       original_entry_to_read);
        if(next_entry_to_read == original_entry_to_read)
        {
            work_order_t *work_order = &thread->work_available.work_orders[next_entry_to_read];
            Assert(work_order);
            Assert(work_order->data);
            Assert(work_order->function);

            work_order->function(work_order->data);
            AtomicIncrement(&thread->work_orders_completed);
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

bool8 
thread_get_next_work_order(threadpool_thread_t *thread)
{
    bool8 result = false;
    if(!thread_do_next_work_order(thread))
    {
        f(!thread_steal_work_order(thread))
        {
            result = true;
        }
    }

    return(result);
}

true_inline void
threadpool_work_order_list_create(work_order_list_t *list, u32 max_thread_count)
{
    list->semaphore = sys_semaphore_create(0, max_thread_count);
}

void
threadpool_init_thread(threadpool_data_t   *threadpool, 
                       threadpool_thread_t *thread, 
                       u32                  linear_allocator_size, 
                       u32                  thread_id)
{
    ZeroStruct(*thread);
    thread->allocator.buffer = (byte*)malloc(Align16(linear_allocator_size));
    thread->allocator.size   = linear_allocator_size;
    thread->thread_id        = thread_id;

    thread_data_t *thread_data = Alloc(thread_data_t);
    *thread_data = {
        .threadpool = threadpool,
        .thread_id  = thread->thread_id
    };

    thread->handle           = SDL_CreateThread(thread_proc_entry, null, thread_data);
    thread->allocator.mutex  = sys_mutex_create();
    thread->threads          = threadpool->threads;

    threadpool_work_order_list_create(&thread->work_available, threadpool->thread_count);
    threadpool_work_order_list_create(&thread->work_completed, threadpool->thread_count);
}

void
threadpool_init(threadpool_data_t *threadpool, u32 max_threads, bool8 start_instantly)
{
    ZeroStruct(*threadpool);

    threadpool->thread_count = max_threads;
    threadpool->is_started   = start_instantly;

    for(u32 thread_index = 0;
        thread_index < max_threads;
        ++thread_index)
    {
        threadpool_thread_t *thread = threadpool->threads + thread_index;

        threadpool_init_thread(threadpool, thread, MB(10), thread_index);
        thread->thread_count = max_threads;
    }
}

void
threadpool_start(threadpool_data_t *threadpool)
{
    for(u32 thread_index = 0;
        thread_index < threadpool->thread_count;
        ++thread_index)
    {
        threadpool_thread_t *thread = threadpool->threads + thread_index;
        sys_semaphore_release(&thread->work_available.semaphore, 1);
    }
    AtomicStore(&threadpool->is_started, true);
}

void
threadpool_flush_work_orders(threadpool_data_t *threadpool)
{
}

int
thread_proc_entry(void *data)
{
    thread_data_t *thread_data    = (thread_data_t *)data;
    threadpool_data_t *threadpool = thread_data->threadpool;
    u32 thread_id                 = thread_data->thread_id;

    threadpool_thread_t *thread = threadpool->threads + thread_id;
    for(;;)
    {
        if(threadpool->is_started == true)
        {
            // NOTE(Sleepster): 'true' indicates we failed to get work... 
            if(thread_get_next_work_order(thread))
            {
                sys_mutex_lock(&thread->allocator.mutex, true);
                thread->allocator.used = 0;
                sys_mutex_unlock(&thread->allocator.mutex);

                sfence();
                ReadWriteBarrier;
                sys_semaphore_wait(&thread->work_available.semaphore, 0);
            }

            if(threadpool->should_exit == true) break;
        }
    }

    return(0);
}

int
main(void) 
{
    u64 perf_count_freq = SDL_GetPerformanceFrequency();

    u32 thread_count = sys_get_cpu_count();
    threadpool_data_t *threadpool = Alloc(threadpool_data_t);
    threadpool_init(threadpool, thread_count, false);

    // NOTE(Sleepster): Single threaded this takes 35.45 seconds. 
    //                  In parallel, it takes ~4.2 seconds
    vec3_t rotation_vector = vec3(1.0, 0.3, 0.0);
#if 1
    // NOTE(Sleepster): Fast path 
    parallel_for(threadpool, index, MAX_WORK_ORDERS, [=]() {
        mat4_t matrix          = mat4_identity();
        mat4_t translation_mat = mat4_make_translation(vec3(100, 100, 0));
        mat4_t rotation_matrix = mat4_rotate(matrix, rotation_vector, 10.8f);

        mat4_t final_matrix = mat4_multiply(matrix, mat4_multiply(translation_mat, rotation_matrix));
        for(u32 mul_index = 0;
            mul_index < 100;
            ++mul_index)
        {
            final_matrix = mat4_multiply(final_matrix, mat4_multiply(translation_mat, rotation_matrix));
            final_matrix = mat4_scale(final_matrix, vec3(index, index, index));
            (void)final_matrix;
        }

        printf("Finished work at index: '%d'...\n", index);
    });

    u64 last_tsc = SDL_GetPerformanceCounter();
    threadpool_start(threadpool);

    u32 work_orders_completed = 0;
    while(work_orders_completed < MAX_WORK_ORDERS)
    {
        work_orders_completed = 0;
        for(u32 thread_index = 0;
            thread_index < thread_count;
            ++thread_index)
        {
            threadpool_thread_t *thread = threadpool->threads + thread_index;
            u32 work_completed = AtomicLoad(&thread->work_orders_completed);

            work_orders_completed += work_completed;
        }
    }
#else
    // NOTE(Sleepster): Slow path
    u64 last_tsc = SDL_GetPerformanceCounter();
    for(u32 index = 0;
        index < MAX_WORK_ORDERS;
        ++index)
    {
        mat4_t matrix          = mat4_identity();
        mat4_t translation_mat = mat4_make_translation(vec3(100, 100, 0));
        mat4_t rotation_matrix = mat4_rotate(matrix, rotation_vector, 10.8f);

        mat4_t final_matrix = mat4_multiply(matrix, mat4_multiply(translation_mat, rotation_matrix));
        for(u32 mul_index = 0;
            mul_index < 100;
            ++mul_index)
        {
            final_matrix = mat4_multiply(final_matrix, mat4_multiply(translation_mat, rotation_matrix));
            final_matrix = mat4_scale(final_matrix, vec3(index, index, index));
            (void)final_matrix;
        }

        printf("Finished work at index: '%d'...\n", index);
    }
#endif
    // TODO(Sleepster): We have no good estimate on how long this is taking because we don't have the infrastructure
    // to time that right now. We need some of that.

    u64 current_tsc = SDL_GetPerformanceCounter();
    u64 delta_tsc   = current_tsc - last_tsc;
    last_tsc        = current_tsc;

    float32 delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    log_info("FINISHED... Time taken: '%.6f' seconds...\n", delta_time);

    return(0);
}
