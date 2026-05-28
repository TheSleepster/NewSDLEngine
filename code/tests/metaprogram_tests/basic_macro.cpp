/* ========================================================================
   $File: metaprogram_test_basic_macro.cpp $
   $Date: May 21 2026 11:16 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define BASIC_MACRO 10
#define BASIC_PARENTHSIS_MACRO (1820)

#define BASIC_COMPOUND_MACRO   (BASIC_PARENTHSIS_MACRO * 10)

#define BASIC_MACRO_WITH_ARGUMENTS(arg0, arg1) arg0##arg1
#define BASIC_MACRO_WITH_ARGUMENTS_AND_PAREN(arg2, arg3) (arg2##arg3)

// NOTE(Sleepster): Should be refused... 
//
// Supporting these, at least at the moment seems pointless and just a hassle.
// #define BASIC_VA_ARGS_MACRO(...) item##__VA_ARGS__

#define global_variable static
#define local_persist   static
#define internal_api    static

#if 0

#define THIS_SHOULD_NOT_GET_FOUND

#endif
