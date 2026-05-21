/* ========================================================================
   $File: metaprogram_test_complex_macro.cpp $
   $Date: May 21 2026 11:16 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define COMPLEX_MACRO_MULTILINE(argument0, argument1, argument2) \
argument0##argument1##argument2

#define COMPLEX_MACRO(argument) argument
