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

#define DEBUG_BLOCK_ID (0xF0C0FF)

struct memory_node_t
{
    byte *start;
    s64   size;
};

struct memory_header_t
{
    s32   ID;
    s32   _reserved;
    s64   block_size;
    byte *base;
};

int
main(void)
{
    byte *memory = (byte*)sys_allocate_memory(GB(16));

    u64 allocation = MB(100);

    memory_header_t *header = (memory_header_t*)memory;
    memory += sizeof(memory_header_t);
    header->base = memory;
    header->ID   = DEBUG_BLOCK_ID;

    memory += sizeof(allocation);
#if 0
    u64 DEBUG_buffer = MB(10);
    sys_set_memory_properties(memory, READ_PROTECTED|WRITE_PROTECTED|EXECUTE_PROTECTED, DEBUG_buffer);
    memory += DEBUG_buffer;
#endif
    
    printf("Hello, World!\n");
    return(0);
}
