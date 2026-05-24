/* ========================================================================
   $File: metaprogram_test_declared_functions.cpp $
   $Date: May 21 2026 03:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

void 
simple_test_function_decl();

short
complex_test_function_decl(int argument1, int argument2);

static inline void
more_complex_function_decl();

static inline double 
simple_function_definition(float argument)
{
}

u32 
simpler_function_definition()
{
}

static inline float 
complex_function_definition(int A, int B)
{
    return(A + B);
}
