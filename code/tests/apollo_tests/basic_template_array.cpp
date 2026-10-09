/* ========================================================================
   $File: basic_array.cpp $
   $Date: October 06 2026 03:46 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

template <typename T, int capacity>
struct static_array_t
{
    T items[capacity];
    static constexpr int count = capacity;
};

struct SOA
{
    static_array_t<int, 32>   int_array32;
    static_array_t<float, 32> float_array32;

    int c_array[32];
    int c_array_incomplete[];
};
