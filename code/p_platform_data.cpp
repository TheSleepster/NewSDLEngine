/* ========================================================================
   $File: p_platform_data.cpp $
   $Date: December 04 2025 12:05 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <p_platform_data.h>


#if OS_WINDOWS
#include <os_windows.cpp>
#elif OS_LINUX
#error not implemented.
//#include <os_linux.cpp>
#elif OS_MAC
#error not implemented.
//#include <os_macos.cpp>
#endif
