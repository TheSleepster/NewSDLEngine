/* ========================================================================
   $File: metaprogram_test_declared_functions.cpp $
   $Date: May 21 2026 03:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

void 
simple_test_function_decl();

void
complex_test_function_decl(int argument1, int argument2);

static inline void
more_complex_function_decl();

static inline void
simple_function_definition(void)
{
}

void 
simpler_function_definition()
{
}

static inline int 
complex_function_definition(int A, int B)
{
    return(A + B);
}
