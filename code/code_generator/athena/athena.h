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
    ATHENA_METATYPE_PRIMITIVE,
    ATHENA_METATYPE_STRUCT,
    ATHENA_METATYPE_ENUM,
    ATHENA_METATYPE_PROCEDURE,
};

enum athena_evaluated_type 
{
    ATHENA_VALUE_TYPE_INVALID,
    ATHENA_VALUE_TYPE_INT32,
    ATHENA_VALUE_TYPE_INT64,
    ATHENA_VALUE_TYPE_UNSIGNED32,
    ATHENA_VALUE_TYPE_UNSIGNED64,
    ATHENA_VALUE_TYPE_FLOAT32,
    ATHENA_VALUE_TYPE_FLOAT64,
    ATHENA_VALUE_TYPE_STRING,
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
    // NOTE(Sleepster): Either type_info_struct_t, or type_info_procedure_t 
    const type_info_t        *parent;

    unsigned int              offset;
    //unsigned_int            array_size;
    unsigned int              flags;
    unsigned int              pointer_depth;

    type_default_value_t      value;
};

struct type_info_struct_t
{
    const type_info_t         type_info;
    unsigned int              member_count;
    const type_info_member_t *members;
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

struct attribute_info_list_t {
    const char               *attribute_name;
    const type_info_t *const *infos;
    u32                       info_count;

    const type_info_t *const *begin() { return(infos);              }
    const type_info_t *const *end()   { return(infos + info_count); }

    const type_info_t *const *begin() const { return(infos);                }
    const type_info_t *const *end()   const { return((infos + info_count)); }
};

// function
void athena_handle_type_info(const char *filepath, int directory, int recursive);

#ifdef ATHENA_IMPLEMENTATION
# define ATHENA_API 
#else
#define ATHENA_API extern
#endif

namespace athena_internal {
    template<typename T, unsigned int N = sizeof(T)>
    constexpr unsigned int safe_sizeof_impl(int) { return N; }

    template<typename T>
    constexpr unsigned int safe_sizeof_impl(...) { return 0; }

    template<typename T>
    constexpr unsigned int safe_sizeof() { return safe_sizeof_impl<T>(0); }
}

extern const type_info_t *const athena_type_information_array[];

// function
void athena_handle_type_info(const char *filepath, int directory, int recursive);

#ifdef ATHENA_IMPLMENETATION
# define ATHENA_API 
#else
#define ATHENA_API extern
#endif

namespace Athena {

// NOTE(Sleepster): STB style lib
#ifdef C_STRING_H
ATHENA_API const type_info_t        *type_info(string_t string);
ATHENA_API const type_info_t        *type_info(unsigned long long type_id);
ATHENA_API const type_info_t        *type_info(const char *string);
ATHENA_API const type_info_member_t *get_member(const type_info_t *type_info, string_t member_name);
#endif
ATHENA_API const type_info_member_t *get_member(const type_info_t *type_info, const char *member_name);
ATHENA_API const type_info_member_t *get_member(const type_info_struct_t *type_info, const char *member_name);

ATHENA_API const type_info_struct_t *get_struct_info_from_member(const type_info_t *info);
ATHENA_API const type_info_struct_t *get_struct_info_from_member(const type_info_member_t *member);

ATHENA_API const type_info_procedure_t *as_procedure(const type_info_t *info);
ATHENA_API const type_info_procedure_t *as_procedure(const type_info_member_t *info);

ATHENA_API const type_info_struct_t *as_structure(const type_info_t *info);
ATHENA_API const type_info_struct_t *as_structure(const type_info_member_t *info);

ATHENA_API const attribute_info_list_t *get_attribute_list(char *name);
}

#define CODE_GEN_IGNORE_FILE
#define CODE_GEN_IGNORE_DECL

#endif // ATHENA_H

