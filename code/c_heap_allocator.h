#if !defined(C_HEAP_ALLOCATOR_H)
/* ========================================================================
   $File: c_heap_allocator.h $
   $Date: September 11 2026 04:11 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_HEAP_ALLOCATOR_H
#include <stdio.h>

#include <c_base.h>
#include <c_types.h>
#include <c_threadpool.h>

// NOTE(Sleepster): Athena doesn't like macros inside of structures and such... Ignore this file 
CODE_GEN_IGNORE_FILE

#define DEBUG_SECTION_ID (0xF0C0FFUL)
#define DEBUG_PAGE_ID    (0x000FF0CC)

#ifndef MAX_MEMORY_SECTIONS
constexpr u32     MAX_MEMORY_SECTIONS = 1024;
#endif
constexpr float32 PROTECTED_ALLOCATION_SIZE_FACTOR = 0.05;

constexpr u64     ALLOCATOR_DEFAULT_PAGE_SECTION_SIZE = MB(10);
constexpr u64     ALLOCATOR_MIN_UNIQUE_PAGE_SIZE      = MB(10);

struct memory_page_t;

thread_local s32 this_thread_index = -1;

#define MEMORY_ALLOCATOR_TAG_LIST(X) \
    X(ALLOCATOR_TAG_FREE, "ALLOCATOR_TAG_FREE", free) \
    X(ALLOCATOR_TAG_STATIC, "ALLOCATOR_TAG_STATIC", static) \
    X(ALLOCATOR_TAG_TEMP, "ALLOCATOR_TAG_TEMP", temp) \
    X(ALLOCATOR_TAG_CACHE, "ALLOCATOR_TAG_CACHE", cached) \
    X(ALLOCATOR_TAG_ENGINE, "ALLOCATOR_TAG_ENGINE", engine) \
    X(ALLOCATOR_TAG_BACKEND_RENDERER, "ALLOCATOR_TAG_BACKEND_RENDERER", backend_renderer) \
    X(ALLOCATOR_TAG_ASSET, "ALLOCATOR_TAG_ASSET", asset) \
    X(ALLOCATOR_TAG_RHI, "ALLOCATOR_TAG_RHI", RHI) \
    X(ALLOCATOR_TAG_UI, "ALLOCATOR_TAG_UI", UI) \
    X(ALLOCATOR_TAG_GAME, "ALLOCATOR_TAG_GAME", game) \

enum memory_allocator_tag_t
{
#define X(tag_enum, tag_string, total_member) tag_enum,
    MEMORY_ALLOCATOR_TAG_LIST(X)
#undef X
    TAG_COUNT
};

#pragma pack(push, 1)
struct memory_section_t 
{
    s32   ID;
    s32   memory_tag;
    u64   section_size;
#ifndef RELEASE 
    // NOTE(Sleepster): Here because in DEBUG mode we must
    // know the offset to the OS protected memory page.
    s64   user_allocation_size;
#else 
    void          *__Padding2;
#endif
    byte          *section_base;
    memory_page_t *owner_page;

    memory_section_t *next_section;
    memory_section_t *prev_section;
    void             *__Padding;
};

struct memory_page_t
{
    u64               ID;
    u64               page_size;
    memory_section_t  first_section;
    memory_section_t *cursor;
    byte             *page_base;
    union {
        u64 allocation_stats[TAG_COUNT];
        struct {
#define X(tag_enum, tag_string, total_member) u64 total_##total_member;
            MEMORY_ALLOCATOR_TAG_LIST(X)
#undef X
        };
    };

    // NOTE(Sleepster): The central allocator ignores the prev_page! 
    memory_page_t    *next_page;
    memory_page_t    *prev_page;
};
#pragma pack(pop)

struct tag_section_array_t 
{
    array_t<memory_section_t*, MAX_MEMORY_SECTIONS> array;
    s32 count;
};

struct allocator_thread_context_t
{
    memory_page_t      *first_page;
    memory_page_t      *current_page;
    tag_section_array_t tag_array[TAG_COUNT];
};

struct memory_allocator_t
{
    void        *memory;
    s64          os_page_size;
    volatile u64 max_capacity;
    volatile u64 next_page_offset;

    volatile s32               thread_count;
    allocator_thread_context_t thread_contexts[MAX_THREAD_COUNT];

    bool8                      is_initialized;
};

StaticAssert(sizeof(memory_section_t) % 16 == 0, "Because we use the heading of the allocation as storage for the memory section, it must be 16byte aligned to prevent SSE2 instructions (which require 16byte alignment) from blowing up");
StaticAssert(sizeof(memory_page_t) % 16 == 0, "Because we use the heading of the allocation as storage for the page section, it must be 16byte aligned to prevent SSE2 instructions (which require 16byte alignment) from blowing up");

ENGINE_API void  c_memory_allocator_init(void *base_address, u64 total_allocation);

ENGINE_API void *c_alloc(u64 size, s32 tag);
ENGINE_API void  c_free_alloc(void *memory);
ENGINE_API void  c_free_tagged_allocations(s32 tag);
ENGINE_API void  c_free_tagged_allocation_range(s32 min_tag, s32 max_tag);

template <typename T>
T*
c_alloc_type_(s32 tag)
{
    T *result = (T*)c_alloc(sizeof(T), tag);
    *result = {};

    return(result);
}

#define c_alloc_type(type, tag) c_alloc_type_<type>(tag)

#endif // C_HEAP_ALLOCATOR_H

