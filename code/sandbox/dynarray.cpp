/* ========================================================================
   $File: dynarray.cpp $
   $Date: June 08 2026 04:48 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include <stdio.h>

/////////////////////////
// STATIC ARRAY
/////////////////////////

template <typename T, u32 count>
struct array_t
{
    T  *items;
    u32 max_count;

    array_t(memory_arena_t *arena = null);
    T  &operator[](u32 index);
    T *operator+(u32 index);
};

template <typename T, u32 count>
array_t<T, count>::array_t(memory_arena_t *arena)
{
    this->max_count = count;

    if(arena) this->items = c_arena_push_array(arena, T, count);
    else      this->items = (T*)calloc(sizeof(T), count);
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
dynarray_add(dynarray_t<T> *array, T *element)
{
    if((array->used + 1) > array->capacity)
    {
        array->capacity = Max(8, array->capacity);
        array->items = (T*)reallocarray(array->items, sizeof(T), array->capacity);
    }

    array->items[array->used++] = *element;
}

template <typename T>
void
dynarray_remove(dynarray_t<T> *array, u32 index)
{
    Assert(index <= array->capacity);
    u32 last_index = array->used - 1;
    if(index != last_index)
    {
        array[index] = array[last_index];
    }
    --array->used;
}

template <typename T>
T
dynarray_pop(dynarray_t<T> *array)
{
    Assert(array->used - 1 >= 0);

    T result = array->items[--array->used];
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

/////////////////////////
// DYNAMIC ARRAY
/////////////////////////

int
main(void)
{
    memory_arena_t arena = c_arena_create(MB(10));
    (void)arena;

    printf("Hello, World!\n");
    array_t<u32, 1024> items;

    items[0] = 10;
    items[1] = 10;
    items[2] = 10;
    items[3] = 10;
    items[4] = 10;

    dynarray_t<u32> elements;

    for(u32 index = 0;
        index < 10;
        ++index)
    {
        u32 test_number = 932 + index;
        dynarray_add(&elements, &test_number);

        u32 test = dynarray_pop(&elements);
        (void)test;

        test = 0;
    }
}
