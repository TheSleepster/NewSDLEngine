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

// TODO(Sleepster): 
// - [ ] X-Macro for mapping the member names to structures
// - [ ] X-Macro for mapping enum member names to their parent enum
// - [ ] X-Macro for mapping type names to their type information

// NOTE(Sleepster): This is an x-macro that generates a table for mapping 
//                  the sturcture type to an enum 
#define META_STRUCT_TYPE_LIST(X) \
    X(META_STRUCT_TYPE_Struct, "struct") \
    X(META_STRUCT_TYPE_Union, "union") \
    X(META_STRUCT_TYPE_Enum, "enum")

typedef enum meta_struct_type 
{
    META_STRUCT_TYPE_Invalid,

#define X(enum, string) enum, 
    META_STRUCT_TYPE_LIST(X)
#undef X

    META_STRUCT_TYPE_Count
}meta_struct_type_t;

meta_struct_type_t
get_structure_type_from_string(string_t type)
{
    meta_struct_type_t result = META_STRUCT_TYPE_Invalid;
    switch(type.count)
    {
    // NOTE(Sleepster): Uses the same x-macro trick from above to find out which it belongs too.
    //
    // In this case, it generates something that looks like:
#define X(enum, string)                                            \
        case sizeof(string) - 1:                                   \
        {                                                          \
            if(memcmp(string, type.data, sizeof(string) - 1) == 0) \
            {                                                      \
                result = enum;                                     \
                goto exit;                                         \
            }                                                      \
        }break; 

    // NOTE(Sleepster): And then we pass whatever X is here, to our table macro. 
    //                  It takes care of this and generates the rest. This will generate a switch
    //                  case statement for every member of the list we created automatically.
    META_STRUCT_TYPE_LIST(X)
#undef X
    }

exit:
    return result;
}

typedef struct meta_struct meta_struct_t;
typedef enum metatype_kind
{
    META_TYPE_KIND_Invalid,
    META_TYPE_KIND_Primitive, // int, float, uint, etc.
    META_TYPE_KIND_Struct, // struct, union
    META_TYPE_KIND_Enum,
    META_TYPE_KIND_Array,
    META_TYPE_KIND_DynamicArray,
    META_TYPE_KIND_HashTable,
    META_TYPE_KIND_Count
}metatype_kind_t;

typedef enum metatype_flags
{
    META_TYPE_FLAGS_None,
    META_TYPE_FLAGS_Constant,
    META_TYPE_FLAGS_Volatile,
    META_TYPE_FLAGS_Static,
    META_TYPE_FLAGS_Pointer,
    META_TYPE_FLAGS_Anonymous,
    META_TYPE_FLAGS_Count
}metatype_flags_t;

// NOTE(Sleepster): For the type table. We can save this type as a string like we do now,
//                  but when it comes time to add the string to the type table,
typedef struct metatype_data
{
    string_t        type_name;
    metatype_kind_t kind;
    u32             modifier_flags;
    u32             pointer_depth;
    u32             array_size;
}metatype_data_t;

typedef struct meta_member
{
    string_t        name;
    u32             type;
    metatype_data_t type_info;

    meta_struct_t  *nested_struct;
}meta_member_t;

typedef struct meta_struct
{
    metatype_data_t           type_data;
    meta_struct_type_t        structure_type;
    u32                       member_count;
    DynArray_t(meta_member_t) members;
}meta_struct_t;

typedef struct ast_file_data
{
    tokenizer_t               tokenizer;

    DynArray_t(meta_struct_t) structures;
    u32                       structure_count;

    DynArray_t(meta_struct_t) enums;
    u32                       enum_count;
}ast_file_data_t;

typedef struct ast_state 
{
    DynArray_t(ast_file_data) ast_files;
    u32                      file_count;

    HashTable_t(string_t)    type_table;
    u32                      type_count;
}ast_state_t;
 
static ast_state_t state;

