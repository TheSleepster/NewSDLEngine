/* ========================================================================
   $File: new_malloc.cpp $
   $Date: April 24 2026 02:49 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <c_base.h>
#include <c_types.h>
#include <p_platform_data.h>

#define DEBUG_SECTION_ID (0xF0C0FFUL)
#define DEBUG_PAGE_ID    (0x000FF0CC)

constexpr u32     MAX_MEMORY_SECTIONS = 1024;
constexpr float32 PROTECTED_ALLOCATION_SIZE_FACTOR = 0.05;

constexpr u64     ALLOCATOR_DEFAULT_PAGE_SECTION_SIZE = MB(10);
constexpr u64     ALLOCATOR_MIN_UNIQUE_PAGE_SIZE      = MB(10);

struct memory_page_t;

#define DEBUG 1 

thread_local s32 this_thread_index = -1;

enum memory_allocator_tag_t
{
    TAG_CLEAR,            // unused section
    TAG_STATIC,           // manually freed
    TAG_TEMP,             // "garbage collected"
    TAG_CACHE,            // can be reclaimed whenever
    TAG_COUNT
};

struct memory_section_t 
{
    s32   ID;
    s32   memory_tag;
    u64   section_size;
#ifdef DEBUG
    // NOTE(Sleepster): Here because in DEBUG mode we must
    // know the offset to the OS protected memory page.
    s64   user_allocation_size;
#endif
    byte          *section_base;
    memory_page_t *owner_page;

    memory_section_t *next_section;
    memory_section_t *prev_section;
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
            u64 total_free;
            u64 total_cached;
            u64 total_temp;
            u64 total_static;
        };
    };

    // NOTE(Sleepster): The central allocator ignores the prev_page! 
    memory_page_t    *next_page;
    memory_page_t    *prev_page;

    bool32            in_use;
};

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
    void             *memory;
    s64               os_page_size;
    volatile u64      max_capacity;
    volatile s32      thread_count;

    volatile  u64     next_page_offset;
    allocator_thread_context_t thread_contexts[MAX_THREAD_COUNT];
};

static memory_allocator_t allocator = {};

static void *alloc(u64 size, s32 tag);
static void  free_alloc(void *memory);
static void  free_tagged_allocations(s32 tag);
static void  free_tagged_allocation_range(s32 min_tag, s32 max_tag);

/*
==============================================
get_tag_name
==============================================
*/

static char*
get_tag_name(s32 tag)
{
    char *result = null;
    switch(tag)
    {
        case TAG_CLEAR:  { result = "TAG_CLEAR";  }break;
        case TAG_STATIC: { result = "TAG_STATIC"; }break;
        case TAG_TEMP:   { result = "TAG_TEMP";   }break;
        case TAG_CACHE:  { result = "TAG_CACHE";  }break;
    }

    return(result);
}

/*
==============================================
print_allocator_section_list
==============================================
*/

