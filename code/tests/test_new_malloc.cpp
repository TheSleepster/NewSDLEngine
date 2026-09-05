/* ========================================================================
   $File: test_new_malloc.cpp $
   $Date: September 05 2026 10:19 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include "test_manager.h"

#include <c_string.cpp>
#include <c_memory_arena.cpp>
#include <p_platform_data.cpp>

#define MAIN
#include <sandbox/new_malloc.cpp>

constexpr u32 MAX_ALLOCATIONS = 1000;
void *allocations[MAX_ALLOCATIONS] = {};

TEST(OneKBAllocations)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)alloc(KB(1), TAG_STATIC);
        memset(allocation, 5, KB(1));

        allocations[index] = allocation;
    }
    print_allocator_info();
}

TEST(FreeingOneKBAllocations)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        free_alloc(allocations[index]);
    }
    print_allocator_info();
}

TEST(EnsureTagArrayIsEmpty)
{
    allocator_thread_context_t *context = &allocator.thread_contexts[0];
    tag_section_array_t static_tag_array = context->tag_array[TAG_STATIC];
    Assert(static_tag_array.count == 0);
}

TEST(GroupFreeingTagGroup)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)alloc(KB(1), TAG_STATIC);
        memset(allocation, 5, KB(1));
    }
    print_allocator_info();

    log_trace("Freeing Tags...\n");
    free_tagged_allocations(TAG_STATIC);
    print_allocator_info();
}

TEST(Allocation9MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)alloc(MB(9), TAG_STATIC);
        memset(allocation, 5, MB(9));

        allocations[index] = allocation;
    }
    print_allocator_info();
}

TEST(Freeing9MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        free_alloc(allocations[index]);
    }
    print_allocator_info();
}

TEST(EnsureTagArrayIsEmpty2)
{
    allocator_thread_context_t *context = &allocator.thread_contexts[0];
    tag_section_array_t static_tag_array = context->tag_array[TAG_STATIC];
    Assert(static_tag_array.count == 0);
}

TEST(AllocateCached5MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        alloc(MB(5), TAG_CACHE);
    }
}

TEST(ReclaimCachedMemory)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        alloc(MB(9), TAG_STATIC);
    }
}

TEST(ReallocateLargerPages)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        alloc(MB(20), TAG_STATIC);
    }
}

int
main(void)
{
    memory_allocator_init(null, GB(12));

    test_manager_t test_manager = {};
    test_manager_init(&test_manager);

    test_manager_run_tests(&test_manager);
    return(0);
}
