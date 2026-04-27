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

struct slmalloc 
{
    void *data;
    u32   data_size;
    u32   used;

    u32   allocation_count;
};

#pragma pack(push, 0)
struct allocation_t
{   
    u32   ID;
    u32   size;
};
#pragma pack(pop)

static _Thread_local slmalloc *allocator;

void*
allocator_alloc(u64 size)
{
    void *result = null;

    result = (byte*)allocator->data + allocator->used;
    
    u64 allocation_size = Align16(size + sizeof(allocation_t));
    allocation_t *header = (allocation_t*)result;
    header->ID   = allocator->allocation_count++;
    header->size = size;

    allocator->used += allocation_size;

    u8 *allocation = (byte*)result + sizeof(allocation_t);
    result = (void*)allocation;

    return(result);
}

int
main(void)
{
    allocator = Alloc(slmalloc);
    allocator->data = mmap(null, MB(100), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    allocator->data_size = MB(100);

    Assert(allocator->data != MAP_FAILED);

    u32 *test = (u32*)allocator_alloc(sizeof(u32));
    (void)test;

    printf("Hello, World!\n");
    return(0);
}
