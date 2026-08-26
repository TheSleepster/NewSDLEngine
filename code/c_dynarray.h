#if !defined(C_DYNARRAY_H)
/* ========================================================================
   $File: c_dynarray.h $
   $Date: August 17 2026 03:11 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_DYNARRAY_H
#include <c_base.h>
#include <c_types.h>
#include <string.h>

/////////////////////////
// STATIC ARRAY
/////////////////////////
//
// TODO(Sleepster): Custom allocator overriding 
//
//  Perhaps just achieve this with variables like:
//
//  static array_allocate_impl_t current_array_allocator;
//
//  This way you can just set the allocator yourself with a function like:
//
//  void push_array_allocator();
//  void pop_array_allocator();

#ifdef C_DYNARRAY_IMPLEMENTATION
typedef void *c_array_allocate_impl_t(void *allocator, u32 allocation_size);
typedef void *c_array_realloc_impl_t(void *allocator, void *memory, u32 new_size);
typedef void  c_array_free_impl_t(void *allocator, void *memory);

void*
c_dynarray_default_allocate_impl(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    return(result);
}

void*
c_dynarray_default_realloc_impl(void *allocator, void *memory, u32 new_size)
{
    void *result = null;
    result = realloc(memory, new_size);

    return(result);
}

void
c_dynarray_default_free_impl(void *allocator, void *memory)
{
    free(memory);
}
#endif
// TODO(Sleepster): Custom allocator overriding 

template <typename T, u32 capacity>
struct array_t
{
    T    items[capacity];
    u32  count = capacity;

    T &operator[](u32 index);
    T *operator+(u32 index);

    // NOTE(Sleepster): Stupid C++ stuff 
    T *begin() { return items; }
    T *end()   { return items + count; }

    const T *begin() const { return items; }
    const T *end()   const { return items + count; }
};

template <typename T, u32 count>
T&
array_t<T, count>::operator[](u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T, u32 count>
T*
array_t<T, count>::operator+(u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items + index);
}

// NOTE(Sleepster): Right now, this creates a ton of "use after free" bugs. Hopefully with our own allocator that's not a problem. 
template <typename T, u32 count>
void
c_array_resize(array_t<T, count> *array, u32 new_count)
{
    array->items    = (T*)reallocarray(array->items, sizeof(T), new_count);
    array->count = new_count;
}

template <typename T, u32 count>
void
c_array_clear(array_t<T, count> *array)
{
    memset(array->items, 0, sizeof(T) * array->count);
}

template <typename T, u32 count>
s32
c_array_find(array_t<T, count> *array, T *element)
{
    s32 result = -1;
    for(u32 index = 0;
        index < array->used;
        ++index)
    {
        T *found = array->items + index;
        if(memcmp(found, element, sizeof(T)) == 0)
        {
            result = index;
            break;
        }
    }

    return(result);
}

template <typename T, u32 count>
void
c_dynarray_remove(array_t<T, count> *array, u32 index)
{
    Assert(index <= array->count);

    for(u32 this_index = index;
        this_index < (array->used - 1);
        ++this_index)
    {
        array->items[this_index] = array->items[this_index + 1];
    }

    --array->used;
}

/////////////////////////
// DYNAMIC ARRAY
/////////////////////////

template <typename T>
struct dynarray_t
{
    T   *items;
    u32  count;
    u32  used;

    T &operator[](u32 index);
    T *operator+(u32 index);

    // NOTE(Sleepster): Stupid C++ crap 
    T *begin() { return items; }
    T *end()   { return items + used; }

    const T *begin() const { return items; }
    const T *end()   const { return items + used; }
};

template <typename T>
T&
dynarray_t<T>::operator[](u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T>
T*
dynarray_t<T>::operator+(u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items + index);
}

template <typename T>
void
c_dynarray_reserve(dynarray_t<T> *array, u32 to_reserve)
{
    if(to_reserve > array->count)
    {
        array->count = to_reserve;
        array->items = (T*)reallocarray(array->items, sizeof(T), array->count);
        Assert(array->items);

        memset(array->items, 0, sizeof(T) * array->count);
    }
}

template <typename T>
T*
c_dynarray_add(dynarray_t<T> *array, T *element)
{
    T *result = null;
    if((array->used + 1) > array->count)
    {
        u32 old_count = array->count;

        u32 new_count = Max(5, array->count * 2);
        T  *new_items = (T*)realloc(array->items, sizeof(T) * new_count);
        ZeroMemory(new_items, sizeof(T) * new_count);

        array->count = new_count;
        array->items = new_items;

        Assert(array->items);
        if(old_count > 0)
        {
            void *offset_ptr = (void*)(array->items + old_count);
            u32 copy_size    = (sizeof(T) * (array->count - old_count));

            memset(offset_ptr, 0, copy_size);
        }
    }

    result = array->items + array->used; 
    array->items[array->used++] = *element;

    return(result);
}

template <typename T>
void
c_dynarray_remove(dynarray_t<T> *array, u32 index)
{
    Assert(index <= array->count);

    for(u32 this_index = index;
        this_index < (array->used - 1);
        ++this_index)
    {
        array->items[this_index] = array->items[this_index + 1];
    }

    --array->used;
}

template <typename T>
T
c_dynarray_pop(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T result = array->items[array->used];
    --array->used;

    return(result);
}

template <typename T>
T*
c_dynarray_pop_ptr(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T *result = array->items + array->used;
    --array->used;

    return(result);
}

template <typename T>
s32
c_dynarray_find(dynarray_t<T> *array, T *element)
{
    s32 result = -1;
    for(u32 index = 0;
        index < array->used;
        ++index)
    {
        T *found = array->items + index;
        if(memcmp(found, element, sizeof(T)) == 0)
        {
            result = index;
            break;
        }
    }

    return(result);
}

template <typename T>
bool8
c_dynarray_add_if_unique(dynarray_t<T> *array, T *element, s32 *index_out = null)
{
    bool8 added = false;

    s32 index = c_dynarray_find(array, element);
    if(index == -1)
    {
        c_dynarray_add(array, element);
        if(index_out)
        {
            *index_out = array->used - 1;
        }

        added = true;
    }

    return(added);
}

template <typename T>
void
c_dynarray_insert_at(dynarray_t<T> *array, T *element, u32 index)
{
    array->items[index] = *element;
}

template <typename T>
T
c_dynarray_get_at_index(dynarray_t<T> *array, u32 index)
{
    T result = array->items[index];
    return(result);
}

template <typename T>
T*
c_dynarray_get_ptr_at_index(dynarray_t<T> *array, u32 index)
{
    T* result = array->items + index;
    return(result);
}

template <typename T>
void
c_dynarray_reset(dynarray_t<T> *array)
{
    array->used = 0;
    memset(array->items, 0, sizeof(T) * array->count);
}

template <typename T>
void
c_dynarray_copy(dynarray_t<T> *destination, dynarray_t<T> *source)
{
    Assert(source->items);
    Assert(source->count);
    if(!destination->items || destination->count != source->count)
    {
        destination->items = (T*)realloc(destination->items, Align(sizeof(T) * source->count, 32));
        Assert(destination->items);
    }

    destination->count = source->count;
    destination->used     = source->used;

    memcpy(destination->items, source->items, sizeof(T) * destination->count);
}

template <typename T>
void
c_dynarray_free(dynarray_t<T> *array)
{
    array->used  = 0;
    array->count = 0;

    free(array->items);
}

#endif // C_DYNARRAY_H

