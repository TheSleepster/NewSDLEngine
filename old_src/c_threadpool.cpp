/* ========================================================================
   $File: c_threadpool.cpp $
   $Date: December 04 2025 04:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_intrinsics.h>
#include <c_base.h>
#include <c_types.h>

#include <c_threadpool.h>
#include <p_platform_data.h>

PLATFORM_THREAD_PROC(ThreadProc);

bool8
c_threadpool_perform_next_task(threadpool_queue_t *queue)
{
    bool8 result = true;

    u32 this_entry_to_read = queue->next_entry_to_read;
    u32 next_entry_to_read = (this_entry_to_read + 1) % MAX_QUEUE_ENTRIES;
    if(this_entry_to_read != queue->next_entry_to_write)
    {
        ReadWriteBarrier;
        u32 task = AtomicCompareExchange((volatile s32*)&queue->next_entry_to_read,
                                         next_entry_to_read,
                                         this_entry_to_read);
        if(task == this_entry_to_read)
        {
            threadpool_queue_entry_t *entry = queue->entries + task;
            if(entry->is_valid)
            {
                ReadWriteBarrier;

                entry->is_valid = false;
                entry->callback(entry->user_data);
                AtomicIncrement32(&queue->entries_completed);
            }
        }
    }
    else
    {
        result = false;
    }

    return(result);
}

void
c_threadpool_init(threadpool_t *pool) 
{
#if OS_WINDOWS
    SetProcessDPIAware();
    timeBeginPeriod(1);
#endif

    u32 num_processors = sys_get_cpu_count();

    ZeroStruct(*pool);
    pool->semaphore   = sys_semaphore_create(0, num_processors);
    pool->max_threads = num_processors;

    for(u32 thread_index = 0;
        thread_index < num_processors;
        ++thread_index)
    {
        sys_thread_create(ThreadProc, pool, true);
    }
}

bool8
c_threadpool_add_task(threadpool_t *threadpool, void *user_data, threadpool_callback_t *callback, u32 priority)
{
    bool8 result = true;

    threadpool_queue_t *queue = null;
    Assert(priority != TPTP_Invalid);
    switch(priority) 
    {
        case TPTP_Low:
        {
            queue = &threadpool->low_priority_queue;
        }break;
        case TPTP_High:
        {
            queue = &threadpool->high_priority_queue;
        }break;
    }
    
    ReadWriteBarrier;
    u32 entry_to_write      = queue->next_entry_to_write;
    u32 next_entry_to_write = (entry_to_write + 1) % MAX_QUEUE_ENTRIES;
    Assert(next_entry_to_write != queue->next_entry_to_read);

    u32 queue_entry_to_write = AtomicCompareExchange32((volatile s32*)&queue->next_entry_to_write,
                                                       next_entry_to_write,
                                                       entry_to_write);
    if(queue_entry_to_write == entry_to_write)
    {
        threadpool_queue_entry_t *entry = queue->entries + entry_to_write;

        ReadWriteBarrier;
        entry->is_valid  = true;
        entry->user_data = user_data;
        entry->callback  = callback;

        AtomicIncrement32(&queue->completion_goal);
        sys_semaphore_release(&threadpool->semaphore, 1);
    }
    else
    {
        result = false;
    }

    return(result);
}

void 
c_threadpool_flush_queue(threadpool_queue_t *queue)
{
    while(queue->completion_goal != queue->entries_completed)
    {
        if(!c_threadpool_perform_next_task(queue)) break;
    }

    ReadWriteBarrier;
    AtomicExchange(&queue->completion_goal,   0);
    AtomicExchange(&queue->entries_completed, 0);
}

void
c_threadpool_flush_task_queues(threadpool_t *threadpool)
{
    c_threadpool_flush_queue(&threadpool->high_priority_queue);
    c_threadpool_flush_queue(&threadpool->low_priority_queue);
}

PLATFORM_THREAD_PROC(ThreadProc)
{
    threadpool_t *pool = (threadpool_t*)user_data;
    for(;;)
    {
        if(!c_threadpool_perform_next_task(&pool->high_priority_queue))
        {
            if(!c_threadpool_perform_next_task(&pool->low_priority_queue))
            {
                sys_semaphore_wait(&pool->semaphore, 0);
            }
        }
    }
    return(0);
}
