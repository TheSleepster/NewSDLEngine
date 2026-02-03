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

typedef struct meta_struct meta_struct_t;

// TODO(Sleepster): 
// - [X] Enum support
// - [?] X-Macro for mapping enum member names to their parent enum
//
// - [ ] Array size is not being set (This is because I don't want to rewrite C utilities like strtol() to use length based strings)
// - [X] Anonymous structures need to be handled
// - [X] X-Macro for mapping the member names to structures
// - [X] X-Macro for mapping type names to their type information
// - [X] Modifier flags are broken when output

// NOTE(Sleepster): This is an x-macro that generates a table for mapping 
//                  the sturcture type to an enum 
#define META_STRUCT_TYPE_LIST(X) \
    X(META_STRUCT_TYPE_Struct, "struct") \
    X(META_STRUCT_TYPE_Union,  "union") \
    X(META_STRUCT_TYPE_Enum,   "enum")

typedef enum meta_struct_type 
{
    META_STRUCT_TYPE_Invalid,

#define X(enum, string) enum, 
    META_STRUCT_TYPE_LIST(X)
#undef X

    META_STRUCT_TYPE_Count
}meta_struct_type_t;

#define METATYPE_KIND_LIST(X) \
    X(META_TYPE_KIND_Primitive,    "META_TYPE_KIND_Primitive") \
    X(META_TYPE_KIND_Struct,       "META_TYPE_KIND_Struct") \
    X(META_TYPE_KIND_Enum,         "META_TYPE_KIND_Enum") \
    X(META_TYPE_KIND_Array,        "META_TYPE_KIND_Array") \
    X(META_TYPE_KIND_DynamicArray, "META_TYPE_KIND_DynamicArray") \
    X(META_TYPE_KIND_HashTable,    "META_TYPE_KIND_HashTable") \
    X(META_TYPE_KIND_Invalid,      "META_TYPE_KIND_Invalid") 

typedef enum metatype_kind
{
    // NOTE(Sleepster): Default to primitive 
#define X(enum, string) enum,
    METATYPE_KIND_LIST(X)
#undef X

    META_TYPE_KIND_Count
}metatype_kind_t;

#define METATYPE_FLAG_LIST(X) \
    X(META_TYPE_FLAGS_None,      1u << 0, "META_TYPE_FLAGS_None") \
    X(META_TYPE_FLAGS_Constant,  1u << 1, "META_TYPE_FLAGS_Constant") \
    X(META_TYPE_FLAGS_Volatile,  1u << 2, "META_TYPE_FLAGS_Volatile") \
    X(META_TYPE_FLAGS_Static,    1u << 3, "META_TYPE_FLAGS_Static") \
    X(META_TYPE_FLAGS_Pointer,   1u << 4, "META_TYPE_FLAGS_Pointer") \
    X(META_TYPE_FLAGS_Anonymous, 1u << 5, "META_TYPE_FLAGS_Anonymous")

typedef enum metatype_flags
{
#define X(enum, value, string) enum = value,
    METATYPE_FLAG_LIST(X)
#undef X

    META_TYPE_FLAGS_Count
}metatype_flags_t;

// NOTE(Sleepster): For the type table. We can save this type as a string like we do now,
//                  but when it comes time to add the string to the type table,
typedef struct metatype_data
{
    string_t        type_name;
    metatype_kind_t kind;
    u32             modifier_flags;
    u32             flag_counter;
    u32             pointer_depth;
    u32             array_size;
}metatype_data_t;

