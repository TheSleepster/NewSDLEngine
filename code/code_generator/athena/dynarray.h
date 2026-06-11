#if !defined(DYNARRAY_H)
/* ========================================================================
   $File: dynarray.h $
   $Date: June 10 2026 10:31 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define DYNARRAY_H
#include <c_base.h>
#include <c_types.h>
#include <c_string.h>

/////////////////////////
// STATIC ARRAY
/////////////////////////


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
//
typedef void *array_allocate_impl_t(void *allocator, u32 allocation_size);
typedef void *array_realloc_impl_t(void *allocator, void *memory, u32 new_size);
typedef void  array_free_impl_t(void *allocator, void *memory);

void*
array_default_allocate_impl(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    return(result);
}

void*
array_default_realloc_impl(void *allocator, void *memory, u32 new_size)
{
    void *result = null;
    result = realloc(memory, new_size);

    return(result);
}

void
array_default_free_impl(void *allocator, void *memory)
{
    free(memory);
}
// TODO(Sleepster): Custom allocator overriding 


template <typename T>
struct array_t
{
    T   *items;
    u32  capacity;

    array_t(u32 initial_capacity = 0);

    T  &operator[](u32 index);
    T *operator+(u32 index);

    // NOTE(Sleepster): Stupid C++ stuff 
    T *begin() { return items; }
    T *end()   { return items + capacity; }

    const T *begin() const { return items; }
    const T *end()   const { return items + capacity; }
};

template <typename T>
array_t<T>::array_t(u32 initial_capacity)
{
    this->capacity = initial_capacity;
    if(initial_capacity > 0)
    {
        this->items = reallocarray(this->items, sizeof(T), initial_capacity);
    }
}

template <typename T>
T&
array_t<T>::operator[](u32 index)
{
    Expect(index < this->capacity, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->capacity);
    return(this->items[index]);
}

template <typename T>
T*
array_t<T>::operator+(u32 index)
{
    Expect(index < this->capacity, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->capacity);
    return(this->items + index);
}

template <typename T>
void
array_resize(array_t<T> *array, u32 new_capacity)
{
    array->items    = reallocarray(array->items, sizeof(T), new_capacity);
    array->capacity = new_capacity;
}

template <typename T>
void
array_clear(array_t<T> *array)
{
    memset(array->items, 0, sizeof(T) * array->capacity);
}

template <typename T>
void
array_free(array_t<T> *array)
{
    array->capacity = 0;
    free(array->items);
}

/////////////////////////
// DYNAMIC ARRAY
/////////////////////////

template <typename T>
struct dynarray_t
{
    T   *items;
    u32  capacity;
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
    Expect(index < this->capacity, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->capacity);
    return(this->items[index]);
}

template <typename T>
T*
dynarray_t<T>::operator+(u32 index)
{
    Expect(index < this->capacity, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->capacity);
    return(this->items + index);
}

template <typename T>
void
dynarray_reserve(dynarray_t<T> *array, u32 to_reserve)
{
    if(to_reserve > array->capacity)
    {
        array->capacity = to_reserve;
        array->items = reallocarray(array->items, sizeof(T), array->capacity);
    }
}

template <typename T>
void
dynarray_add(dynarray_t<T> *array, T element)
{
    if((array->used + 1) > array->capacity)
    {
        array->capacity = Max(8, array->capacity * 2);
        array->items = (T*)reallocarray(array->items, sizeof(T), array->capacity);
    }

    array->items[array->used++] = element;
}

template <typename T>
void
dynarray_remove(dynarray_t<T> *array, u32 index)
{
    Assert(index <= array->capacity);

    for(u32 this_index = index;
        this_index < (array->used - 1);
        ++this_index)
    {
        array[this_index] = array[this_index + 1];
    }

    --array->used;
}

template <typename T>
T
dynarray_pop(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T result = array->items[array->used];
    array->items[array->used] = {}; 
    --array->used;

    return(result);
}

template <typename T>
T*
dynarray_pop_ptr(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T *result = array->items + array->used;
    array->items[array->used] = {}; 
    --array->used;

    return(result);
}

template <typename T>
u32
dynarray_find(dynarray_t<T> *array, T *element)
{
    u32 result = 0;
    for(u32 index = 0;
        index < array->used;
        ++index)
    {
        T *found = array + index;
        if(*found == *element)
        {
            result = index;
            break;
        }
    }

    return(result);
}

template <typename T>
void
dynarray_insert_at(dynarray_t<T> *array, T *element, u32 index)
{
    array->items[index] = *element;
}

template <typename T>
void
dynarray_reset(dynarray_t<T> *array)
{
    array->used     = 0;
    array->capacity = 0;
    free(array->items);
}

template <typename T>
void
dynarray_copy(dynarray_t<T> *destination, dynarray_t<T> *source)
{
    Assert(source->items);
    Assert(source->capacity);
    if(!destination->items || destination->capacity != source->capacity)
    {
        destination->items = (T*)reallocarray(destination->items, sizeof(T), source->capacity);
    }

    destination->capacity = source->capacity;
    destination->used     = source->used;

    memcpy(destination->items, source->items, sizeof(T) * destination->capacity);
}

template <typename T>
void
dynarray_free(dynarray_t<T> *array)
{
    array->used     = 0;
    array->capacity = 0;

    free(array->items);
}

#endif // DYNARRAY_H

