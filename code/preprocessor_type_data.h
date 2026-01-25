#if !defined(PREPROCESSOR_TYPE_DATA_H)
/* ========================================================================
   $File: preprocessor_type_data.h $
   $Date: January 24 2026 10:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define PREPROCESSOR_TYPE_DATA_H
#include <c_types.h>

#define MAX_STRUCTURE_MEMBERS (128)

typedef struct type_info_member
{
    const char *name;
    const char *type_name;
    u32         offset;
    u32         size;
}type_info_member_t;

typedef struct type_info_struct
{
    const char         *name;
    type_info_member_t  members[MAX_STRUCTURE_MEMBERS];
    u32                 member_count;
}type_info_struct_t;

#if 0

// NOTE(Sleepster): Predefined, we make this
typedef struct member_type_info
{
    const char *name;
    bool8       is_pointer;
    u32         offset;
    u32         size;
    u32         type;
}member_type_info_t;

// NOTE(Sleepster): Predefined, we use this as a generic 
struct type_info_struct
{
    const char         *name;
    u32                 type;
    type_info_member_t *members;
    u32                 member_count;
}type_info_struct_t;

// NOTE(Sleepster): This is generated...
typedef enum GENERATED_program_types
{
    Type_bool8,
    Type_memory_arena_t,
    ...
}GENERATED_program_types_t;

// NOTE(Sleepster): So is this...
struct type_info_global_context_t 
{
    const char *name;
    u32         type;
    struct members {
        member_type_info_t is_initialized;
        member_type_info_t running;
        ...
    };
    u32 member_count;
};

// NOTE(Sleepster): And so is this.
struct type_info_global_context_t type_info_global_context = {
    .name = "global_context_t",
    .type = TYPE_global_context_t;
    .members = {
        .is_initialized = {.name = "is_initialized", .offset = offsetof(global_context_t, is_initalized), .size = sizeof(bool8), .type = ProgramType_bool8},
        .running  = {.name = "running", .offset = offsetof(global_context_t, running), .size = sizeof(bool8), .type = ProgramType_bool8},
    },
};

#endif

#endif // PREPROCESSOR_TYPE_DATA_H

