/* ========================================================================
   $File: metaprogram_test_declared_functions.cpp $
   $Date: May 21 2026 03:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

void 
simple_test_function_decl();

short
complex_test_function_decl(int argument1 = 0, int argument2 = 0);

static inline void
more_complex_function_decl();

static inline const double 
simple_function_definition(float argument)
{
}

u32 
simpler_function_definition(void)
{
}

static inline float 
complex_function_definition(int A, int B)
{
    return(A + B);
}

static inline void*
allocator_function(memory_arena_t *memory_arena, u64 size)
{
    return(A + B);
}

