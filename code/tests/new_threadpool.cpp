/* ========================================================================
   $File: new_threadpool.cpp $
   $Date: February 08 2026 07:29 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#include <c_base.h>
#include <c_intrinsics.h>
#include <c_globals.cpp>
#include <c_string.cpp>

#include <p_platform_data.h>
#include <sys_linux.cpp>

constexpr u32 MAX_WORK_ORDERS  = 800;
constexpr u32 MAX_THREAD_COUNT = 36;

struct threadpool_data_t;
typedef void work_order_fn(void *data);

struct thread_data_t
{
    threadpool_data_t *threadpool;
    u32                thread_id;
};

struct work_order_t
{
    bool8          is_valid;

    void          *data;
    work_order_fn *function;
};

struct threadpool_thread_t
{
    u32           thread_id;
    SDL_Thread   *handle;
        
    work_order_t work_orders[MAX_WORK_ORDERS];
    u32          next_entry_to_write;
    u32          next_entry_to_read;

    // NOTE(Sleepster): For work stealing 
    volatile u32 tail_entry_index;


    byte        *linear_allocator;
    u32          allocator_size;
    u32          allocator_used;
};

struct threadpool_data_t
{
    threadpool_thread_t threads[MAX_THREAD_COUNT];
        
    work_order_t work_orders[MAX_WORK_ORDERS];
    u32          work_order_count;
    u32          work_orders_completed;

    byte        *linear_allocator;
    u32          allocator_size;
    u32          allocator_used;
};

template <typename LambdaType>
void
invoker(void *lambda_data)
{
    LambdaType *lambda = (LambdaType*)lambda_data;
    (*lambda)();
}

template <typename LambdaType>
void
push_work_order(threadpool_data_t *threadpool, LambdaType lambda)
{
    work_order_t *order = threadpool->work_orders + threadpool->work_order_count++;

    void *data = 0;

    u32 allocation_size = Align8(sizeof(LambdaType));
    if(threadpool->allocator_used + allocation_size <= threadpool->allocator_size)
    {
        data = threadpool->linear_allocator + threadpool->allocator_used;
        threadpool->allocator_used += allocation_size;
    }
    Assert(data != null);

    memcpy(data, &lambda, sizeof(LambdaType));
    order->data     = data;
    order->function = invoker<LambdaType>;
    order->is_valid = true;
}

int
thread_proc_entry(void *data)
{
    thread_data_t *thread_data    = (thread_data_t *)data;
    threadpool_data_t *threadpool = thread_data->threadpool;
    u32 thread_id                 = thread_data->thread_id;

    threadpool_thread_t *thread = threadpool->threads + thread_id;
    while(thread->next_entry_to_read != thread->next_entry_to_write)
    {
        work_order_t *order = thread->work_orders + thread->next_entry_to_read;
        if(order->is_valid)
        {
            order->function(order->data);
        }
    }

    return(0);
}

void
threadpool_init_thread(threadpool_data_t *threadpool, threadpool_thread_t *thread, u32 linear_allocator_size, u32 thread_id)
{
    ZeroStruct(*thread);
    thread->linear_allocator = (byte*)malloc(Align16(linear_allocator_size));
    thread->thread_id        = thread_id;
    thread->handle           = SDL_CreateThread(thread_proc_entry, null, threadpool);
}

int
main(void)
{
    threadpool_data_t threadpool = {};

    thread_data_t thread_data = {};
    thread_data.threadpool = &threadpool;

    u32 thread_count = sys_get_cpu_count();
    for(u32 thread_index = 0;
        thread_index < thread_count;
        ++thread_index)
    {
        threadpool_thread_t *thread = threadpool.threads + thread_index;
        thread_data.thread_id = thread_index;

        threadpool_init_thread(&threadpool, thread, KB(10), thread_index);
    }

    threadpool.linear_allocator  = (byte*)malloc(KB(200));
    threadpool.allocator_size    = KB(200);
    ZeroMemory(threadpool.linear_allocator, threadpool.allocator_size);

    threadpool_data_t *threadpool_ptr = &threadpool;
    for(u32 index = 0;
        index < MAX_WORK_ORDERS;
        ++index)
    {
        push_work_order(&threadpool, [=]() mutable {
            printf("Hello from lambda... This is work order: '%d'...\n", threadpool_ptr->work_orders_completed);
            threadpool_ptr->work_orders_completed++;
        });
    }

    return(0);
}
