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

constexpr u32 MAX_WORK_ORDERS  = 10000;
constexpr u32 MAX_THREAD_COUNT = 36;

constexpr u32 CACHE_LINE = 64;

struct threadpool_data_t;
typedef void work_order_fn(void *data);

struct thread_data_t
{
    threadpool_data_t *threadpool;
    u32                thread_id;
};

struct alignas(CACHE_LINE) work_order_t
{
#if 0
    string_t       DEBUG_name;
#endif

    void          *data;
    work_order_fn *function;
};

struct work_order_list_t 
{
    sys_semaphore_t semaphore;
    sys_mutex_t     mutex;

    work_order_t    work_orders[MAX_WORK_ORDERS];
    volatile u32    next_entry_to_read;
    volatile u32    next_entry_to_write;
};

struct threadpool_thread_t
{
    u32               thread_id;
    SDL_Thread       *handle;

    work_order_list_t work_available;
    work_order_list_t work_completed;
    u32               work_order_count;
    u32               work_orders_completed;

    sys_mutex_t       allocator_mutex;
    byte             *linear_allocator;
    u32               allocator_size;
    volatile u32      allocator_used;
};

struct threadpool_data_t
{
    threadpool_thread_t threads[MAX_THREAD_COUNT];
    u32                 thread_count;

    volatile u32        next_worker_index;

    bool32              is_started;
    bool32              should_exit;
};

template <typename LambdaType>
internal_api void
invoker(void *lambda_data)
{
    LambdaType *lambda = (LambdaType*)lambda_data;
    (*lambda)();
}

void
append_work_to_thread_list(work_order_list_t *list, work_order_t *work_order)
{
    sys_mutex_lock(&list->mutex, true);

    u32 original_next_entry_to_write =  list->next_entry_to_write;
    u32 new_next_entry_to_write      = (original_next_entry_to_write + 1) % MAX_WORK_ORDERS;

    mfence();
    sfence();
    ReadWriteBarrier;
    
    u32 next_entry_to_write = AtomicCompareExchange32((volatile s32*)&list->next_entry_to_write,
                                                      new_next_entry_to_write, 
                                                      original_next_entry_to_write);
    if(next_entry_to_write == original_next_entry_to_write)
    {
        list->work_orders[original_next_entry_to_write] = *work_order;

        mfence();
        WriteBarrier;
    }
    sys_mutex_unlock(&list->mutex);
}

template <typename LambdaType>
void
push_work_order(threadpool_data_t *threadpool, LambdaType lambda)
{
    u32 original_next_worker_index =  threadpool->next_worker_index;
    u32 next_worker_index          = (original_next_worker_index + 1) % threadpool->thread_count;
    ReadWriteBarrier;

    u32 next_work_index = AtomicCompareExchange32((volatile s32*)&threadpool->next_worker_index,
                                                  next_worker_index,
                                                  original_next_worker_index);
    if(next_work_index == original_next_worker_index)
    {
        threadpool_thread_t *thread = threadpool->threads + next_worker_index;
        work_order_t new_work_order = {}; 
        void *data = 0;

        sys_mutex_lock(&thread->allocator_mutex, true);

        u32 allocation_size = Align8(sizeof(LambdaType));
        if(thread->allocator_used + allocation_size <= thread->allocator_size)
        {
            data = thread->linear_allocator + thread->allocator_used;
            thread->allocator_used += allocation_size;
        }

        sys_mutex_unlock(&thread->allocator_mutex);

        Assert(data != null);
        memcpy(data, &lambda, sizeof(LambdaType));
        new_work_order.data     = data;
        new_work_order.function = invoker<LambdaType>;

        append_work_to_thread_list(&thread->work_available, &new_work_order);
        thread->work_order_count++;

        mfence();
        sfence();
        ReadWriteBarrier;

        sys_semaphore_release(&thread->work_available.semaphore, 1);
    }
}

