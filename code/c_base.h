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

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

///////////////////////////////////
// NOTE(Sleepster): HELPER MACROS 
#define Min(a, b) (((a) > (b)) ? (b) : (a))
#define Max(a, b) (((a) < (b)) ? (b) : (a))

#define Statement(x)                 do{x}while(0)

#define GlueHelper(A, B)             A##B
#define Glue(A, B)                   GlueHelper(A, B)

#define ArrayCount(x)                (sizeof(x) / sizeof(*(x)))
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

// NOTE(Sleepster): Linux Kernel compile time assert! It unfortunately only tells you if the the condition fails... not how it fails... Unlucky!
#define StaticAssert(condition) ((void)sizeof(char[2*!!(condition) - 1]))

#define Align4(value) ((value  + 3)  & ~3)
#define Align8(value) ((value  + 7)  & ~7)
#define Align16(value) ((value + 15) & ~15)

#include "c_types.h"

global_variable s8  MIN_S8  = (s8) 0x80;
global_variable s16 MIN_S16 = (s16)0x8000;
global_variable s32 MIN_S32 = (s32)0x80000000;
global_variable s64 MIN_S64 = (s64)0x8000000000000000llu;

global_variable s8  MAX_S8  = (s8) 0x7f;
global_variable s16 MAX_S16 = (s16)0x7ffff;
global_variable s32 MAX_S32 = (s32)0x7ffffffff;
global_variable s64 MAX_S64 = (s64)0x7fffffffffffffffllu;

global_variable u8  MIN_U8  = (u8) 0x80;
global_variable u16 MIN_U16 = (u16)0x8000;
global_variable u32 MIN_U32 = (u32)0x80000000;
global_variable u64 MIN_U64 = (u64)0x8000000000000000llu;

global_variable u8  MAX_U8  = (u8) 0x7f;
global_variable u16 MAX_U16 = (u16)0x7ffff;
global_variable u32 MAX_U32 = (u32)0x7fffffff;
global_variable u64 MAX_U64 = (u64)0x7fffffffffffffffllu;

global_variable float32 machine_epsilon_f32 = 1.1920929e-7f;
global_variable float64 machine_epsilon_f64 = 2.220446e-16;

typedef void void_func(void);

#define AssertBreak       (*(char*)0 = 0)
#define Expect(cond, ...) if(!(cond)) { AssertBreak; fprintf(stderr, ##__VA_ARGS__); }
#define Assert(cond)      if(!(cond)) { AssertBreak; }

#define Alloc(type)             (type*)malloc(sizeof(type))
#define AllocArray(type, count) (type*)malloc(sizeof(type) * count)
#define AllocSize(size)                malloc(size)


#endif // C_BASE_H