typedef struct meta_member
{
    string_t        name;
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

typedef struct type_info_member {
    const char     *name; // string
    u32             type; // types enum
                          
    // NOTE(Sleepster): A lot of this is recycled and repeated, but 
    //                  it's needs to be in a different format here.
    metatype_kind_t kind;
    u32             modifier_flags;
    u32             flag_counter;

    u32             pointer_depth;
    u32             array_size;

    u32             size;
    u32             offset;
}type_info_member_t;

typedef struct type_info_struct {
    const char         *name;           // string
    u32                 type;           // types enum
    metatype_kind_t     kind;           // is it struct? union? enum?
    u32                 modifier_flags; // for anonymous structures

    u32                 size;
    u32                 member_count;
    type_info_member_t *members;
}type_info_struct_t;

typedef struct type_info
{
    const char         *name;
    u32                 type;
    u32                 size;

    type_info_struct_t *struct_info;
}type_info_t;

typedef struct registry_type
{
    string_t        canonical_name;
    metatype_kind_t type_kind;
}registry_type_t;

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

    // NOTE(Sleepster): The hash will map string names of types to indices in the dynamic array below.
    // This way we can make it so that both strings:
    // ast_state
    // and
    // ast_state_t
    // will both map to slot 5, which will store the canonical name for the type as:
    HashTable_t(s64)            type_table_hash;
    DynArray_t(registry_type_t) type_table;
    u32                         type_count;
}ast_state_t;
 
static ast_state_t state;

string_t 
get_metatype_kind_string(metatype_kind_t kind)
{
    string_t result = {};
    switch(kind)
    {
#define X(enum, string) \
        case enum: \
        { \
            result = c_string_make_copy(&global_context->temporary_arena, STR(string)); \
            goto exit; \
        }break; 

    METATYPE_KIND_LIST(X)
#undef X
    }
exit:
    return(result);
}

string_t
get_metatype_flag_string(metatype_flags_t flag)
{
    string_t result = {};
    switch(flag)
    {
#define X(enum, value, string) \
        case enum: \
        { \
            result = c_string_make_copy(&global_context->temporary_arena, STR(string)); \
            goto exit; \
        }break; 

    METATYPE_FLAG_LIST(X)
#undef X
    }
exit:
    return(result);
}

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
    return(result);
}

// TODO(Sleepster): There's a small issue, specifically with hash tables and such. If we find a type with a modifier or hash table or dynamic array,
// the we will save that type as a dynamic array. Even if it's a primitive, or a structure
internal_api void
insert_type_information(metatype_data_t *type_info)
{
    string_t lookup_name = type_info->type_name;

    // NOTE(Sleepster): Fix instances where the typename is something like:
    //
    // example
    //
    // vs
    //
    // example_t
    if(type_info->kind == META_TYPE_KIND_Struct)
    {
        if(c_string_ends_with(lookup_name, STR("_t")))
        {
            Assert(lookup_name.count - 2 > 0);
            lookup_name.count -= 2;
        }
    }
        
    s64 *registry_index = c_hash_table_get_value_ptr(&state.type_table_hash, lookup_name);
    if(*registry_index == -1)
    {
        // NOTE(Sleepster): Type name is not in the hash 
        registry_type_t new_type;
        new_type.canonical_name = type_info->type_name;
        new_type.type_kind      = type_info->kind;

        c_dynarray_push(state.type_table, new_type);
        *registry_index = state.type_count++;
    }
    else if((*registry_index != -1) && type_info->kind == META_TYPE_KIND_Struct)
    {
        // NOTE(Sleepster): It is in the hash, however we might need to update it's canonical name 
        registry_type_t *type = c_dynarray_get_ptr(state.type_table, *registry_index);
        if(c_string_ends_with(type_info->type_name, STR("_t")))
        {
            type->canonical_name = type_info->type_name;
            type->type_kind      = type_info->kind;
        }
    }
}

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
            type_info->flag_counter++;
        }
        else if(c_string_compare(token.string, STR("volatile")))
        {
            type_info->modifier_flags |= META_TYPE_FLAGS_Volatile;
            type_info->flag_counter++;
        }
        else if(c_string_compare(token.string, STR("static")))
        {
            type_info->modifier_flags |= META_TYPE_FLAGS_Volatile;
            type_info->flag_counter++;
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
        type_info->flag_counter++;
        type_info->pointer_depth++;

        token = c_tokenizer_get_next_token(&file_data->tokenizer);
    }

    // NOTE(Sleepster): Name
    member->name = token.string;

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

    // NOTE(Sleepster): Insert the type into the type_table 
    insert_type_information(type_info);


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
        type_info->flag_counter++;
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

                if(c_string_compare(token.string, STR("struct")) ||
                   c_string_compare(token.string, STR("union")))
                {
                    // NOTE(Sleepster): Recursive descent 
                    meta_struct_t *nested_struct = parse_structure(file_data, token);

                    // NOTE(Sleepster): If the structure is anonymous, then there's no way to have any 
                    // type information about it, so just cleanly append it to this structure.
                    if(nested_struct->type_data.modifier_flags & META_TYPE_FLAGS_Anonymous)
                    {
                        for(u32 member_index = 0;
                            member_index < nested_struct->member_count;
                            ++member_index)
                        {
                            meta_member_t new_member = c_dynarray_get_value(nested_struct->members, member_index);

                            c_dynarray_push(structure_info.members, new_member);
                            structure_info.member_count++;
                        }
                    }
                    else
                    {
                        member.nested_struct = nested_struct;
                    }
                        
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
                    insert_type_information(type_info);

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

    if((type_info->modifier_flags & META_TYPE_FLAGS_Anonymous) == 0)
    {
        c_dynarray_push(file_data->structures, structure_info);
        result = c_dynarray_get_value(&file_data->structures, file_data->structure_count);
        file_data->structure_count++;
    }
    else
    {
        result = c_arena_push_struct(&global_context->temporary_arena, meta_struct_t);
        memcpy(result, &structure_info, sizeof(meta_struct_t));
    }

    return(result);
}

