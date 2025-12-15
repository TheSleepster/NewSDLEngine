#if !defined(C_THREADPOOL_H)
/* ========================================================================
   $File: c_threadpool.h $
   $Date: December 04 2025 04:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_THREADPOOL_H
#include <c_intrinsics.h>
#include <c_base.h>
#include <c_types.h>

#include <p_platform_data.h>

#define MAX_QUEUE_ENTRIES (10000)
typedef void threadpool_callback_t(void *user_data);

enum job_priority_t 
{
    TPTP_Invalid,
    TPTP_Low,
    TPTP_High,
    TPTP_Count
};

struct threadpool_queue_entry_t
{
    bool8                  is_valid;

    void                  *user_data;
    threadpool_callback_t *callback;
};

struct threadpool_queue_t
{
    volatile u32 completion_goal;
    volatile u32 entries_completed;

    volatile u32 next_entry_to_write;
    volatile u32 next_entry_to_read;

    threadpool_queue_entry_t entries[MAX_QUEUE_ENTRIES];
};

struct threadpool_t
{
    sys_semaphore_t semaphore;
    u32             threads_awake;
    u32             max_threads;
    
    threadpool_queue_t high_priority_queue;
    threadpool_queue_t low_priority_queue;
};

void  c_threadpool_init(threadpool_t *pool);
bool8 c_threadpool_add_task(threadpool_t *threadpool, void *user_data, threadpool_callback_t *callback, u32 priority);
bool8 c_threadpool_perform_next_task(threadpool_queue_t *queue);
void  c_threadpool_flush_queue(threadpool_queue_t *queue);
void  c_threadpool_flush_task_queues(threadpool_t *threadpool);

#endif // C_THREADPOOL_H

