#if !defined(C_SYNCHRONIZATION_H)
/* ========================================================================
   $File: c_synchronization.h $
   $Date: December 08 2025 08:03 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_SYNCHRONIZATION_H

#if OS_WINDOWS
    typedef void* sys_semaphore_handle_t;
    typedef void* sys_mutex_handle_t;
    typedef void* sys_thread_handle_t;
#elif OS_LINUX
#include <SDL3/SDL.h>

typedef SDL_Semaphore* sys_semaphore_handle_t;
typedef SDL_Mutex*     sys_mutex_handle_t;
typedef SDL_Thread*    sys_thread_handle_t;
#elif OS_MAC
#error "not implemented...\n"
#endif

#include <c_base.h>
#include <c_types.h>
#include <c_intrinsics.h>

typedef struct sys_thread
{
    sys_thread_handle_t handle;
    u32                 thread_id;
    void               *user_data;
}sys_thread_t;

typedef struct sys_mutex
{
    sys_mutex_handle_t handle;
}sys_mutex_t;

typedef struct sys_semaphore
{
    sys_semaphore_handle_t handle;
}sys_semaphore_t;

// NOTE(Sleepster): 
// Ticket Mutex. Super simple. Potentially faster than an OS mutex since you avoid the overhead of the OS scheduler... 
// But be careful beause spinlocking for long periods of time is really really bad...

struct ticket_mutex_t
{
    volatile u64 next_ticket;
    volatile u64 working_ticket;
};

inline u64
c_ticket_mutex_take_ticket(ticket_mutex_t *mutex)
{
    u64 result = 0;
    result = AtomicIncrement64(&mutex->next_ticket);
    return(result);
}

inline bool8
c_ticket_mutex_try_wait(ticket_mutex_t *mutex, u64 ticket)
{
    bool8 result = false;
    if((u64)AtomicLoad64(&mutex->working_ticket) == ticket)
    {
        result = true;
    }

    return(result);
}

inline void
c_ticket_mutex_wait(ticket_mutex_t *mutex, u64 ticket)
{
    while(!c_ticket_mutex_try_wait(mutex, ticket));
}

inline void
c_ticket_mutex_advance_ticket(ticket_mutex_t *mutex)
{
    AtomicIncrement64(&mutex->working_ticket);
}

inline void
c_ticket_mutex_take_and_wait(ticket_mutex_t *mutex)
{
    u64 ticket = c_ticket_mutex_take_ticket(mutex);
    c_ticket_mutex_wait(mutex, ticket);
}

#define TicketMutexScope(mutex) DeferLoop(c_ticket_mutex_take_and_wait((mutex)), c_ticket_mutex_advance_ticket((mutex)))

#endif // C_SYNCHRONIZATION_H

