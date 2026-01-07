/* ========================================================================
   $File: dynarray.cpp $
   $Date: December 01 2025 12:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_dynarray.h>

#include <c_dynarray_impl.cpp>

#define ITERATIONS (20)

int
main(void)
{
    DynArray_t(u32) test_elements = null;
    u32 element0 = 15;

    test_elements = c_dynarray_create(u32);
    test_elements = c_dynarray_reserve(test_elements, 50);

    printf("testing pushing and auto creation...\n");
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        element0 = index;
        c_dynarray_push(test_elements, element0);
    }
    printf("success!\n");

    printf("testing popping...\n");
    u32 element = c_dynarray_pop(test_elements);
    printf("element: '%u'...\n", element);

    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        printf("value at index %d: '%u'\n", index, test_elements[index]);
    }
    printf("success!\n");

    printf("testing copying...\n");
    DynArray_t(u32) test_elements2 = null;
    c_dynarray_copy(test_elements, test_elements2);
    printf("success...\n");
    printf("element at index 2 in new array: '%d'...\n", test_elements2[2]);

    printf("testing clearing...\n");
    c_dynarray_clear(test_elements);
    printf("didn't crash...\n^");
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        printf("value at index %d: '%u'\n", index, test_elements[index]);
    }
    printf("success!\n");

    return(0);
}
