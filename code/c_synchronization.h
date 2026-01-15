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

#endif // C_SYNCHRONIZATION_H

