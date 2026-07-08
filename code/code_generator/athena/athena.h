#if !defined(ATHENA_H)
/* ========================================================================
   $File: athena.h $
   $Date: July 03 2026 02:26 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_H
#include <string.h>

struct type_info_t;
struct type_info_member_t;
struct type_info_struct_t;
struct type_info_procedure_t;

enum athena_reflection_type 
{
    ATHENA_METATYPE_INFO_PRIMITIVE,
    ATHENA_METATYPE_INFO_STRUCT,
    ATHENA_METATYPE_INFO_ENUM,
    ATHENA_METATYPE_INFO_PROCEDURE,
};

enum athena_evaluated_type 
{
    ATHENA_TYPE_INT32,
    ATHENA_TYPE_INT64,
    ATHENA_TYPE_UNSIGNED32,
    ATHENA_TYPE_UNSIGNED64,
    ATHENA_TYPE_FLOAT32,
    ATHENA_TYPE_FLOAT64,
    ATHENA_TYPE_STRING,
};

struct type_default_value_t 
{
    unsigned int type;
    union {
        int                int32;
        long long          int64;
        unsigned int       u32;
        unsigned long long u64;
        float              float32;
        double             float64;
        const char        *string;
    };
};

struct type_info_t
{
    const char  *type_name;
    type_info_t *alias_of;
    type_info_t *next_overload;
    unsigned int metatype;
    unsigned int type_id;
    unsigned int size;
};

struct type_info_member_t
{
    const type_info_t        *type_info;
    const char               *member_name;
    const type_info_struct_t *parent;
    unsigned int              offset;
    //unsigned_int            array_size;
    unsigned int              flags;
    unsigned int              pointer_depth;

    type_default_value_t      value;
};

struct type_info_struct_t
{
    const type_info_t type_info;
    unsigned int      member_count;
};

struct type_info_procedure_t
{
    type_info_t               type_info;
    unsigned int              argument_count;
    const type_info_t        *return_type;
    const type_info_member_t *arguments;
};


struct athena_reflection_bundle_t
{
    const type_info_t *type_info_array;
    unsigned int       type_info_array_size;
};

extern const type_info_t **athena_type_information_array;

// function
void athena_handle_type_info(const char *filepath, int directory, int recursive);

#ifdef ATHENA_IMPLMENETATION
# define ATHENA_API 
#else
#define ATHENA_API extern
#endif

ATHENA_API inline const type_info_t *type_info(unsigned long long type_id);
ATHENA_API inline const type_info_t *type_info(const char *string);

#ifdef C_STRING_H
ATHENA_API inline const type_info_t *type_info(string_t string);
#endif

// NOTE(Sleepster): Templates
template <typename T>
ATHENA_API inline const type_info_t*
type_info()
{
    return(nullptr);
}

#ifndef ATHENA_GENERATED_FILE_H
#define ATHENA_RTTI_COMPLETE_TYPE_LIST(X)
#endif

#define X(cpp_type, type_id, structure, string) \
    template <> \
    inline const type_info_t* \
    type_info<cpp_type>() { \
        return(static_cast<const type_info_t *>(structure)); \
    }
    
    ATHENA_RTTI_COMPLETE_TYPE_LIST(X)
#undef X

template<typename T>
ATHENA_API inline const type_info_t*
type_info(const T &item)
{
    return(type_info<T>());
}

// NOTE(Sleepster): STB style lib
#ifdef C_STRING_H
ATHENA_API inline const type_info_t        *type_info(string_t string);
ATHENA_API inline const type_info_member_t *athena_get_member_info(const type_info_t *type_info, string_t member_name);
#endif

ATHENA_API inline const type_info_t        *type_info(unsigned long long type_id);
ATHENA_API inline const type_info_t        *type_info(const char *string);
ATHENA_API inline const type_info_member_t *athena_get_member_info(const type_info_t *type_info, const char *member_name);
ATHENA_API inline const type_info_struct_t *athena_get_struct_info_from_member(const type_info_t *info);
ATHENA_API inline const type_info_struct_t *athena_get_struct_info_from_member(const type_info_member_t *member);

#endif // ATHENA_H