static void
print_allocator_info(void)
{
    allocator_thread_context_t *context = allocator.thread_contexts + this_thread_index;

    s32 section_count = 0;
    s32 page_count    = 0;
    u64 total_zone_usage_size = 0;
    log_info("Allocator Section List:\n");

    memory_page_t *current_page = context->first_page;
    while(current_page)
    {
        ++page_count;
        log_info("\tPage Boundary:\n");
        memory_section_t *current_section = &current_page->first_section; 
        do {
            char *tag_name = get_tag_name(current_section->memory_tag);
            log_info("\t\tSection at '%p': size = %llu, allocated = '%s', tag = '%s', id = '%u'...\n",
                     current_section, 
                     current_section->section_size, 
                     current_section->memory_tag != TAG_CLEAR ? "true" : "false",
                     tag_name,
                     current_section->ID);

            ++section_count;
            total_zone_usage_size += current_section->section_size;

            current_section = current_section->next_section;
        }while(current_section != &current_page->first_section);

        log_info("\tPage Info:\n");
        log_info("\t\tTotal size free: '%llu'...\n", current_page->total_free);
        log_info("\t\tTotal size cached: '%llu'...\n", current_page->total_cached);
        log_info("\t\tTotal size temp: '%llu'...\n", current_page->total_temp);
        log_info("\t\tTotal size static: '%llu'...\n", current_page->total_static);

        u64 total_bytes_used = (u64)(current_page->total_free + current_page->total_cached + current_page->total_temp + current_page->total_static);
        Assert(total_bytes_used == current_page->page_size);

        current_page = current_page->next_page;
    }

    log_info("\n");
    log_info("Allocator Stats:\n");
    log_info("\tTotal Number of Sections: '%d'...\n", section_count);
    log_info("\tMemory used by sections: '%d'...\n", section_count * sizeof(memory_section_t));
    log_info("\tTotal Pages: '%d'...\n", page_count);
    log_info("\n");
    log_info("\tAllocator size is: '%llu'...\n", allocator.max_capacity);
    log_info("\tTotal size used by zones: '%llu'...\n", total_zone_usage_size);

    u64 total_size = total_zone_usage_size + (section_count * sizeof(memory_section_t));
    log_info("\tTotal size used (with sizeof(memory_section_t)): '%llu'...\n", total_size);
    if(total_size > allocator.max_capacity)
    {
        log_error("The total size used by the allocator is greater than it's capacity!\n");
    }

    log_info("\n");
    log_info("Allocation Tag Information:\n");
    for(s32 tag_index = TAG_CLEAR;
        tag_index < TAG_COUNT;
        ++tag_index)
    {
        tag_section_array_t *array = context->tag_array + tag_index;
        if(array->count > 0)
        {
            log_info("\tAllocation Tag '%s':\n", get_tag_name(tag_index));
            log_info("\t\tAllocation count: '%d'\n", array->count);

            u64 total_size = 0;
            for(memory_section_t *section: array->array)
            {
                if(section)
                {
                    total_size += section->section_size;
                }
            }
            log_info("\t\tTotal size: '%llu'\n", total_size);
        }
    }
}

/*
==============================================
memory_allocator_init
==============================================
*/

static void
memory_allocator_init(void *base_address, u64 total_allocation)
{
    allocator.memory        = sys_allocate_memory(base_address, total_allocation);
    allocator.max_capacity  = total_allocation;
    allocator.os_page_size  = sys_get_virtual_memory_page_size();
    allocator.next_page_offset = allocator.os_page_size;
}

/*
==============================================
thread_get_next_page
==============================================
*/

static memory_page_t*
thread_get_next_page(u64 new_page_size)
{
    memory_page_t *result = null;
    new_page_size = (Align(Max(new_page_size + sizeof(memory_page_t), ALLOCATOR_DEFAULT_PAGE_SECTION_SIZE), allocator.os_page_size));

    u64 next_page_offset = 1; 
    u64 new_next_page_offset = 2;
    u64 this_page_offset = 0;
    while(this_page_offset != next_page_offset)
    {
        next_page_offset     = AtomicLoad64(&allocator.next_page_offset);
        new_next_page_offset = next_page_offset + new_page_size;

        this_page_offset = AtomicCompareExchange64(&allocator.next_page_offset,
                                                    new_next_page_offset,
                                                    next_page_offset);
    }

    if(allocator.next_page_offset <= (u64)allocator.max_capacity)
    {
        result = (memory_page_t*)((byte*)allocator.memory + next_page_offset);
        ZeroStruct(*result);

        result->page_base = (byte*)result + sizeof(memory_page_t);

        result->first_section.ID           = DEBUG_SECTION_ID;
        result->first_section.memory_tag   = TAG_CLEAR;
        result->first_section.section_base = result->page_base;
        result->first_section.section_size = new_page_size - sizeof(memory_page_t);
        result->first_section.owner_page   = result;

        result->first_section.next_section = &result->first_section;
        result->first_section.prev_section = &result->first_section;
        result->cursor                     = &result->first_section;
        result->total_free                 = new_page_size;

        result->next_page = null;
        result->prev_page = null;

        result->page_size = new_page_size;
        result->ID        = DEBUG_PAGE_ID;
    }

    return(result);
}

