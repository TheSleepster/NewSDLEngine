#if !defined(APOLLO_H)
/* ========================================================================
   $File: apollo.h $
   $Date: October 09 2026 04:04 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define APOLLO_H
#include <string.h>

struct type_info_t;
struct type_info_member_t;
struct type_info_struct_t;
struct type_info_procedure_t;

enum athena_reflection_type 
{
    APOLLO_METATYPE_PRIMITIVE,
    APOLLO_METATYPE_STRUCT,
    APOLLO_METATYPE_ENUM,
    APOLLO_METATYPE_PROCEDURE,
};

struct type_info_t
{
    const char  *type_name;
    unsigned int metatype;
    unsigned int size;
};

#endif // APOLLO_H

