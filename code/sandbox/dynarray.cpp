/* ========================================================================
   $File: dynarray.cpp $
   $Date: June 08 2026 04:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#if 0
#include <stdio.h>

/////////////////////////
// STATIC ARRAY
/////////////////////////

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

// TODO(Sleepster): Remove count
template <typename T>
struct array_t
{
    T                     *items;
    u32                    max_count;

    array_allocate_impl_t *allocate;
    array_realloc_impl_t  *reallocate;
    array_free_impl_t     *free;

    array_t(void                  *allocator = null, 
            array_allocate_impl_t *allocate  = array_default_allocate_impl,
            array_realloc_impl_t  *realloc   = array_default_realloc_impl,
            array_free_impl_t     *free      = array_default_free_impl);

    T  &operator[](u32 index);
    T *operator+(u32 index);
};

template <typename T, u32 count>
array_t<T, count>::array_t(void                  *allocator, 
                           array_allocate_impl_t *allocate,
                           array_realloc_impl_t  *realloc,
                           array_free_impl_t     *free)
{
    this->max_count = count;
    this->items     = (T*)this->allocate(allocator, sizeof(T) * count);
}

template <typename T, u32 count>
T&
array_t<T, count>::operator[](u32 index)
{
    Assert(index <= this->max_count);
    return(this->items[index]);
}

template <typename T, u32 count>
T*
array_t<T, count>::operator+(u32 index)
{
    Assert(index <= this->max_count);
    return(this->items + index);
}

template <typename T, u32 count>
void
array_resize(array_t<T, count> *array, u32 max_size)
{
    array->items = reallocarray(array->items, sizeof(T), max_size);
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
};

template <typename T>
T&
dynarray_t<T>::operator[](u32 index)
{
    Assert(index <= capacity);
    return(this->items[index]);
}

template <typename T>
T*
dynarray_t<T>::operator+(u32 index)
{
    Assert(index <= this->capacity);
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

    T result = array->items[--array->used];
    array->items[array->used + 1] = {}; 

    return(result);
}

template <typename T>
T*
dynarray_pop_ptr(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T *result = array->items + (--array->used);

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
    array->used  = 0;
    array->items = free(array->items);
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

/////////////////////////
// MAIN 
/////////////////////////

int
main(void)
{
    memory_arena_t arena = c_arena_create(MB(10));
    (void)arena;

    printf("Hello, World!\n");
    array_t<u32, 1024> items(&arena);

    items[0] = 10;
    items[1] = 10;
    items[2] = 10;
    items[3] = 10;
    items[4] = 10;

    dynarray_t<u32> elements = {};

    for(u32 index = 0;
        index < 10;
        ++index)
    {
        u32 test_number = 932 + index;
        dynarray_add(&elements, test_number);
    }

    dynarray_t<u32> elements2 = {};
    dynarray_copy(&elements2, &elements);

    u32 item = 0;
    for(u32 index = 0;
        index < 10;
        ++index)
    {
        item = dynarray_pop(&elements2);
    }

    (void)item;
    printf("Hello World2\n");
}
#else
int
main()
{
}
#endif