/*
==============================================
register_thread_for_allocator
==============================================
*/

static void
register_thread_for_allocator(void)
{
    this_thread_index = AtomicIncrement32(&allocator.thread_count);
    allocator_thread_context_t *context = allocator.thread_contexts + this_thread_index;
    context->first_page   = thread_get_next_page(ALLOCATOR_DEFAULT_PAGE_SECTION_SIZE);
    context->current_page = context->first_page;

    context->current_page->next_page = null;
    context->current_page->prev_page = null;

    context->current_page->first_section.owner_page = context->current_page;
}

/*
==============================================
alloc_impl

Used in debug build, meant to prevent us from performing
buffer overflows and such
==============================================
*/

static void*
alloc_impl(u64 size, s32 tag)
{
    void *result = null;
    if(this_thread_index == -1)
    {
        register_thread_for_allocator();
    }
    
    allocator_thread_context_t *context = allocator.thread_contexts + this_thread_index;

#if DEBUG
    u64 user_allocation_size  = Align((size + sizeof(memory_section_t)), allocator.os_page_size);
    u64 total_allocation_size = user_allocation_size + allocator.os_page_size; 
#else
    u64 total_allocation_size = size;
#endif

    while(!result) 
    {
        memory_section_t *valid_section = null;

        // NOTE(Sleepster): Grab from free_list, but don't remove it yet. We remove it below... 
        tag_section_array_t *free_list = context->tag_array + TAG_CLEAR;
        if(free_list->count > 0)
        {
            for(s32 section_index = 0;
                section_index < free_list->count;
                ++section_index)
            {
                memory_section_t *current_section = free_list->array[section_index];
                if(current_section->section_size >= total_allocation_size)
                {
                    valid_section = free_list->array[section_index];
                    context->current_page = valid_section->owner_page;

                    break;
                }
            }
        }

        memory_section_t *current_section  = context->current_page->cursor;
        memory_section_t *starting_section = context->current_page->cursor->prev_section;
        while(!valid_section)
        {
            // NOTE(Sleepster): Free cached blocks as they are found... 
            if(current_section->memory_tag == TAG_CACHE)
            {

                memory_section_t *this_section = current_section;
                current_section = this_section->prev_section;
                free_alloc(this_section->section_base);
            }
            else
            {
                // NOTE(Sleepster): See if this is both cleared and can hold our allocation... 
                if(current_section->memory_tag == TAG_CLEAR &&
                   current_section->section_size >= total_allocation_size)
                {
                    valid_section = current_section;
                    break;
                }

                current_section = current_section->next_section;
            }

            if(current_section == starting_section) 
            {
                break;
            }
        }

        if(valid_section)
        {   
            Assert(valid_section->section_size >= total_allocation_size);

            // NOTE(Sleepster): Remove the item from the free list 
            tag_section_array_t *tag_array = context->tag_array + TAG_CLEAR;
            array_view_t<memory_section_t*> view = tag_array->array;
            s32 index = c_array_find(view, &valid_section);
            if(index != -1)
            {
                c_array_remove(view, index, tag_array->count);
                --tag_array->count;
                Assert(tag_array->count >= 0);
            }

            // NOTE(Sleepster): The start of the guard page 
            s64 section_offset = valid_section->section_size - total_allocation_size;

            memory_section_t *allocation = (memory_section_t*)(valid_section->section_base + section_offset);
            valid_section->section_size -= total_allocation_size;

            allocation->ID = DEBUG_SECTION_ID;
            allocation->memory_tag   = tag;
            allocation->section_size = total_allocation_size;
            allocation->section_base = ((byte*)allocation + sizeof(memory_section_t)); // offset by sizeof(memory_section_t) for the user storage

            allocation->next_section = valid_section->next_section;
            allocation->prev_section = valid_section;
            allocation->owner_page   = valid_section->owner_page;
            valid_section->next_section->prev_section = allocation;
            valid_section->next_section = allocation;

            valid_section->owner_page->allocation_stats[tag]       += total_allocation_size;
            valid_section->owner_page->allocation_stats[TAG_CLEAR] -= total_allocation_size;
#if DEBUG
            allocation->user_allocation_size = user_allocation_size;
#endif
            result = (void*)allocation->section_base;
            if(tag != TAG_CLEAR)
            {
                tag_section_array_t *array = context->tag_array + tag;
                array->array[array->count++] = allocation;
            }

#if DEBUG 
            // NOTE(Sleepster): Start of the guard page. 
            void *protected_address = (byte*)allocation + user_allocation_size;
            Assert(sys_set_memory_access_flags(protected_address, allocator.os_page_size, OS_MEMORY_ACCESS_FLAG_NONE));
#endif
        }
        else
        {
            Assert(free_list->count == 0);
            //log_error("Failure to find a block of size: '%llu' for this allocation...\n", total_allocation_size);
            if(!context->current_page->next_page)
            {
                // We need a new page... Or some more memory so what can we do?:
                //
                // - Search the current pages we have to see if the allocation can fit in that page
                // - Combine pages that are empty (nothing but cached and freed) until the allocation CAN fit
                // - If we combined all adjacent pages and we still can't fit the allocation, THEN we get a new page.

                memory_page_t *old_current_page = context->current_page;
                memory_page_t *new_page         = thread_get_next_page(total_allocation_size);
                if(new_page)
                {
                    context->current_page = new_page;
                    context->current_page->first_section.owner_page = old_current_page;

                    new_page->next_page = old_current_page;
                    context->current_page->prev_page = old_current_page;

                    tag_section_array_t *array = (context->tag_array + TAG_CLEAR);
                    array->array[array->count++] = &context->current_page->first_section;
                }
                else
                {
                    print_allocator_info();
                    Expect(false, "Allocator has run out of page space...\n");
                }
            }
            else
            {
                context->current_page = context->current_page->next_page;
            }
        }
    }

    return(result);
}

