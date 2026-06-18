/* ========================================================================
   $File: templates.cpp $
   $Date: May 24 2026 01:45 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

template <typename T>
struct template_container
{
    template_test<T> items;
};

template <typename T>
struct template_test
{
    T *items;
    u32 count;
};