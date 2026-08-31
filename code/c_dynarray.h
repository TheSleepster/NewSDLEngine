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
#include <c_math.h>
#include <string.h>

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
typedef void *c_array_allocate_impl_t(void *allocator, s32 allocation_size);
typedef void *c_array_realloc_impl_t(void *allocator, void *memory, s32 new_size);
typedef void  c_array_free_impl_t(void *allocator, void *memory);

void*
c_dynarray_default_allocate_impl(void *allocator, s32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    return(result);
}

void*
c_dynarray_default_realloc_impl(void *allocator, void *memory, s32 new_size)
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

/////////////////////////
// ARRAY VIEW
//
// This essentially acts as std::span for arrays.
/////////////////////////

template <typename T>
struct array_view_t
{
    T  *items;
    s32 count;

    T &operator[](s32 index);
    T *operator+(s32 index);

    // NOTE(Sleepster): Stupid C++ stuff 
    // Also, Athena blows up if we do parenthesis in here. I hate that.
    T *begin() { return items; }
    T *end()   { return items + count; }

    const T *begin() const { return items; }
    const T *end()   const { return items + count; }
};

template <typename T>
T&
array_view_t<T>::operator[](s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T>
T*
array_view_t<T>::operator+(s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items + index);
}

template <typename T>
void
c_array_clear(array_view_t<T> array)
{
    memset(array.items, 0, sizeof(T) * array.count);
}

template <typename T>
s32
c_array_find(array_view_t<T> array, T *element)
{
    s32 result = -1;
    for(s32 index = 0;
        index < array.used;
        ++index)
    {
        T *found = array.items + index;
        if(memcmp(found, element, sizeof(T)) == 0)
        {
            result = index;
            break;
        }
    }

    return(result);
}

template <typename T>
void
c_array_remove(array_view_t<T> array, s32 index, s32 max_index)
{
    Assert(index <= array.count);
    if(max_index == -1)
    {
        max_index = array.count;
    }

    for(s32 this_index = index;
        this_index < max_index;
        ++this_index)
    {
        array.items[this_index] = array.items[this_index + 1];
    }
}

template <typename T>
s32
c_array_add_if_unique(array_view_t<T> array, T *element, s32 index_to_emplace)
{
    s32 result = -1;

    T value = *element;

    bool8 found = false;
    for(s32 index = 0;
        index < index_to_emplace;
        ++index)
    {
        T searched_element = array[index];
        if(searched_element == value)
        {
            found  = true;
            result = index;
            break;
        }
    }

    if(!found)
    {
        array[index_to_emplace] = value;
    }

    return(result);
}

/////////////////////////
// STATIC ARRAY
/////////////////////////

template <typename T, s32 capacity>
struct array_t
{
    T    items[capacity];
    s32  count = capacity;

    T &operator[](s32 index);
    T *operator+(s32 index);

    // NOTE(Sleepster): We may want to inspect the runtime cost of this conversion... it's probably near free... but you never know
    operator array_view_t<T>() { return((array_view_t<T>){items, count}); }

    // NOTE(Sleepster): Stupid C++ stuff 
    T *begin() { return items; }
    T *end()   { return items + count; }

    const T *begin() const { return items; }
    const T *end()   const { return items + count; }

};

template <typename T, s32 count>
T&
array_t<T, count>::operator[](s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T, s32 count>
T*
array_t<T, count>::operator+(s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items + index);
}

// NOTE(Sleepster): These all call their "array_view_t" variants... 
template <typename T, s32 count>
void
c_array_clear(array_t<T, count> &array)
{
    c_array_clear(static_cast<array_view_t<T>>(array));
}

template <typename T, s32 count>
s32
c_array_find(array_t<T, count> &array, T *element)
{
    c_array_find(static_cast<array_view_t<T>>(array), element);
}

template <typename T, s32 count>
void
c_array_remove(array_t<T, count> &array, s32 index, s32 max_index)
{
    c_array_remove(static_cast<array_view_t<T>>(array), index, max_index);
}

template <typename T, s32 count>
s32
c_array_add_if_unique(array_t<T, count> &array, T *element, s32 index_to_emplace)
{
    s32 result = c_array_add_if_unique(static_cast<array_view_t<T>>(array), element, index_to_emplace);
    return(result);
}

/////////////////////////
// DYNAMIC ARRAY
/////////////////////////

template <typename T>
struct dynarray_t
{
    T   *items;
    s32  count;
    s32  used;

    T &operator[](s32 index);
    T *operator+(s32 index);

    // NOTE(Sleepster): Stupid C++ crap 
    T *begin() { return items; }
    T *end()   { return items + used; }

    const T *begin() const { return items; }
    const T *end()   const { return items + used; }
};

template <typename T>
T&
dynarray_t<T>::operator[](s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T>
T*
dynarray_t<T>::operator+(s32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while count is: '%u'...\n", index, this->count);
    return(this->items + index);
}

template <typename T>
void
c_dynarray_reserve(dynarray_t<T> *array, s32 to_reserve)
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
        //s32 old_count = array->count;

        // NOTE(Sleepster): realloc does the memcpy for us. We don't need to do it
        s32 new_count = Max(5, array->count * 2);
        T  *new_items = (T*)realloc(array->items, sizeof(T) * new_count);

        array->count = new_count;
        array->items = new_items;

        Assert(array->items);
    }

    result = array->items + array->used; 
    array->items[array->used++] = *element;

    return(result);
}

template <typename T>
void
c_dynarray_remove(dynarray_t<T> *array, s32 index)
{
    Assert(index <= array->count);

    for(s32 this_index = index;
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
    for(s32 index = 0;
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
c_dynarray_insert_at(dynarray_t<T> *array, T *element, s32 index)
{
    array->items[index] = *element;
}

template <typename T>
T
c_dynarray_get_at_index(dynarray_t<T> *array, s32 index)
{
    T result = array->items[index];
    return(result);
}

template <typename T>
T*
c_dynarray_get_ptr_at_index(dynarray_t<T> *array, s32 index)
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

