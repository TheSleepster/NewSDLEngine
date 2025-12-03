#if !defined(C_STRING_H)
/* ========================================================================
   $File: c_string.h $
   $Date: December 03 2025 03:26 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_STRING_H
#include <c_base.h>
#include <c_types.h>

struct string_t 
{
    byte *data;
    u32   count;
};

#define STR(pointer) (string_t){.data = (byte*)pointer, .count = (u32)strlen(pointer)}

#endif // C_STRING_H