/*
==============================================
alloc
==============================================
*/

static void*
alloc(u64 size, s32 tag)
{
    void *result = alloc_impl(size, tag);
    return(result);
}

/*
==============================================
free_alloc

DEBUG VERSION
==============================================
*/

static void
free_alloc(void *memory)
{
    allocator_thread_context_t *context = allocator.thread_contexts + this_thread_index;

    memory_section_t *section = (memory_section_t*)((byte*)memory - sizeof(memory_section_t));
    Assert(section->ID == DEBUG_SECTION_ID);
    Assert(section->memory_tag != TAG_CLEAR);

#if DEBUG 
    void *protected_address = (byte*)section + section->user_allocation_size;
    Assert(sys_set_memory_access_flags(protected_address, allocator.os_page_size, OS_MEMORY_ACCESS_FLAG_READ|OS_MEMORY_ACCESS_FLAG_WRITE));
#endif

    // NOTE(Sleepster): Save these for when we're adjusting how much of the page
    // is being used by specific allocation tags.
    u32 section_tag       = section->memory_tag;
    u64 old_sections_size = section->section_size;

    tag_section_array_t *array = context->tag_array + section->memory_tag;
    // NOTE(Sleepster): Remove the allocation 
    s32 index = c_array_find(array->array, &section);
    if(index != -1)
    {
        c_array_remove(array->array, index, array->count);
        --array->count;
        Assert(array->count >= 0);
    }

    // NOTE(Sleepster): We have to set the section base to that of the actual allocation of the section since the
    // current section->section_base points to section_base + sizeof(memory_section_t), making us go over by one header.
    section->memory_tag   = TAG_CLEAR;
    section->section_base = (byte*)section;

    tag_section_array_t *free_list = context->tag_array + section->memory_tag;
    array_view_t<memory_section_t*> view = free_list->array;

    memory_section_t *cursor = section;
    if(section->next_section != section && 
       section->next_section->memory_tag == TAG_CLEAR &&
      (section->section_base + section->section_size) == section->next_section->section_base)
    {
        memory_section_t *next_section = section->next_section;
        s32 index = c_array_find(view, &next_section);
        if(index != -1)
        {
            c_array_remove(view, index, free_list->count);
            --free_list->count;
        }

        section->section_size += next_section->section_size;
        section->next_section  = next_section->next_section;

        next_section->next_section->prev_section = section;
    }

    if(section->prev_section != section && 
       section->prev_section->memory_tag == TAG_CLEAR &&
       section->prev_section->section_base + section->prev_section->section_size == section->section_base)
    {
        memory_section_t *previous_section = section->prev_section;
        previous_section->section_size += section->section_size;
        previous_section->next_section  = section->next_section;

        section->next_section->prev_section = previous_section;
        cursor = previous_section;
    }

    // NOTE(Sleepster): Add to the free list 
    free_list->array[free_list->count++] = cursor;

    section->owner_page->allocation_stats[section_tag] -= old_sections_size;
    section->owner_page->allocation_stats[TAG_CLEAR]   += old_sections_size;

    // NOTE(Sleepster): Adjust the page's cursor. 
    ((memory_page_t*)section->owner_page)->cursor = cursor;
}


