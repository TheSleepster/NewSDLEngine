/* ========================================================================
   $File: dynarray.cpp $
   $Date: June 08 2026 04:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <string.h>

#include <c_types.h>
#include <c_base.h>

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

#define C_ARRAY_IMPLEMENTATION
#ifdef C_ARRAY_IMPLEMENTATION
typedef void *c_array_allocate_impl_t(void *allocator, u32 allocation_size);
typedef void *c_array_realloc_impl_t(void *allocator, void *memory, u32 new_size);
typedef void  c_array_free_impl_t(void *allocator, void *memory);

void*
c_array_default_allocate_impl(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    return(result);
}

void*
c_array_default_realloc_impl(void *allocator, void *memory, u32 new_size)
{
    void *result = null;
    result = realloc(memory, new_size);

    return(result);
}

void
c_array_default_free_impl(void *allocator, void *memory)
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

template <typename T, u32 capacity>
T&
array_t<T, capacity>::operator[](u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->count);
    return(this->items[index]);
}

template <typename T, u32 capacity>
T*
array_t<T, capacity>::operator+(u32 index)
{
    Expect(index < this->count, "Array bounds check failed... index was: '%u' while capacity is: '%u'...\n", index, this->count);
    return(this->items + index);
}

// NOTE(Sleepster): Right now, this creates a ton of "use after free" bugs. Hopefully with our own allocator that's not a problem. 
template <typename T, u32 capacity>
void
c_array_resize(array_t<T, capacity> *array, u32 new_capacity)
{
    array->items    = (T*)reallocarray(array->items, sizeof(T), new_capacity);
    array->capacity = new_capacity;
}

template <typename T, u32 capacity>
void
c_array_clear(array_t<T, capacity> *array)
{
    memset(array->items, 0, sizeof(T) * array->capacity);
}

template <typename T, u32 capacity>
s32
c_array_find(array_t<T, capacity> *array, T *element)
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
c_dynarray_reserve(dynarray_t<T> *array, u32 to_reserve)
{
    if(to_reserve > array->capacity)
    {
        array->capacity = to_reserve;
        array->items = reallocarray(array->items, sizeof(T), array->capacity);
        Assert(array->items);

        memset(array->items, 0, sizeof(T) * array->capacity);
    }
}

template <typename T>
T*
c_dynarray_add(dynarray_t<T> *array, T *element)
{
    T *result = null;
    if((array->used + 1) > array->capacity)
    {
        u32 old_capacity = array->capacity;

        // TODO(Sleepster): THIS IS REALLLLLLLLY BAD. Change this once we have our own malloc 
        array->capacity = Max(60, array->capacity * 2);
        array->items = (T*)realloc(array->items, Align(sizeof(T) * array->capacity, 32));
        Assert(array->items);

        if(old_capacity > 0)
        {
            void *offset_ptr = (void*)(array->items + old_capacity);
            u32 copy_size    = (sizeof(T) * (array->capacity - old_capacity));

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
    Assert(index <= array->capacity);

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

    s32 index = dynarray_find(array, element);
    if(index == -1)
    {
        dynarray_add(array, element);
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
    memset(array->items, 0, sizeof(T) * array->capacity);
}

template <typename T>
void
c_dynarray_copy(dynarray_t<T> *destination, dynarray_t<T> *source)
{
    Assert(source->items);
    Assert(source->capacity);
    if(!destination->items || destination->capacity != source->capacity)
    {
        destination->items = (T*)realloc(destination->items, Align(sizeof(T) * source->capacity, 32));
        Assert(destination->items);
    }

    destination->capacity = source->capacity;
    destination->used     = source->used;

    memcpy(destination->items, source->items, sizeof(T) * destination->capacity);
}

template <typename T>
void
c_dynarray_free(dynarray_t<T> *array)
{
    array->used     = 0;
    array->capacity = 0;

    free(array->items);
}

/////////////////////////
// MAIN 
/////////////////////////

int
main(void)
{
    array_t<int, 100> test_array;
    return(0);
}
