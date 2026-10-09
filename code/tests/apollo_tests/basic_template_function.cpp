/* ========================================================================
   $File: basic_template_function.cpp $
   $Date: October 06 2026 07:01 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
template <typename T>
struct array_t
{
    int  count;
    T   *items;
};

template <typename T>
array_t<T>
array_create(int count)
{
    array_t<T> result = {};
    return(result);
}

template <typename T>
void
swap(T *A, T *B)
{
}
