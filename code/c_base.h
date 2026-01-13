#if !defined(C_BASE_H)
/* ========================================================================
   $File: c_base.h $
   $Date: November 29 2025 06:28 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_BASE_H
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdlib.h>

// TODO(Sleepster): Instruction set support (SSE2, SSE3, AVX, AVX2, etc...)

// MSVC
#if defined(_MSC_VER)
#define COMPILER_CL 1
#if defined(_WIN_32)
#define OS_WINDOWS 1
#endif

// ARCH
#if defined(_M_AMD64)
# define ARCH_X64 1
#elif defined(_M_I86)
# define ARCH_X86 1
#elif defined(_M_ARM)
# define ARCH_ARM 1
#else
# error ARCH not found...
#endif

// CLANG
#elif defined(__clang__)
# define COMPILER_CLANG 1
#if defined(_WIN_32)
# define OS_WINDOWS 1
#elif defined(__gnu_linux__)
# define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
# define OS_MAC 1
#else
# error OS not detected...
#endif

// ARCH
#if defined(__amd64__)
# define ARCH_X64 1
#  elif defined(__i386__)
# define ARCH_X86 1
#  elif defined(__arm__)
# define ARCH_ARM 1
#  elif defined(__aarch_64__)
# define ARCH_ARM64 1
#else
# error x86/x64 is the only supported architecture at the moment...
#endif

// GNU C
#elif defined(__GNUC__)
# define COMPILER_GCC 1
#if defined(_WIN_32)
# define OS_WINDOWS 1
#elif defined(__gnu_linux__)
# define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
# define OS_MAC 1
#else
# error OS not detected...
#endif
    
// ARCH
#if defined(__amd64__)
# define ARCH_X64 1
#elif defined(__i386__)
# define ARCH_X86 1
#elif defined(__arm__)
# define ARCH_ARM 1
#elif defined(__aarch_64__)
# define ARCH_ARM64 1
#else
# error "cannot find ARCH..."
#endif
#else
# error "unable to distinguish this compiler..."
#endif

// COMPILERS
#if !defined(COMPILER_CL)
# define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif

// OPERATING SYSTEMS
#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(OS_MAC)
# define OS_MAC 0
#endif

// ARCH
#if !defined(ARCH_X64)
# define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
# define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
# define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
# define ARCH_ARM64 0
#endif

#if !defined(ASSERT_ENABLED)
# define ASSERT_ENABLED
#endif

///////////////////////////////////
// NOTE(Sleepster): HELPER MACROS 
#define Statement(x)                 do{x}while(0)

#define GlueHelper(A, B)             A##B
#define Glue(A, B)                   GlueHelper(A, B)

#define ArrayCount(x)                (sizeof(x) / sizeof((x[0])))
#define IntFromPtr(x)                ((u32)    ((char *)x - (char*)0))
#define PtrFromInt(x)                ((void *) ((char *)0 + (x)))

#define MemberHelper(type, member)   (((type *)0)->member)
#define OffsetOf(type, member)       (&MemberHelper(type, member))
#define OffsetOfInt(type, member)    (IntFromPtr(OffsetOf(type, member)))

#define ZeroMemory(data, size)       (memset((data), 0, (size)))
#define ZeroStruct(type)             (ZeroMemory(&type, sizeof(type)))

#define MemoryCopy(dest, source, size) (memmove((dest), (source), (size)))
#define MemoryCopyStruct(dest, source) (MemoryCopy((dest), (source), Min(sizeof(*(dest), sizeof(*(source))))))

#define InvalidCodePath Assert(false)

#define Align(value, number) ((value + ((number) - 1))  & ~((number) - 1))
#define Align4(value)  ((value + 3)  & ~3)
#define Align8(value)  ((value + 7)  & ~7)
#define Align16(value) ((value + 15) & ~15)

#include "c_types.h"

global_variable s8  S8_MIN  = (s8) 0x80;
global_variable s16 S16_MIN = (s16)0x8000;
global_variable s32 S32_MIN = (s32)0x80000000;
global_variable s64 S64_MIN = (s64)0x8000000000000000llu;

global_variable s8  S8_MAX  = (s8) 0x7f;
global_variable s16 S16_MAX = (s16)0x7ffff;
global_variable s32 S32_MAX = (s32)0x7ffffffff;
global_variable s64 S64_MAX = (s64)0x7fffffffffffffffllu;

global_variable u8  U8_MIN  = (u8) 0x00;
global_variable u16 U16_MIN = (u16)0x0000;
global_variable u32 U32_MIN = (u32)0x00000000;
global_variable u64 U64_MIN = (u64)0x0000000000000000llu;

global_variable u8  U8_MAX  = (u8) 0xff;
global_variable u16 U16_MAX = (u16)0xfffff;
global_variable u32 U32_MAX = (u32)0xffffffff;
global_variable u64 U64_MAX = (u64)0xFFFFFFFFllu;

global_variable float32 machine_epsilon_f32 = 1.1920929e-7f;
global_variable float64 machine_epsilon_f64 = 2.220446e-16;

typedef void void_func(void);

#ifdef ASSERT_ENABLED
#define AssertBreak       (*(char*)0 = 0)

#define StaticAssert(cond, msg) static_assert(cond, msg) 
#define Expect(cond, ...) if(!(cond)) { fprintf(stderr, "FILE: [%s], LINE: '%d':\t", __FILE__, __LINE__); fprintf(stderr, ##__VA_ARGS__); getchar(); AssertBreak;}
#define Assert(cond)      if(!(cond)) { fprintf(stderr, "FILE: [%s], LINE: '%d': Assertion failed:...\n", __FILE__, __LINE__); AssertBreak;}
#else
#define StaticAssert(cond, msg)
#define Expect(cond, ...)
#define Assert(cond)
#endif

// NOTE(Sleepster): By default virtual memory pages are zeroed for you. 
// Why malloc doesn't do this is beyond me. Probably has to do with speed...
#define TypeOf(type) __typeof__(type)

#define Alloc(type) ({                    \
    void *_result = malloc(sizeof(type)); \
    ZeroMemory(_result, sizeof(type));    \
                                          \
    (type*)_result;                       \
})

#define AllocArray(type, count) ({                \
    void *_result = malloc(sizeof(type) * count); \
    ZeroMemory(_result, sizeof(type) * count);    \
                                                  \
    (type*)_result;                               \
})

#define AllocSize(size) ({        \
    void *_result = malloc(size); \
    ZeroMemory(_result, size);    \
                                  \
    _result;                      \
})

#define KB(x) ((u64)(x) * 1024ULL)
#define MB(x) (KB((x))  * 1024ULL)
#define GB(x) (MB((x))  * 1024ULL)

#define INVALID_ID ((u32)-1)

// NOTE(Sleepster): C++
#include <type_traits>
#define TypesSame(A, B) std::is_same_v<TypeOf(A), TypeOf(B)>

// For some reason, if you DEREFERENCE A POINTER, decltype still tells you it's a reference...
//#define TypesSame(A, B) std::is_same_v<decltype(A), decltype(B)>


#endif // C_BASE_H