internal_api void
parse_member_data(ast_file_data_t *file_data, 
                  meta_struct_t   *structure, 
                  meta_member_t   *member,
                  token_data_t     token)
{
    metatype_data_t *type_info = &member->type_info;
        
    // NOTE(Sleepster): Cleans up and sets modifiers 
    for(;;)
    {
        if(c_string_compare(token.string, STR("const")))
        {
            type_info->modifier_flags |= META_TYPE_FLAGS_Constant;
        }
        else if(c_string_compare(token.string, STR("volatile")))
        {
            type_info->modifier_flags |= META_TYPE_FLAGS_Volatile;
        }
        else if(c_string_compare(token.string, STR("static")))
        {
            type_info->modifier_flags |= META_TYPE_FLAGS_Volatile;
        }
        else
        {
            break;
        }
        token = c_tokenizer_get_next_token(&file_data->tokenizer);
    }

    // NOTE(Sleepster): If a special type of mine, clear it. 
    bool8 is_dynarray   = c_string_compare(token.string, STR("DynArray_t"));
    bool8 is_hash_table = c_string_compare(token.string, STR("HashTable_t"));
    if(is_dynarray || is_hash_table)
    {
        // NOTE(Sleepster): First Eats the special macro token
        c_tokenizer_get_next_token(&file_data->tokenizer);

        // NOTE(Sleepster): Then, eats the open paren, setting this to the type the special macro
        // holds.
        token = c_tokenizer_get_next_token(&file_data->tokenizer);

        // NOTE(Sleepster): Finally, sets the kind. 
        if(is_dynarray)   type_info->kind = META_TYPE_KIND_DynamicArray;
        if(is_hash_table) type_info->kind = META_TYPE_KIND_HashTable;
    }
    // NOTE(Sleepster): If we see this at this point, it's a C style structure definition like:
    //
    // typedef struct thing {
    //      struct thing *next_thing;
    // }thing_t;
    if(c_string_compare(token.string, STR("struct")) ||
       c_string_compare(token.string, STR("union")))
    {
        type_info->kind = META_TYPE_KIND_Struct;
        c_tokenizer_get_next_token(&file_data->tokenizer);
    }
    // NOTE(Sleepster): Should be the type
    type_info->type_name = token.string;

    // NOTE(Sleepster): If we used a special type, then this will be a closing paren
    // if we used a pointer, then this will be an asterisk. In either case, advance
    token = c_tokenizer_get_next_token(&file_data->tokenizer);
    if(token.type == TT_ClosingParen)
    {
        token = c_tokenizer_get_next_token(&file_data->tokenizer);
    }
    else if(token.type == TT_Asterisk)
    {
        type_info->modifier_flags |= META_TYPE_FLAGS_Pointer;
        type_info->pointer_depth++;

        token = c_tokenizer_get_next_token(&file_data->tokenizer);
    }

    // NOTE(Sleepster): Name
    member->name = token.string;
    fprintf(stdout, "Member: '%.*s %.*s' found...\n", 
            type_info->type_name.count, C_STR(type_info->type_name),
            member->name.count,         C_STR(member->name));

    // NOTE(Sleepster): Check if this is an array... 
    token = c_tokenizer_get_next_token(&file_data->tokenizer);
    if(token.type == TT_OpenBracket)
    {
        type_info->kind = META_TYPE_KIND_Array;

        // NOTE(Sleepster): Get the array size;
        token = c_tokenizer_get_next_token(&file_data->tokenizer);
        Assert(token.type == TT_Number);
         
        // TODO(Sleepster): Fix the read value functions inside c_string.cpp
        //type_info->array_size = c_string_read_u32(token.string);
    }

    // NOTE(Sleepster): Once we have the name and the array size, 
    //                  we care nothing about what comes after...
    while(token.type != TT_Semicolon)
    {
        token = c_tokenizer_get_next_token(&file_data->tokenizer);
    }
}

