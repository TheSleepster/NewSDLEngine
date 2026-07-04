#if !defined(ATHENA_H)
/* ========================================================================
   $File: athena.h $
   $Date: July 03 2026 02:26 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_H
typedef unsigned int unsigned_int;

struct type_info_t;
struct type_info_member_t;
struct type_info_struct_t;
struct type_info_procedure_t;

enum athena_reflection_type 
{
    ATHENA_TYPE_INFO_PRIMITIVE,
    ATHENA_TYPE_INFO_STRUCT,
    ATHENA_TYPE_INFO_ENUM,
    ATHENA_TYPE_INFO_PROCEDURE,
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
    unsigned_int type;
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
    unsigned int type;
    unsigned int size;
};

struct type_info_member_t: public type_info_t
{
    const char               *member_name;
    const type_info_struct_t *parent;
    //unsigned_int              array_size;
    unsigned_int              flags;
    unsigned_int              pointer_depth;

    type_default_value_t      value;
};

struct type_info_struct_t: public type_info_t
{
    unsigned_int        member_count;
    type_info_member_t *members;
};

struct type_info_procedure_t: public type_info_t
{
    unsigned_int        argument_count;
    type_info_member_t *arguments;
    type_info_t        *return_type;
};

struct athena_reflection_bundle_t
{
    const type_info_t *type_info_array;
    unsigned_int       type_info_array_size;
};

extern const type_info_t athena_type_information_array[];

// function
void athena_handle_type_info(const char *filepath, int directory, int recursive);

// NOTE(Sleepster): STB style lib? 
#ifdef ATHENA_IMPLMENETATION 
#endif // ATHENA_IMPLMENETATION 

#endif // ATHENA_H

