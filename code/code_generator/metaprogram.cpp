/* ========================================================================
   $File: metaprogram.cpp $
   $Date: February 01 2026 10:59 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>

#define HASH_TABLE_IMPLEMENTATION
#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#include <c_hash_table.h>
#include <c_file_api.h>
#include <c_string.h>
#include <c_tokenizer.h>
#include <c_program_flag_handler.h>

#include <p_platform_data.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_tokenizer.cpp>

typedef enum metatype_kind
{
    META_TYPE_Invalid,
    META_TYPE_Basic, // int, float, uint, etc.
    META_TYPE_Struct, // struct, union
    META_TYPE_Enum,
    META_TYPE_Array,
    META_TYPE_DynamicArray,
    META_TYPE_HashTable,
    META_TYPE_Count
}metatype_kind_t;

typedef enum metatype_flags
{
    META_TYPE_FLAGS_None,
    META_TYPE_FLAGS_Constant,
    META_TYPE_FLAGS_Volatile,
    META_TYPE_FLAGS_Pointer,
    META_TYPE_FLAGS_Count
}metatype_flags_t;

typedef struct metatype
{
    string_t        name;
    metatype_kind_t kind;
    u32             type_flags;
    u32             pointer_depth;
    u32             array_size;
}metatype_t;

typedef struct meta_member
{
    string_t        name;
    metatype_kind_t kind;
}metamember_t;

typedef struct meta_struct
{
    string_t        name;
    u32             member_count;
    metamember_t   *members;
}meta_struct_t;

int
main(void)
{
    tokenizer_t tokenizer = {};
    tokenizer.data = c_file_read_entirety(STR("tests/GENERATED_test.h"));
    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        switch(token.type)
        {
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("struct")) || 
                   c_string_compare(token.string, STR("union")))
                {
                }
                else if(c_string_compare(token.string, STR("enum")))
                {
                    // nothing for the time being
                }
            }break;
        }
    }

    return(0);
}