internal_api meta_struct_t* 
parse_structure(ast_file_data *file_data, token_data_t structure_type)
{
    meta_struct_t *result = null;

    meta_struct_t structure_info = {};
    structure_info.structure_type = get_structure_type_from_string(structure_type.string);
    structure_info.members        = c_dynarray_create(meta_member_t);

    token_data_t type_name_token = c_tokenizer_get_next_token(&file_data->tokenizer);

    // TODO(Sleepster): add the type name to a list of types????
    metatype_data_t *type_info = &structure_info.type_data;
    type_info->kind      = META_TYPE_KIND_Struct;
    type_info->type_name = type_name_token.string;

    if(type_name_token.type != TT_Identifier &&
       type_name_token.type == TT_OpeningBrace)
    {
        // NOTE(Sleepster): Anonymous 
        type_info->modifier_flags |= META_TYPE_FLAGS_Anonymous;
    }
    else if(type_name_token.type == TT_Identifier)
    {
        // NOTE(Sleepster): Eat the open brace 
        c_tokenizer_get_next_token(&file_data->tokenizer);
    }

    token_data_t token = type_name_token;
    bool8 parsing      = true;
    while(parsing)
    {
        token = c_tokenizer_get_next_token(&file_data->tokenizer);
        switch(token.type)
        {
            case TT_Identifier:
            {
                meta_member_t member = {};
                // TODO(Sleepster): How do we want deal with this? Do we want the structures
                // to be members of the main structure as well? Do we want the member to 
                // have to worry about whether or not this can happen???
                if(c_string_compare(token.string, STR("struct")) ||
                   c_string_compare(token.string, STR("union")))
                {
                    // NOTE(Sleepster): Recursive descent 
                    member.nested_struct = parse_structure(file_data, token);
                    break;
                }
                parse_member_data(file_data, &structure_info, &member, token);

                c_dynarray_push(structure_info.members, member);
                structure_info.member_count++;
            }break;
            case TT_ClosingBrace:
            {
                // TODO(Sleepster): check the next token, if it's an TT_Identifier 
                // than we have a typedef struct, if it's a semicolon then it's a normal C++ 
                // structure.
                token = c_tokenizer_get_next_token(&file_data->tokenizer);
                if(token.type == TT_Identifier)
                {
                    // TODO(Sleepster): Append this to the type table;
                    type_info->type_name = token.string;
                    token = c_tokenizer_get_next_token(&file_data->tokenizer);
                }
                parsing = false;
            }break;
            case TT_EOF:
            {
                break;
            }break;
        }
    }
    c_dynarray_push(file_data->structures, structure_info);
    result = c_dynarray_get_value(&file_data->structures, file_data->structure_count);
    file_data->structure_count++;

    return(result);
}

internal_api void
build_file_ast(ast_file_data_t *file)
{
    while(file->tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&file->tokenizer);
        switch(token.type)
        {
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("struct")) || 
                   c_string_compare(token.string, STR("union")))
                {
                    parse_structure(file, token);
                }
                else if(c_string_compare(token.string, STR("enum")))
                {
                    // nothing for the time being
                }
            }break;
            case TT_EOF:
            {
                break;
            }break;
        }
    }
}

internal_api void
generate_type_enum_table_data(ast_file_data_t *file_data)
{
    c_dynarray_for(&file_data->structures, structure_index)
    {
        meta_struct_t *structure = c_dynarray_get_value(&file_data->structures, structure_index);
        c_dynarray_for(&structure->members, member_index)
        {
            // TODO(Sleepster): This. 
            meta_member_t *member = c_dynarray_get_value(&structure->members, member_index);
        }
    }
}

int
main(void)
{
    state.ast_files = c_dynarray_create(ast_file_data_t);
    c_hash_table_init(&state.type_table, 9187);

    ast_file_data_t file = {};
    ZeroStruct(file); 

    file.tokenizer.data = c_file_read_entirety(STR("tests/GENERATED_test.h"));
    build_file_ast(&file);
    generate_type_enum_table_data(&file);
#if 0
    generate_type_information(&file);
#endif

    c_dynarray_push(state.ast_files, file);

    return(0);
}
