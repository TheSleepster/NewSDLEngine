#if !defined(C_TYPES_H)
/* ========================================================================
   $File: c_types.h $
   $Date: November 28 2025 09:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_TYPES_H
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdlib.h>

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef s8       bool8;
typedef s32      bool32;

typedef size_t   usize;
typedef u8       byte;

typedef float    float32;
typedef double   float64;

typedef float    real32;
typedef double   real64;

#define AssertBreak (*(char*)0 = 0)
#define Expect(cond) if(!(cond)) { AssertBreak; }

#define global        static
#define local_persist static
#define internal      static

#define null          NULL

#define external      extern "C"


#define Alloc(type)             (type*)malloc(sizeof(type))
#define AllocArray(type, count) (type*)malloc(sizeof(type) * count)
#define AllocSize(size)                malloc(size)

#endif // C_TYPES_H

