/* ========================================================================
   $File: test_new_malloc.cpp $
   $Date: September 05 2026 10:19 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include "test_manager.h"

#define MAIN
#include <sandbox/new_malloc.cpp>

constexpr u32 MAX_ALLOCATIONS = 100;
void *allocations[MAX_ALLOCATIONS] = {};

TEST(OneKBAllocations)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)c_alloc(KB(1), ALLOCATOR_TAG_STATIC);
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
        c_free_alloc(allocations[index]);
    }
    print_allocator_info();
}

TEST(EnsureTagArrayIsEmpty)
{
    allocator_thread_context_t *context = &allocator.thread_contexts[0];
    tag_section_array_t static_tag_array = context->tag_array[ALLOCATOR_TAG_STATIC];
    Assert(static_tag_array.count == 0);
}

TEST(GroupFreeingTagGroup)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)c_alloc(KB(1), ALLOCATOR_TAG_STATIC);
        memset(allocation, 5, KB(1));
    }
    print_allocator_info();

    log_trace("Freeing Tags...\n");
    c_free_tagged_allocations(ALLOCATOR_TAG_STATIC);
    print_allocator_info();
}

TEST(Allocation8MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)c_alloc(MB(8), ALLOCATOR_TAG_STATIC);
        memset(allocation, 5, MB(8));

        allocations[index] = allocation;
    }
    print_allocator_info();
}

TEST(Freeing8MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        c_free_alloc(allocations[index]);
    }
    print_allocator_info();
}

TEST(EnsureTagArrayIsEmpty2)
{
    allocator_thread_context_t *context = &allocator.thread_contexts[0];
    tag_section_array_t static_tag_array = context->tag_array[ALLOCATOR_TAG_STATIC];
    Assert(static_tag_array.count == 0);
}

TEST(AllocateCached5MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)c_alloc(MB(5), ALLOCATOR_TAG_CACHE);
        memset(allocation, 5, MB(5));

        allocations[index] = allocation;
    }
}

TEST(ReclaimCachedMemory)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)c_alloc(MB(9), ALLOCATOR_TAG_STATIC);
        allocations[index] = allocation;
    }

    c_free_tagged_allocations(ALLOCATOR_TAG_CACHE);
}

TEST(FreeReclaimedMemory)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        c_free_alloc(allocations[index]);
    }

    print_allocator_info();
}

TEST(ReallocateLargerPages)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        c_alloc(MB(20), ALLOCATOR_TAG_STATIC);
    }
}

int
main(void)
{
    c_memory_allocator_init(null, GB(3));
    test_manager_run_tests();

    return(0);
}
