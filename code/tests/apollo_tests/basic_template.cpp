/* ========================================================================
   $File: basic_template.cpp $
   $Date: October 06 2026 03:39 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

template <typename T>
struct array_t 
{
    T  *items;
    int count;
};

struct SOA
{
    array_t<int> int_array;
    array_t<float> float_array;
};