/*
==============================================
free_tagged_allocations
==============================================
*/

static void
free_tagged_allocations(s32 tag)
{
    Assert(tag != TAG_CLEAR);

    allocator_thread_context_t *context = allocator.thread_contexts + this_thread_index;
    tag_section_array_t *array = context->tag_array + tag;

    memory_section_t *section = array->array[0];
    while(section)
    {
        void *address = section->section_base;
        free_alloc(address);

        section = array->array[0];
    }

    array->count = 0;
}

/*
==============================================
free_tagged_allocation_range
==============================================
*/

static void
free_tagged_allocation_range(s32 min_tag, s32 max_tag)
{
    for(s32 tag = min_tag;
        tag <= max_tag;
        ++tag)
    {
        free_tagged_allocations(tag);
    }
}

static
PLATFORM_THREAD_PROC(test_thread_proc)
{
    (void)user_data;
    for(s32 index = 0;
        index < 50;
        ++index)
    {
        alloc(MB(9), TAG_CACHE);
    }

    alloc(MB(9.5), TAG_STATIC);
    print_allocator_info();

    SDL_Delay(20);
    return(0);
}

#ifndef MAIN 
int
main(void)
{
    void *DEBUG_base_address = (void*)TB(2);
    memory_allocator_init(DEBUG_base_address, GB(5));

    log_info("Sizeof(memory) = %llu..\n", 1000);
    log_info("Sizeof(memory_section_t) = %llu..\n", sizeof(memory_section_t));

    //sys_thread_t handle = sys_thread_create(test_thread_proc, null, true);
    //(void)handle;

#if 0
    log_trace("1st...\n");
    print_allocator_info();
    free_tagged_allocation_range(TAG_STATIC, TAG_CACHE);
    log_trace("2nd...\n");
    print_allocator_info();
#endif

    SDL_Init(0);

    // NOTE(Sleepster): Single threaded... 
    u64 last_tick = SDL_GetTicks();
    for(u32 index = 0;
        index < 500;
        ++index)
    {
        malloc(MB(1));
    }
    u64 this_tick = SDL_GetTicks();
    u64 delta_ticks = this_tick - last_tick;

    int *allocations[1000] = {};
    last_tick = SDL_GetTicks();
    for(u32 index = 0;
        index < 500;
        ++index)
    {
        allocations[index] = (int*)alloc(MB(1), TAG_STATIC);
    }
    this_tick = SDL_GetTicks();

    u64 our_delta_ticks = this_tick - last_tick;
    log_info("Our time: '%llu'ms\n", our_delta_ticks);
    log_info("glibc time: '%llu'ms\n", delta_ticks);

    for(u32 index = 0;
        index < 500;
        ++index)
    {
        free_alloc(allocations[index]);
    }

    //free_tagged_allocation_range(TAG_STATIC, TAG_CACHE);
    //print_allocator_info();

    SDL_Delay(20);
    return(0);
}
#endif
