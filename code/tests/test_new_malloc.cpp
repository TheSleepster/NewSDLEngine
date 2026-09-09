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

TEST(Allocation8MBSections)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        int *allocation = (int*)alloc(MB(8), TAG_STATIC);
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
        int *allocation = (int*)alloc(MB(5), TAG_CACHE);
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
        int *allocation = (int*)alloc(MB(9), TAG_STATIC);
        allocations[index] = allocation;
    }

    free_tagged_allocations(TAG_CACHE);
}

TEST(FreeReclaimedMemory)
{
    for(u32 index = 0;
        index < MAX_ALLOCATIONS;
        ++index)
    {
        free_alloc(allocations[index]);
    }

    print_allocator_info();
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
    printf("Hello, World!\n");
    memory_allocator_init(null, GB(3));
    test_manager_run_tests();

    return(0);
}
