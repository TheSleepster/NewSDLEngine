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

struct virtual_memory_region_t 
{
    void *memory_base;
    u64   reserved;
    u64   committed;
    u64   used;

    virtual_memory_region_t *next_region;
};

struct thread_heap_t
{
    virtual_memory_region_t *first_region;
    virtual_memory_region_t *last_region;
};

struct memory_allocation_t
{
    virtual_memory_region_t *region;
    u64                      allocation_size;
};

static thread_heap_t global_heap;
static u64           DEFAULT_HEAP_SIZE;


void *slalloc_impl(u64 allocation_size);

#define slalloc(allocation_size)                    slalloc_impl((allocation_size))
#define slalloc_struct(structure)       (structure*)slalloc_impl(sizeof((structure)))
#define slalloc_array(structure, count) (structure*)slalloc_impl(sizeof((structure)) * (count))

internal_api bool8 
commit_virtual_memory(virtual_memory_region_t *region, u64 commit_size)
{
    bool8 result = true;
    u64 committed_size = sys_align_to_page_size(commit_size);
    Expect(committed_size <= region->reserved, 
           "Attempted to commit more virtual memory than was reserved in this allocation. Reserved: '%lu', Requested Commit size: '%lu'...\n",
           region->reserved, committed_size);

    byte *commit_start     = (byte*)region->memory_base + region->committed;
    u64   full_commit_size = commit_size;
    if(mprotect(commit_start, full_commit_size, PROT_READ | PROT_WRITE) != 0)
    {
        int error = errno;
        log_fatal("Failure to commit '%lu' bytes: (%s) code: %d\n", commit_size, strerror(error), error);

        result = false;
    }

    region->committed += committed_size;

    return(result);
}

internal_api virtual_memory_region_t*
slalloc_create_heap(u64 reserve_size)
{
    virtual_memory_region_t *result = null;

    reserve_size = sys_align_to_page_size(reserve_size + sizeof(virtual_memory_region_t));
    void *base = mmap(0, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(base != MAP_FAILED)
    {
        u64 initial_commit = sys_align_to_page_size(sizeof(virtual_memory_region_t));
        if(mprotect(base, initial_commit, PROT_READ | PROT_WRITE) != 0)
        {
            int error = errno;
            log_fatal("Failure to commit '%lu' bytes: (%s) code: %d\n", reserve_size, strerror(error), error);
        }

        // NOTE(Sleepster): Acts as a header for managing the virtual memory; 
        virtual_memory_region_t *region = (virtual_memory_region_t*)base;
        region->memory_base = (byte*)base;
        region->reserved    = reserve_size;
        region->committed   = initial_commit;
        region->used        = sizeof(virtual_memory_region_t);

        result = region;
    }
    else
    {
        int error = errno;
        log_fatal("mmap reserve failed: (%s) code: %d\n", strerror(error), error);
    }

    if(!global_heap.first_region)
    {
        global_heap.first_region = result;
        global_heap.last_region  = result;
    }
    else
    {
        for(virtual_memory_region_t *current_region = global_heap.first_region;
           current_region;
           current_region = current_region->next_region)
        {
            if(current_region->next_region == null)
            {
                current_region->next_region = result;
                global_heap.last_region     = result;

                break;
            }
        }
    }

    return(result);
}

/*  
 *  items -> 2000 -> 4000
 *  items -> 2000 -> 8000
 */

void*
slalloc_impl(u64 allocation_size)
{
    void *result = null;
    virtual_memory_region_t *region = global_heap.last_region;
    if(!region || (region->used + allocation_size > region->reserved))
    {
        if(DEFAULT_HEAP_SIZE == 0) DEFAULT_HEAP_SIZE = GB(4);
        region = slalloc_create_heap(DEFAULT_HEAP_SIZE);
    }

    allocation_size += sizeof(memory_allocation_t);
    if(region->used + allocation_size >= region->committed)
    {
        // NOTE(Sleepster): Error handled internally 
        u64 bytes_to_commit = (region->used + allocation_size) - region->committed;
        commit_virtual_memory(region, bytes_to_commit);
    }

    memory_allocation_t *allocation = (memory_allocation_t*)((byte*)region->memory_base + region->used);
    allocation->region              = region;
    allocation->allocation_size     = allocation_size;

    region->used += allocation_size;

    result = (void*)((byte*)allocation + sizeof(memory_allocation_t));

    return(result);
}

int
main(void)
{
    printf("Hello, World!\n");

    for(u64 index = 0;
        index < 1000000;
        ++index)
    {
        int *test_value  = (int*)slalloc(sizeof(int));
        int *test_value2 = (int*)slalloc(sizeof(int));

        *test_value  = 4;
        *test_value2 = 6;
    }

    return(0);
}
