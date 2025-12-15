#if !defined(C_INTRINSICS_H)
/* ========================================================================
   $File: c_intrinsics.h $
   $Date: Tue, 29 Jul 25: 05:18PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_INTRINSICS_H
#include <c_base.h>
#include <c_types.h>

/*
  TODO:
  - MSVC native intrinsics (????)
  - ARM NEON intrinsics
  - clean this up so that we can just use the clang/gcc intrinsics for all platforms, 
    but then have a section for x64 specific like __rdtsc and __readgsqword()
 */

#if defined COMPILER_CLANG || defined COMPILER_GCC
    #define restrict __restrict

    #if ARCH_X64
        #include <emmintrin.h>
        #include <xmmintrin.h>
        #include <immintrin.h>
        #include <x86intrin.h>

        #if OS_WINDOWS 
            // NOTE(Sleepster): Read time stamp counter...
            #define rdtsc()                  __rdtsc()
            #define rdtscp(processor_id_out) __rdtscp(processor_id_out)

            // NOTE(Sleepster): 3 intructions to get the thread ID as opposed to the 8 from GetCurrentThreadID on Windows
            #define GetThreadID() *((u32*)(((u8*)__readgsqword(0x30)) + 0x48))
            #define true_inline __forceinline
        #elif OS_LINUX
            #include <unistd.h>
            // NOTE(Sleepster): Read time stamp counter...
            #define rdtsc()                  __rdtsc()
            #define rdtscp(processor_id_out) (0) 

            // NOTE(Sleepster): 3 intructions to get the thread ID as opposed to the 8 from GetCurrentThreadID on Windows
            #define GetThreadID() gettid() 
            #define true_inline __attribute__((always_inline))
        #elif OS_MAC
            #error "LMAO what hte fuck are you doing here???"
        #endif

        #define PopCount16(value) __builtin_popcount((s16)value)
        #define PopCount32(value) __builtin_popcount((s32)value)
        #define PopCount64(value) __builtin_popcount((s64)value)

    /* ===========================================
       ================== FENCES =================
       ===========================================*/

        /* NOTE(Sleepster): These are CPU instructions to inform the cpu that reads/writes shouldn't be rearranged.
        Not that this really matters on x86 CPUs since unlike ARM, reads and writes have an enforced order. 
        */
        #define mfence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
        #define sfence() __atomic_thread_fence(__ATOMIC_RELEASE)
        #define lfence() __atomic_thread_fence(__ATOMIC_ACQUIRE)

        // NOTE(Sleepster): These are COMPILER barriers. "Don't move reads/writes above this line" 
        #define ReadBarrier       __asm__ __volatile__("" ::: "memory")
        #define WriteBarrier      __asm__ __volatile__("" ::: "memory")
        #define ReadWriteBarrier  __asm__ __volatile__("" ::: "memory")

    /* =============================================================
       ====================== ATOMIC INCREMENT =====================
       ============================================================= */
        // NOTE(Sleepster): Increments the value, returns the newly incremented value. 
        #define AtomicIncrement16(ptr) __atomic_fetch_add((volatile s16*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicIncrement32(ptr) __atomic_fetch_add((volatile s32*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicIncrement64(ptr) __atomic_fetch_add((volatile s64*)(ptr), 1, __ATOMIC_SEQ_CST)

        #define AtomicIncrement(ptr) AtomicIncrement32(ptr)

    /* =============================================================
       ====================== ATOMIC DECREMENT =====================
       ============================================================= */
        // NOTE(Sleepster): Decrements the value, returns the newly decremented value. 
        #define AtomicDecrement16(ptr) __atomic_fetch_sub((volatile s16*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicDecrement32(ptr) __atomic_fetch_sub((volatile s32*)(ptr), 1, __ATOMIC_SEQ_CST)
        #define AtomicDecrement64(ptr) __atomic_fetch_sub((volatile s64*)(ptr), 1, __ATOMIC_SEQ_CST)

        #define AtomicDecrement(ptr) AtomicDecrement32(ptr)

    /* =============================================================
       ======================== ATOMIC ADD =========================
       ============================================================= */
        // NOTE(Sleepster): Adds the value, returns the new sum of the values. 
        #define AtomicAdd16(ptr, value) __atomic_add_fetch((volatile s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
        #define AtomicAdd32(ptr, value) __atomic_add_fetch((volatile s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
        #define Atomicadd64(ptr, value) __atomic_add_fetch((volatile s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

        #define AtomicAdd(ptr, value) AtomicAdd32(ptr, value)

    /* =============================================================
       =================== ATOMIC EXCHANGE ADD =====================
       ============================================================= */
        // NOTE(Sleepster): Gets the value in the register, calaculates the sum using the addend. Returns the pre-summed value. 
        #define AtomicExchangeAdd16(ptr, value) __atomic_fetch_add((volatile s16*)(ptr), (s16)value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeAdd32(ptr, value) __atomic_fetch_add((volatile s32*)(ptr), (s32)value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeAdd64(ptr, value) __atomic_fetch_add((volatile s64*)(ptr), (s64)value, __ATOMIC_SEQ_CST)

        #define AtomicExchangeAdd(ptr, value) AtomicAdd32(ptr, value)

        // NOTE(Sleepster): Technically not a real intrinsic but convienence is king.
    /* =============================================================
       ====================== ATOMIC SUBTRACT ======================
       ============================================================= */

        // NOTE(Sleepster): Subtracts the value, returns the new difference of the values. 
        #define AtomicSubtract16(ptr, value) __atomic_add_fetch((volatile s16*)(ptr), (s16)-value, __ATOMIC_SEQ_CST)
        #define AtomicSubtract32(ptr, value) __atomic_add_fetch((volatile s32*)(ptr), (s32)-value, __ATOMIC_SEQ_CST)
        #define AtomicSubtract64(ptr, value) __atomic_add_fetch((volatile s64*)(ptr), (s64)-value, __ATOMIC_SEQ_CST)

        #define AtomicSubtract(ptr, value) AtomicSubtract32(ptr, val) 

    /* =============================================================
       ================= ATOMIC EXCHANGE SUBTRACT ==================
       ============================================================= */
        // NOTE(Sleepster): Gets the value in the register, calaculates the difference. Returns the pre-summed value. 
        #define AtomicExchangeSubtract16(ptr, value) __atomic_fetch_add((volatile s16*)(ptr), (s16)-value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeSubtract32(ptr, value) __atomic_fetch_add((volatile s32*)(ptr), (s32)-value, __ATOMIC_SEQ_CST)
        #define AtomicExchangeSubtract64(ptr, value) __atomic_fetch_add((volatile s64*)(ptr), (s64)-value, __ATOMIC_SEQ_CST)

        #define AtomicExchangeSubtract(ptr, value) AtomicExchangeSubtract32(ptr, val) 

    /* =============================================================
       ====================== ATOMIC EXCHANGE ======================
       ============================================================= */
        // NOTE(Sleepster): Returns the value in the register, swaps the 'value' and the contents of 'ptr'
        #define AtomicExchange16(ptr, val) __atomic_exchange_n((volatile s16*)(ptr), (s16)(val), __ATOMIC_SEQ_CST)
        #define AtomicExchange32(ptr, val) __atomic_exchange_n((volatile s32*)(ptr), (s32)(val), __ATOMIC_SEQ_CST)
        #define AtomicExchange64(ptr, val) __atomic_exchange_n((volatile s64*)(ptr), (s64)(val), __ATOMIC_SEQ_CST)

        #define AtomicExchange(ptr, val) AtomicExchange32(ptr, val)
        
    /* =============================================================
       ======================== ATOMIC LOAD ========================
       ============================================================= */

        #define AtomicLoad16(ptr) __atomic_load_n((volatile s16 *)ptr, __ATOMIC_ACQUIRE)
        #define AtomicLoad32(ptr) __atomic_load_n((volatile s32 *)ptr, __ATOMIC_ACQUIRE)
        #define AtomicLoad64(ptr) __atomic_load_n((volatile s64 *)ptr, __ATOMIC_ACQUIRE)

        #define AtomicLoad(ptr) __atomic_load_n((volatile s32 *)ptr, __ATOMIC_ACQUIRE)

    /* =============================================================
       ======================== ATOMIC STORE========================
       ============================================================= */

       #define AtomicStore16(ptr, val) __atomic_store_n((volatile s16*)ptr, val, __ATOMIC_RELEASE)
       #define AtomicStore32(ptr, val) __atomic_store_n((volatile s32*)ptr, val, __ATOMIC_RELEASE)
       #define AtomicStore64(ptr, val) __atomic_store_n((volatile s64*)ptr, val, __ATOMIC_RELEASE)

       #define AtomicStore(ptr, val) __atomic_store_n((volatile s32*)ptr, val, __ATOMIC_RELEASE)

    /* =============================================================
       =================== ATOMIC COMPARE EXCHANGE =================
       ============================================================= */
        #define AtomicCompareExchange16(ptr, exchange, comparand) ({        \
            s16 _expected = (s16)(comparand);                               \
            __atomic_compare_exchange_n((volatile s16*)(ptr), &_expected,   \
                                        (s16)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange32(ptr, exchange, comparand) ({        \
            s32 _expected = (s32)(comparand);                               \
            __atomic_compare_exchange_n((volatile s32*)(ptr), &_expected,   \
                                        (s32)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange64(ptr, exchange, comparand) ({        \
            int64_t _expected = (s64)(comparand);                           \
            __atomic_compare_exchange_n((volatile s64*)(ptr), &_expected,   \
                                        (s64)(exchange), 0,                 \
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);\
            _expected;                                                      \
        })

        #define AtomicCompareExchange(ptr, exchange, comparand) AtomicCompareExchange32(ptr, exchange, comparand)
    #else
        #error "only x64 is supported..."
    #endif
#else
    #error "Only GCC/Clang are supported..."
#endif

#endif
