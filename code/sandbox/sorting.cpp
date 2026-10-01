/* ========================================================================
   $File: sorting.cpp $
   $Date: September 30 2026 06:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <c_types.h>
#include <c_dynarray.h>

template <typename T>
void
swap(T *A, T *B)
{
    T temp = *A;
    *A = *B;
    *B = temp;
}

/*
==================================
Quicksort
==================================
*/

template <typename T, typename Lambda>
void
quicksort(T *elements, s32 count, Lambda &function)
{
    if(count >= 2)
    {
        T pivot_element = elements[count / 2];

        s32 left_element  = 0;
        s32 right_element = count - 1;
        for(;;)
        {
            while(function(elements[left_element], pivot_element)           < 0) {left_element++;}
            while(function(pivot_element,          elements[right_element]) < 0) {right_element--;}
            if(left_element >= right_element)
            {
                break;
            }

            T *A = elements + left_element;
            T *B = elements + right_element;
            swap(A, B);

            left_element  += 1;
            right_element -= 1;
        }

        quicksort(elements, left_element, function);
        quicksort(elements + left_element, count - left_element, function);
    }
}

template <typename T, typename Lambda>
void
quicksort(array_view_t<T> array, Lambda &function)
{
    quicksort(array.items, array.count, function);
}

/*
==================================
Insertion Sort
==================================
*/

template <typename T, typename Lambda>
void
insertion_sort(T *elements, s32 begin, s32 end, Lambda &function)
{
    for(s32 source_index = begin + 1;
        source_index < end;
        ++source_index)
    {
        T value = elements[source_index];
        s32 index_to_insert_at = source_index;
        while(index_to_insert_at != begin && (function(value, elements[index_to_insert_at - 1]) < 0))
        {
            elements[index_to_insert_at] = elements[index_to_insert_at - 1];
            index_to_insert_at -= 1;
        }

        elements[index_to_insert_at] = value;
    }
}


template <typename T, typename Lambda>
void
insertion_sort(array_view_t<T> array, Lambda &function)
{
    insertion_sort(array.items, 0, array.count, function);
}

/*
==================================
Intro Sort
==================================
*/

constexpr s32 INSERTION_SORT_THRESHOLD = 32;

template <typename T, typename Lambda>
s32
introsort_partition(T *elements, s32 start, s32 end, T &pivot_element, Lambda &function)
{
    s32 result = 0;

    s32 left_element  = start;
    s32 right_element = end - 1;
    for(;;)
    {
        while(function(elements[left_element], pivot_element)           < 0) {left_element++;}
        while(function(pivot_element,          elements[right_element]) < 0) {right_element--;}
        if(left_element >= right_element)
        {
            result = left_element;
            break;
        }

        T *A = elements + left_element;
        T *B = elements + right_element;
        swap(A, B);

        left_element  += 1;
        right_element -= 1;
    }

    return(result);
}

template <typename T, typename Lambda>
void
introsort_impl(T *elements, s32 start, s32 end, Lambda &function)
{
    while(end - start > INSERTION_SORT_THRESHOLD)
    {
        T pivot = elements[start + (end - start) / 2];
        s32 partition_index = introsort_partition(elements, start, end, pivot, function);
        if(end - partition_index <= partition_index - start)
        {
            introsort_impl(elements, partition_index, end, function);
            end = partition_index;
        }
        else
        {
            introsort_impl(elements, start, partition_index, function);
            start = partition_index;
        }
    }
}

template <typename T, typename Lambda>
void
introsort(T *elements, s32 start, s32 end, Lambda &function)
{
    if(start < end)
    {
        introsort_impl(elements, start, end, function);
        insertion_sort(elements, start, end, function);
    }
}

/*
==================================
Radix Sort
==================================
*/

int
main(void)
{
    fixed_array_t<int, 19> array = {1, 0, 2, 3, 4, 8, 7, 9, 20, 10,
                                    9, 5, 6, 6, 2, -1, 2, 9, 9};
    printf("Presort:\n");
    for(int element: array)
    {
        printf("\t%d\n", element);
    }

    auto compare_function = [](int A, int B) -> int {
        return((A > B) - (A < B));
    };
#if 1
    quicksort(array.items, array.count, compare_function); 
#elif 0
    insertion_sort(array.items, 0, array.count, compare_function);
#elif 1
    introsort(array.items, 0, array.count, compare_function);
#endif

    printf("Post sort:\n");
    for(int element: array)
    {
        printf("\t%d\n", element);
    }
}