internal_api void
append_item_modifier_flags(string_builder_t *builder, u32 object_flag_count, u32 modifier_flags)
{
    u32 flag_count = 0;
    if(object_flag_count == 0)
    {
        string_t flag_string = get_metatype_flag_string(META_TYPE_FLAGS_None);
        c_string_builder_sprint(builder, "%.*s", flag_string.count, C_STR(flag_string));
        
        return;
    }

    for(u32 flag_index = 0;
        flag_index < META_TYPE_FLAGS_Count;
        ++flag_index)
    {
        metatype_flags_t flag = (metatype_flags_t)(1u << flag_index);
        if((modifier_flags & flag) != 0)
        {
            string_t flag_string = get_metatype_flag_string(flag);
            c_string_builder_sprint(builder, "%.*s", flag_string.count, C_STR(flag_string));
            if(flag_count < (object_flag_count - 1))
            {
                c_string_builder_sprint(builder, "|");
            }

            ++flag_count;
        }
        
        if(flag_count >= object_flag_count) return;
    }
}

internal_api void
generate_type_information(ast_file_data_t *ast)
{
    string_builder_t type_enum_builder;
    string_builder_t type_table_builder;
    string_builder_t struct_info_builder;
    string_builder_t struct_static_def_builder;

    c_string_builder_init(&type_enum_builder,         MB(20));
    c_string_builder_init(&type_table_builder,        MB(20));
    c_string_builder_init(&struct_info_builder,       MB(20));
    c_string_builder_init(&struct_static_def_builder, MB(20));

    defer(c_string_builder_deinit(&type_enum_builder));
    defer(c_string_builder_deinit(&type_table_builder));
    defer(c_string_builder_deinit(&struct_info_builder));
    defer(c_string_builder_deinit(&struct_static_def_builder));

    c_string_builder_sprint(&type_enum_builder, "// THIS FILE IS GENERATED BY METAPROGRAM.EXE...\n");
    c_string_builder_sprint(&type_enum_builder, "// IT CONTAINS ALL THE RTTI NEEDED FOR THE PROGRAM...\n\n");
    c_string_builder_sprint(&type_enum_builder, "#if !defined(GENERATED_PROGRAM_RTTI_H)\n");
    c_string_builder_sprint(&type_enum_builder, "#define GENERATED_PROGRAM_RTTI_H\n\n");

    c_string_builder_sprint(&type_enum_builder, "#define GENERATED_PROGRAM_TYPE_LIST(X) \\\n");
    c_string_builder_sprint(&type_table_builder, "\ntype_info_t GENERATED_type_table[] = {\n");

    // NOTE(Sleepster): Generated Type Enum 
    c_dynarray_for(state.type_table, type_index)
    {
        registry_type_t *type = c_dynarray_get_ptr(state.type_table, type_index);
        c_string_builder_sprint(&type_enum_builder, "\tX(TYPE_%.*s, \"%.*s\") \\\n", 
                                type->canonical_name.count, C_STR(type->canonical_name),
                                type->canonical_name.count, C_STR(type->canonical_name));

        c_string_builder_sprint(&type_table_builder, "\t{.name = \"%.*s\", .type = TYPE_%.*s, .size = sizeof(%.*s), ",
                                type->canonical_name.count, C_STR(type->canonical_name),
                                type->canonical_name.count, C_STR(type->canonical_name),
                                type->canonical_name.count, C_STR(type->canonical_name));
        if(type->type_kind == META_TYPE_KIND_Struct || type->type_kind == META_TYPE_KIND_Enum)
        {
            c_string_builder_sprint(&type_table_builder, ".struct_info = (type_info_struct_t*)&type_info_struct_%.*s_const_data},\n",
                                    type->canonical_name.count, C_STR(type->canonical_name));
        }
        else
        {
            c_string_builder_sprint(&type_table_builder, ".struct_info = %s},\n", "null");
        }
    }

    c_string_builder_sprint(&type_enum_builder, "\n\n");
    c_string_builder_sprint(&type_enum_builder, "enum GENERATED_program_type_t { \n");
    c_string_builder_sprint(&type_enum_builder, "#define X(enum, string) enum,  \n");
    c_string_builder_sprint(&type_enum_builder, "GENERATED_PROGRAM_TYPE_LIST(X)\n");
    c_string_builder_sprint(&type_enum_builder, "#undef X\n");
    c_string_builder_sprint(&type_enum_builder, "};\n\n");


    c_string_builder_sprint(&type_table_builder, "};\n");
    // NOTE(Sleepster): Generate the functions that map the type string to the index that the correct type data occupies. 
    // TODO(Sleepster): Maybe we should just stuff all this into a header, I just don't want to depend on another file right now 
    c_string_builder_sprint(&type_enum_builder, R"(
#define META_STRUCT_TYPE_LIST(X) \
    X(META_STRUCT_TYPE_Struct, "struct") \
    X(META_STRUCT_TYPE_Union,  "union") \
    X(META_STRUCT_TYPE_Enum,   "enum")

typedef enum meta_struct_type 
{
    META_STRUCT_TYPE_Invalid,

#define X(enum, string) enum, 
    META_STRUCT_TYPE_LIST(X)
#undef X

    META_STRUCT_TYPE_Count
}meta_struct_type_t;

#define METATYPE_KIND_LIST(X) \
    X(META_TYPE_KIND_Primitive,    "META_TYPE_KIND_Primitive") \
    X(META_TYPE_KIND_Struct,       "META_TYPE_KIND_Struct") \
    X(META_TYPE_KIND_Enum,         "META_TYPE_KIND_Enum") \
    X(META_TYPE_KIND_Array,        "META_TYPE_KIND_Array") \
    X(META_TYPE_KIND_DynamicArray, "META_TYPE_KIND_DynamicArray") \
    X(META_TYPE_KIND_HashTable,    "META_TYPE_KIND_HashTable") \
    X(META_TYPE_KIND_Invalid,      "META_TYPE_KIND_Invalid") 

typedef enum metatype_kind
{
    // NOTE(Sleepster): Default to primitive 
#define X(enum, string) enum,
    METATYPE_KIND_LIST(X)
#undef X

    META_TYPE_KIND_Count
}metatype_kind_t;

#define METATYPE_FLAG_LIST(X) \
    X(META_TYPE_FLAGS_None,      1u << 0, "META_TYPE_FLAGS_None") \
    X(META_TYPE_FLAGS_Constant,  1u << 1, "META_TYPE_FLAGS_Constant") \
    X(META_TYPE_FLAGS_Volatile,  1u << 2, "META_TYPE_FLAGS_Volatile") \
    X(META_TYPE_FLAGS_Static,    1u << 3, "META_TYPE_FLAGS_Static") \
    X(META_TYPE_FLAGS_Pointer,   1u << 4, "META_TYPE_FLAGS_Pointer") \
    X(META_TYPE_FLAGS_Anonymous, 1u << 5, "META_TYPE_FLAGS_Anonymous")

typedef enum metatype_flags
{
#define X(enum, value, string) enum = value,
    METATYPE_FLAG_LIST(X)
#undef X

    META_TYPE_FLAGS_Count
}metatype_flags_t;

typedef struct type_info_member {
    const char     *name; // string
    u32             type; // types enum
                          
    metatype_kind_t kind;
    u32             modifier_flags;
    u32             flag_counter;

    u32             pointer_depth;
    u32             array_size;

    u32             size;
    u32             offset;
}type_info_member_t;

typedef struct type_info_struct {
    const char         *name;           // string
    u32                 type;           // types enum
    metatype_kind_t     kind;           // is it struct? union? enum?
    u32                 modifier_flags; // for anonymous structures

    u32                 size;
    u32                 member_count;
    type_info_member_t *members;
}type_info_struct_t;

typedef struct type_info
{
    const char         *name;
    u32                 type;
    u32                 size;

    type_info_struct_t *struct_info;
}type_info_t;
)");

    string_t builder_string = c_string_builder_get_current_string(&type_enum_builder);
    fprintf(stdout, "%.*s\n", builder_string.count, C_STR(builder_string));

    // NOTE(Sleepster): Generate the type_info_t for the data structures 
    c_dynarray_for(ast->structures, type_index)
    {
        meta_struct_t *structure = c_dynarray_get_ptr(ast->structures, type_index);

        c_string_builder_sprint(&struct_info_builder, "struct type_info_struct_%.*s {\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\tu32 size;\n");
        c_string_builder_sprint(&struct_info_builder, "\tu32 member_count;\n");
        c_string_builder_sprint(&struct_info_builder, "\tunion {\n");
        c_string_builder_sprint(&struct_info_builder, "\t\ttype_info_member_t member_array[%d];\n", structure->member_count);
        c_string_builder_sprint(&struct_info_builder, "\t\tstruct {\n");
        c_dynarray_for(structure->members, member_index)
        {
            meta_member_t *member = c_dynarray_get_ptr(structure->members, member_index);
            c_string_builder_sprint(&struct_info_builder, "\t\t\ttype_info_member_t %.*s;\n", 
                                    member->name.count, C_STR(member->name));
        }
        c_string_builder_sprint(&struct_info_builder, "\t\t}members;\n");
        c_string_builder_sprint(&struct_info_builder, "\t};\n");
        c_string_builder_sprint(&struct_info_builder, "};\n\n");
    }
    c_string_builder_sprint(&struct_info_builder, "\n");

    // NOTE(Sleepster): Generate the type_info_t for the enum data. 
    c_dynarray_for(ast->enums, enum_index)
    {
        meta_struct_t *enum_data = c_dynarray_get_ptr(ast->enums, enum_index);
        c_string_builder_sprint(&struct_info_builder, "struct type_info_enum_%.*s {\n", enum_data->type_data.type_name.count, C_STR(enum_data->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\tu32 size;\n");
        c_string_builder_sprint(&struct_info_builder, "\tu32 member_count;\n");
        c_string_builder_sprint(&struct_info_builder, "\tunion {\n");
        c_string_builder_sprint(&struct_info_builder, "\t\ttype_info_member_t member_array[%d];\n", enum_data->member_count);
        c_string_builder_sprint(&struct_info_builder, "\t\tstruct {\n");
        c_dynarray_for(enum_data->members, member_index)
        {
            meta_member_t *member = c_dynarray_get_ptr(enum_data->members, member_index);
            c_string_builder_sprint(&struct_info_builder, "\t\t\ttype_info_member_t %.*s;\n", 
                                    member->name.count, C_STR(member->name));
        }
        c_string_builder_sprint(&struct_info_builder, "\t\t}members;\n");
        c_string_builder_sprint(&struct_info_builder, "\t};\n");
        c_string_builder_sprint(&struct_info_builder, "};\n\n");
    }
    c_string_builder_sprint(&struct_info_builder, "\n");

    // NOTE(Sleepster): Generate a const static definiton of the data for those structures 
    c_dynarray_for(ast->structures, type_index)
    {
        meta_struct_t *structure = c_dynarray_get_ptr(ast->structures, type_index);

        c_string_builder_sprint(&struct_info_builder, "const static type_info_struct_%.*s type_info_struct_%.*s_const_data = {\n", 
                                structure->type_data.type_name.count, C_STR(structure->type_data.type_name),
                                structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.name = \"%.*s,\",\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.type = TYPE_%.*s,\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));

        string_t structure_kind = get_metatype_kind_string(structure->type_data.kind);
        c_string_builder_sprint(&struct_info_builder, "\t.kind = %.*s,\n", structure_kind.count, C_STR(structure_kind));

        c_string_builder_sprint(&struct_info_builder, "\t.modifier_flags = ");
        append_item_modifier_flags(&struct_info_builder, structure->type_data.flag_counter, structure->type_data.modifier_flags);

        c_string_builder_sprint(&struct_info_builder, ",\n");
        c_string_builder_sprint(&struct_info_builder, "\t.size = sizeof(%.*s),\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.member_count = %d,\n", structure->member_count);

        c_string_builder_sprint(&struct_info_builder, "\t.members = {\n");
        c_dynarray_for(structure->members, member_index)
        {
            meta_member_t *member = c_dynarray_get_ptr(structure->members, member_index);

            string_t kind_string = get_metatype_kind_string(member->type_info.kind);
            c_string_builder_sprint(&struct_info_builder, "\t\t.%.*s = {.name = \"%.*s\", .type = TYPE_%.*s, .kind = %.*s, .modifier_flags = ",
                                    member->name.count, C_STR(member->name),
                                    member->type_info.type_name.count, C_STR(member->type_info.type_name),
                                    member->type_info.type_name.count, C_STR(member->type_info.type_name),
                                    kind_string.count, C_STR(kind_string));
            append_item_modifier_flags(&struct_info_builder, member->type_info.flag_counter, member->type_info.modifier_flags);

            c_string_builder_sprint(&struct_info_builder, ", .flag_counter = %d, .pointer_depth = %d, .array_size = %d, .size = sizeof(%.*s), .offset = offsetof(%.*s, %.*s)},\n",
                                    member->type_info.flag_counter, 
                                    member->type_info.pointer_depth,
                                    member->type_info.array_size,
                                    structure->type_data.type_name.count, C_STR(structure->type_data.type_name),
                                    structure->type_data.type_name.count, C_STR(structure->type_data.type_name),
                                    member->name.count, C_STR(member->name));
        }
        c_string_builder_sprint(&struct_info_builder, "\t}\n");
        c_string_builder_sprint(&struct_info_builder, "};\n");
    }
    c_string_builder_sprint(&struct_info_builder, "\n");

    c_dynarray_for(ast->enums, type_index)
    {
        meta_struct_t *enum_data = c_dynarray_get_ptr(ast->enums, type_index);

        c_string_builder_sprint(&struct_info_builder, "const static type_info_enum_%.*s type_info_enum_%.*s_const_data = {\n", 
                                enum_data->type_data.type_name.count, C_STR(enum_data->type_data.type_name),
                                enum_data->type_data.type_name.count, C_STR(enum_data->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.name = \"%.*s,\",\n", enum_data->type_data.type_name.count, C_STR(enum_data->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.type = TYPE_%.*s,\n", enum_data->type_data.type_name.count, C_STR(enum_data->type_data.type_name));
        c_string_builder_sprint(&struct_info_builder, "\t.kind = META_TYPE_KIND_Enum,\n");
        c_string_builder_sprint(&struct_info_builder, "\t.member_count = %d,\n", enum_data->member_count);

        c_string_builder_sprint(&struct_info_builder, "\t.members = {\n");
        c_dynarray_for(enum_data->members, member_index)
        {
            meta_member_t *member = c_dynarray_get_ptr(enum_data->members, member_index);
            c_string_builder_sprint(&struct_info_builder, "\t\t.%.*s = {.name = \"%.*s\", .type = TYPE_%.*s, .kind = META_TYPE_KIND_Enum, .modifier_flags = META_TYPE_FLAGS_None, .flag_counter = 0, .pointer_depth = 0, .array_size = 0, .size = sizeof(%.*s), .offset = %.*s},\n",
                                    member->name.count,                    C_STR(member->name),                   // member name
                                    member->name.count,                    C_STR(member->name),                   // .name 
                                    enum_data->type_data.type_name.count,  C_STR(enum_data->type_data.type_name), // .type
                                    member->name.count,                    C_STR(member->name),                   // member name
                                    member->name.count,                    C_STR(member->name)                    // .offset
                                    );
        }
        c_string_builder_sprint(&struct_info_builder, "\t}\n");
        c_string_builder_sprint(&struct_info_builder, "};\n");
    }

    c_string_builder_append_builder(&struct_info_builder, &type_table_builder);
    c_string_builder_sprint(&struct_info_builder, "\n");

    // NOTE(Sleepster): Helper functions 
    c_string_builder_sprint(&struct_info_builder, R"(
string_t
c_meta_get_type_string_from_enum(GENERATED_program_type_t type_enum)
{
    string_t result = {};
    switch(type_enum)
    {
#define X(enum, string)           \
        case enum:                \
        {                         \
            result = STR(string); \
            goto exit;            \
        }break;                   

    GENERATED_PROGRAM_TYPE_LIST(X)
#undef X
    }
exit:
    return(result);
}


GENERATED_program_type_t 
c_meta_get_type_enum_from_string(string_t type_string)
{
    GENERATED_program_type_t result = (GENERATED_program_type_t)0;
#define X(enum, string)                                           \
    if(memcmp(string, type_string.data, sizeof(string) - 1) == 0) \
    {                                                             \
        result = enum;                                            \
        goto exit;                                                \
    }                                                             \
    GENERATED_PROGRAM_TYPE_LIST(X)
#undef X

exit:
    return(result);
}

string_t 
c_meta_get_type_kind_string_from_enum(metatype_kind_t kind)
{
    string_t result = {};
    switch(kind)
    {
#define X(enum, string)                                                                 \
        case enum:                                                                      \
        {                                                                               \
            result = c_string_make_copy(&global_context->temporary_arena, STR(string)); \
            goto exit;                                                                  \
        }break; 

    METATYPE_KIND_LIST(X)
#undef X
    }
exit:
    return(result);
}

metatype_kind_t
c_meta_get_type_kind_enum_from_string(string_t kind_string)
{
    metatype_kind_t result = {};
#define X(enum, string)                                           \
    if(memcmp(string, kind_string.data, sizeof(string) - 1) == 0) \
    {                                                             \
        result = enum;                                            \
        goto exit;                                                \
    }                                                             \

    METATYPE_KIND_LIST(X)
#undef X
exit:
    return(result);
}

string_t
c_meta_get_type_flag_string_from_enum(metatype_flags_t flag)
{
    string_t result = {};
    switch(flag)
    {
#define X(enum, value, string)                                                          \
        case enum:                                                                      \
        {                                                                               \
            result = c_string_make_copy(&global_context->temporary_arena, STR(string)); \
            goto exit;                                                                  \
        }break; 

    METATYPE_FLAG_LIST(X)
#undef X
    }
exit:
    return(result);
}

metatype_flags_t
c_meta_get_type_flag_enum_from_string(string_t flag_name)
{
    metatype_flags_t result = {};
#define X(enum, value, string)                                  \
    if(memcmp(string, flag_name.data, sizeof(string) - 1) == 0) \
    {                                                           \
        result = enum;                                          \
        goto exit;                                              \
    }                                                           \

    METATYPE_FLAG_LIST(X)
#undef X
exit:
    return(result);
}

meta_struct_type_t
c_meta_get_struct_type_class_from_string(string_t type)
{
    meta_struct_type_t result = META_STRUCT_TYPE_Invalid;
#define X(enum, string)                                    \
    if(memcmp(string, type.data, sizeof(string) - 1) == 0) \
    {                                                      \
        result = enum;                                     \
        goto exit;                                         \
    }                                                      \

    META_STRUCT_TYPE_LIST(X)
#undef X

exit:
    return(result);
}

string_t
c_meta_get_struct_type_class_from_enum(meta_struct_type_t type)
{
    string_t result = {};
    switch(type)
    {
#define X(enum, string)                                                                 \
        case enum:                                                                      \
        {                                                                               \
            result = c_string_make_copy(&global_context->temporary_arena, STR(string)); \
            goto exit;                                                                  \
        }break; 

    META_STRUCT_TYPE_LIST(X)
#undef X
    }
exit:
    return(result);
}

type_info_t*
c_meta_get_type_info_by_name(string_t type)
{
    type_info_t *result = null;

    GENERATED_program_type_t type_enum = c_meta_get_type_enum_from_string(type);
    result = &GENERATED_type_table[type_enum];

    return(result);
}

type_info_t*
c_meta_get_type_info_by_enum(GENERATED_program_type_t type_enum)
{
    type_info_t *result = null;
    result = &GENERATED_type_table[type_enum];

    return(result);
});

type_info_member_t*
c_meta_get_member_info(type_info_struct_t *struct_info, string_t member_name)
{
    type_info_member_t *result = null;
    for(u32 member_index = 0;
        member_index < struct_info->member_count;
        ++member_index)
    {
        found = struct_info->members + member_index;
        if(c_string_compare(STR(found->name), member_name))
        {
            found = result;
            break;
        }
    }

    return(result);
}

type_info_struct_t*
c_meta_get_struct_info(string_t structure_type_name)
{
    type_info_struct_t *result = null;
    type_info_t *type = c_meta_get_type_info_by_name(structure_type_name);
    result = type->struct_info;

    return(result);
}

)");
    c_string_builder_sprint(&struct_info_builder, "#endif\n");
    
    builder_string = c_string_builder_get_current_string(&struct_info_builder);
    fprintf(stdout, "%.*s\n", builder_string.count, C_STR(builder_string));
     
    // TODO(Sleepster): Define an x macro that will map the name of the structure to it's type information 
}

void
parse_enum(ast_file_data_t *file_data, token_data_t structure_type)
{
    meta_struct_t enum_data  = {};
    enum_data.structure_type = META_STRUCT_TYPE_Enum;
    enum_data.members        = c_dynarray_create(meta_member_t);

    // NOTE(Sleepster): for now we just assume this exists, if this ever doesn't for some reason wtf??? 
    token_data_t enum_type_name_token = c_tokenizer_get_next_token(&file_data->tokenizer);
    Assert(enum_type_name_token.type == TT_Identifier);
    
    metatype_data_t *type_info = &enum_data.type_data;
    type_info->kind      = META_TYPE_KIND_Enum;
    type_info->type_name = enum_type_name_token.string;

    // NOTE(Sleepster): Eat the open brace 
    c_tokenizer_get_next_token(&file_data->tokenizer);

    bool8 parsing = true;
    while(parsing)
    {
        token_data_t token = c_tokenizer_get_next_token(&file_data->tokenizer);
        switch(token.type)
        {
            case TT_Identifier:
            {
                meta_member_t member = {};
                string_t enum_member_name = token.string;

                member.name = enum_member_name;
                type_info->kind           = META_TYPE_KIND_Enum;
                type_info->modifier_flags = META_TYPE_FLAGS_None;
                type_info->flag_counter   = 0;
                type_info->array_size     = 0;

                c_dynarray_push(enum_data.members, member);
                ++enum_data.member_count;
            }break;
            case TT_ClosingBrace:
            {
                // NOTE(Sleepster): Check if there's another TT_Identifier afterwards, this is the name of a C style declared structure.
                token_data_t next_token = c_tokenizer_get_next_token(&file_data->tokenizer);
                if(next_token.type == TT_Identifier)
                {
                    type_info->type_name = next_token.string;
                }

                insert_type_information(type_info);
                parsing = false;
            }break;
            case TT_EOF:
            {
                break;
            }break;
        }
    }

    c_dynarray_push(file_data->enums, enum_data);
    ++file_data->enum_count;
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
                    parse_enum(file, token);
                }
            }break;
            case TT_EOF:
            {
                break;
            }break;
        }
    }
}

int
main(void)
{
    c_global_context_init();

    state.ast_files  = c_dynarray_create(ast_file_data_t);
    state.type_table = c_dynarray_create(registry_type_t);

    c_hash_table_init(&state.type_table_hash, 9187);
    memset(state.type_table_hash.data, -1, sizeof(s64) * 9187);

    ast_file_data_t file = {};
    ZeroStruct(file); 

    file.tokenizer.data = c_file_read_entirety(STR("tests/GENERATED_test.h"));
    build_file_ast(&file);

    generate_type_information(&file);
    c_dynarray_push(state.ast_files, file);

    return(0);
}