bool8 
thread_do_next_work_order(work_order_list_t *list)
{
    sys_mutex_lock(&list->mutex, true);

    bool8 result = false;
    u32 original_entry_to_read =  list->next_entry_to_read;
    u32 new_next_entry_to_read = (original_entry_to_read + 1) % MAX_WORK_ORDERS;
    if(original_entry_to_read != list->next_entry_to_write)
    {
        mfence();
        sfence();
        ReadWriteBarrier;
        u32 next_entry_to_read = AtomicCompareExchange((volatile s32*)&list->next_entry_to_read, 
                                                       new_next_entry_to_read,
                                                       original_entry_to_read);
        if(next_entry_to_read == original_entry_to_read)
        {
            work_order_t work_order = list->work_orders[next_entry_to_read];
            work_order.function(work_order.data);
        }
    }
    else
    {
        result = true;
    }
    sys_mutex_unlock(&list->mutex);

    return(result);
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
        if(threadpool->is_started)
        {
            // NOTE(Sleepster): 'true' indicates we failed to get work... 
            if(thread_do_next_work_order(&thread->work_available))
            {
                AtomicStore(&thread->allocator_used, 0);

                sfence();
                WriteBarrier;
                sys_semaphore_wait(&thread->work_available.semaphore, 0);
            }
            else
            {
                thread->work_orders_completed++;
            }

            if(threadpool->should_exit == true) break;
        }
        else
        {
            sys_semaphore_wait(&thread->work_available.semaphore, 0);
        }
    }

    return(0);
}

true_inline void
threadpool_work_order_list_create(work_order_list_t *list, u32 max_thread_count)
{
    list->mutex     = sys_mutex_create();
    list->semaphore = sys_semaphore_create(0, max_thread_count);
}

void
threadpool_init_thread(threadpool_data_t   *threadpool, 
                       threadpool_thread_t *thread, 
                       u32                  linear_allocator_size, 
                       u32                  thread_id)
{
    ZeroStruct(*thread);
    thread->linear_allocator = (byte*)malloc(Align16(linear_allocator_size));
    thread->allocator_size   = linear_allocator_size;
    thread->thread_id        = thread_id;

    thread_data_t *thread_data = Alloc(thread_data_t);
    *thread_data = {
        .threadpool = threadpool,
        .thread_id  = thread->thread_id
    };

    thread->handle           = SDL_CreateThread(thread_proc_entry, null, thread_data);
    thread->allocator_mutex  = sys_mutex_create();

    threadpool_work_order_list_create(&thread->work_available, threadpool->thread_count);
    threadpool_work_order_list_create(&thread->work_completed, threadpool->thread_count);
}

#define parallel_for(threadpool, iterator_name, iteration_count, lambda) \
    for(u32 iterator_name = 0; iterator_name < iteration_count; ++iterator_name) \
        push_work_order(threadpool, lambda);


int
main(void)
{
    u64 perf_count_freq = SDL_GetPerformanceFrequency();

    threadpool_data_t *threadpool = Alloc(threadpool_data_t);
    ZeroStruct(*threadpool);

    u32 thread_count = sys_get_cpu_count();
    threadpool->thread_count = thread_count;
    for(u32 thread_index = 0;
        thread_index < thread_count;
        ++thread_index)
    {
        threadpool_thread_t *thread = threadpool->threads + thread_index;
        threadpool_init_thread(threadpool, thread, MB(10), thread_index);
    }

    // NOTE(Sleepster): Single threaded this takes 35.45 seconds. 
    //                  In parallel, it takes ~4.2 seconds
    vec3_t rotation_vector = vec3(1.0, 0.3, 0.0);
    parallel_for(threadpool, index, MAX_WORK_ORDERS, [=]() {
        mat4_t matrix          = mat4_identity();
        mat4_t translation_mat = mat4_make_translation(vec3(100, 100, 0));
        mat4_t rotation_matrix = mat4_rotate(matrix, rotation_vector, 10.8f);

        mat4_t final_matrix = mat4_multiply(matrix, mat4_multiply(translation_mat, rotation_matrix));
        for(u32 mul_index = 0;
            mul_index < 10000;
            ++mul_index)
        {
            final_matrix = mat4_multiply(final_matrix, mat4_multiply(translation_mat, rotation_matrix));
            (void)final_matrix;
        }

        printf("Finished work at index: '%d'...\n", index);
    });

    threadpool->is_started = true;

    u64 last_tsc = SDL_GetPerformanceCounter();
    for(u32 thread_index = 0;
        thread_index < thread_count;
        ++thread_index)
    {
        threadpool_thread_t *thread = threadpool->threads + thread_index;
        sys_semaphore_release(&thread->work_available.semaphore, 1);
    }

    // TODO(Sleepster): We have no good estimate on how long this is taking because we don't have the infrastructure
    // to time that right now. We need some of that.
    SDL_Delay(4500);

    u64 current_tsc = SDL_GetPerformanceCounter();
    u64 delta_tsc   = current_tsc - last_tsc;
    last_tsc        = current_tsc;

    float32 delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    log_info("FINISHED... Time taken: '%.2f' seconds...\n", delta_time);

    return(0);
}
