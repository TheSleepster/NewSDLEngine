#if !defined(P_PLATFORM_DATA_H)
/* ========================================================================
   $File: p_platform_data.h $
   $Date: December 04 2025 12:04 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define P_PLATFORM_DATA_H

#include <c_base.h>
#include <c_types.h>

bool8 s_network_data_init();

#if OS_WINDOWS
#include <os_windows.h>
#elif OS_LINUX
#error not implemented.
//#include <os_linux.h>
#elif OS_MAC
#error not implemented.
//#include <os_macos.h>
#endif

#endif // P_PLATFORM_DATA_H

