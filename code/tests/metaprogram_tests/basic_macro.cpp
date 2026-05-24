/* ========================================================================
   $File: metaprogram_test_basic_macro.cpp $
   $Date: May 21 2026 11:16 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define BASIC_MACRO 10
#define BASIC_PARENTHSIS_MACRO (1820)

// NOTE(Sleepster): Should be refused... 
#define BASIC_VA_ARGS_MACRO(...) item##__VA_ARGS__

#define global_variable static
#define local_persist   static
#define internal_api    static
