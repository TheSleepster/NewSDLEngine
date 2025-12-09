/* ========================================================================
   $File: p_platform_data.cpp $
   $Date: December 08 2025 08:42 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <p_platform_data.h>

#if OS_WINDOWS
    #include <sys_win32.cpp>
#elif OS_LINUX
    #error "not included"
#elif OS_MAC
    #error "lmao really?"
#endif
