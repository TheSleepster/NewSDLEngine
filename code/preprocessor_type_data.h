#if !defined(PREPROCESSOR_TYPE_DATA_H)
/* ========================================================================
   $File: preprocessor_type_data.h $
   $Date: January 24 2026 10:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define PREPROCESSOR_TYPE_DATA_H
#include <c_types.h>

// NOTE(Sleepster): PLACE THIS MACRO ON A STURCTURE TO GENERATE TYPE INFORMATION ABOUT IT
#define GENERATE_TYPE_INFO

// TODO(Sleepster): Merge all the is_* flags (is_pointer, is_constant, is_volatile, etc.) into flags

typedef struct type_info_member
{
    const char *name;
    const char *type_name;
    u32         type;
    u32         offset;
    u32         size;
}type_info_member_t;

// NOTE(Sleepster): Predefined, we use this as a generic 
//                  We can simply cast other type_info_struct_* to this.
typedef struct type_info_struct
{
    const char          *name;
    u32                  type;
    u32                  member_count;
    type_info_member_t  *members;
}type_info_struct_t;

#endif // PREPROCESSOR_TYPE_DATA_H

