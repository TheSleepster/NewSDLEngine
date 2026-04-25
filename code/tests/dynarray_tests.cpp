/* ========================================================================
   $File: dynarray.cpp $
   $Date: December 01 2025 12:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define ITERATIONS (20)

TEST(dynarray_creation)
{
    DynArray_t(u32) test_elements = null;

    test_elements = c_dynarray_create(u32);
    test_elements = c_dynarray_reserve(test_elements, 50);
}

TEST(dynarray_push)
{
    DynArray_t(u32) test_elements = null;
    test_elements = c_dynarray_create(u32);

    u32 element0 = 15;

    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        element0 = index;
        c_dynarray_push(test_elements, element0);
    }
}

TEST(dynarray_pop)
{
    u32 element0 = 0;

    DynArray_t(u32) test_elements = null;
    test_elements = c_dynarray_create(u32);
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        element0 = index;
        c_dynarray_push(test_elements, element0);
    }

    u32 element = c_dynarray_pop(test_elements);
    (void)element;
}

TEST(dynarray_print_values)
{
    u32 element0 = 0;

    DynArray_t(u32) test_elements = null;
    test_elements = c_dynarray_create(u32);
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        element0 = index;
        c_dynarray_push(test_elements, element0);
    }

    u32 element = c_dynarray_pop(test_elements);
    (void)element;

    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        test_elements[index] = 6;
    }
}

TEST(dynarray_copy)
{
    DynArray_t(u32) test_elements = null;
    test_elements = c_dynarray_create(u32);
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        c_dynarray_push(test_elements, index);
    }

    DynArray_t(u32) test_elements2 = null;
    c_dynarray_copy(test_elements, test_elements2);

    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        Assert(test_elements2[index] == index);
    }
}

TEST(dynarray_clear)
{
    u32 element0 = 0;

    DynArray_t(u32) test_elements = null;
    test_elements = c_dynarray_create(u32);
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        element0 = index;
        c_dynarray_push(test_elements, element0);
    }

    c_dynarray_clear(test_elements);
    for(u32 index = 0;
        index < ITERATIONS;
        ++index)
    {
        Assert(test_elements[index] == 0);
    }
}
