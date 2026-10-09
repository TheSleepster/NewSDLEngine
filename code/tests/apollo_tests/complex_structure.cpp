/* ========================================================================
   $File: complex_structure.cpp $
   $Date: October 07 2026 05:03 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

struct complex_test_t
{
    int test0;
    int test1;
    struct {
        int values[16][16];
    }matrices;

    union {
        int   int_storage[10];
        float float_storage;
    };

    struct named_matrices
    {
        int values2[16][16];
    };

    complex_test_t();
    ~complex_test_t();
    complex_test_t(complex_test_t &&test);

    void test_function();
};
