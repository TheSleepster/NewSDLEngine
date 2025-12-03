/* ========================================================================
   $File: c_dynarray_impl.cpp $
   $Date: November 30 2025 08:03 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>

#include <c_dynarray.h>
#include <stdio.h>
#include <stdlib.h>

void*
_dynarray_create_impl(u32 element_size)
{
    void *result = null;

    u32 allocation_size = Align16((element_size * DYNARRAY_INITIAL_SIZE) + (sizeof(dynarray_header_t)));
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    dynarray_header_t *header = (dynarray_header_t*)result;
    result = (byte*)result + sizeof(dynarray_header_t);

    header->capacity = DYNARRAY_INITIAL_SIZE;

    return(result);
}

void*
_dynarray_grow_impl(void **array, u32 element_size, u32 new_capacity)
{
    Expect(new_capacity > DYNARRAY_INITIAL_SIZE, "new capacity is <= Initial\n");
    void *result = null;
    result = (byte*)*array - sizeof(dynarray_header_t);

    result = realloc(result, (element_size * new_capacity) + sizeof(dynarray_header_t));
    result = (byte*)result + sizeof(dynarray_header_t);

    dynarray_header_t *header = _dynarray_header(result);
    header->capacity = new_capacity;

    return(result);
}

void
_dynarray_destroy_impl(void **array)
{
    void *array_data = (byte *)*array - sizeof(dynarray_header_t);
    free(array_data);

    *array = null;
}

void
_dynarray_insert_impl(void **array, void *element, u32 element_size, u32 index)
{
    byte *index_data = (byte*)*array + (element_size * index);
    memcpy(index_data, element, element_size);
}

void
_dynarray_remove_impl(void **array, u32 element_size, u32 index)
{
    byte *array_data          = (byte*)*array;
    dynarray_header_t *header = (dynarray_header_t*)((byte*)*array - sizeof(dynarray_header_t));

    Expect(index <= header->size, "Index is > to that of the header->size");
    if(index < header->size - 1) 
    {
        byte *to   = array_data + (element_size * index);
        byte *from = array_data + (element_size * (index + 1));

        usize bytes_to_write = (header->size - index - 1) * element_size;
        memmove(to, from, bytes_to_write);
    }
    header->size -= 1;
}
