/* ========================================================================
   $File: preprocessor.cpp $
   $Date: January 23 2026 09:01 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>

#define HASH_TABLE_IMPLEMENTATION
#include <c_hash_table.h>

#include <c_file_api.h>
#include <c_string.h>
#include <p_platform_data.h>

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#include <c_program_flag_handler.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>

#include <preprocessor_type_data.h>

// TODO(Sleepster): 
// - [X] UNIONS (NESTED AND UNNESTED)
// - [X] NESTED STRUCTURES
// - [ ] IS_CONSTANT MEMBER BOOL
// - [ ] VOLATILE MEMBER BOOL
// - [ ] LENGTH BASED ARRAY TYPES (U32 INDICES[32]... THIS MIGHT ALREADY BE HANDLED THOUGH USING ARRAYSIZE()...)
// - [ ] MULTITHREADING???
// - [ ] IGNORE #if 0 blocks
// - [ ] REALLY JUST HANDLE ANY # BLOCK
// - [ ] MERGE ALL THE IS_* FLAGS (IS_POINTER, IS_CONSTANT, IS_VOLATILE, ETC.) INTO FLAGS
// - [ ] GET_STRUCT_INFO() FUNCTION TO SWITCH-CASE ON THE STRUCTURE_TYPE
// - [ ] FIGURE OUT A BETTER WAY TO DEAL WITH INSTANCES LIKE ZONE_ALLOCATOR_BLOCK...

#if 0
struct type_info_internal_members_t {
        const char *name;
        u32 type;
        struct members {
                type_info_member_t apples;
        };
        u32 member_count;
};

struct test_element_data 
{
    u32 oranges;
    u32 internal_data[128];
    struct internal_members {
        u32 apples;
    };

    union {
        u32 banannas;
        u32 grapes;
        u32 tomatos;
    };
}

struct type_info_test_element_data_t {
        const char *name;
        u32 type;
        u32 member_count;
        struct members {
            type_info_member_t oranges;
            type_info_member_t internal_data;

            struct {
                type_info_member_t apples;
            }internal_members;

            type_info_member_t banannas;
            type_info_member_t grapes;
            type_info_member_t tomatos;
        };
};

#endif

typedef enum preprocessor_token_type
{
    TT_Invalid,

    TT_Semicolon,
    TT_Colon,
    TT_OpeningBrace,
    TT_ClosingBrace,
    TT_OpeningParen,
    TT_ClosingParen,
    TT_Asterisk,
    TT_OpenBracket,
    TT_ClosingBracket,
    TT_Comma,
    TT_OpenAngleBracket,
    TT_CloseAngleBracket,
    TT_HashTag,
    TT_Exclamation,
    TT_EOF,

    TT_Error,
    TT_Identifier,
    TT_String,

    TT_Count
}preprocessor_token_type_t;

typedef struct preprocessor_token
{
    preprocessor_token_type_t type;
    string_t                  string;
}preprocessor_token_t;

typedef struct preprocessor_state
{
    string_t          token_data;

    string_builder_t  struct_info_builder;
    string_builder_t  struct_const_definition_builder;
    string_builder_t  type_enum_builder;

    DynArray_t(u64)      type_ids;
}preprocessor_state_t;
static preprocessor_state_t state = {};

internal_api inline bool32
is_end_of_line(string_t *current_line)
{
    bool32 result = false;

    char character = current_line->data[0]; 
    if(character == '\n' ||
       character == '\r' || 
       current_line->count == 0)
    {
        result = true;
    }

    return(result);
}

internal_api inline bool32
is_whitespace(string_t *current_line)
{
    bool32 result = false;

    char character = current_line->data[0];
    if(character == ' '  ||
       character == '\n' ||
       character == '\r' ||
       character == '\t')
    {
        result = true;
    }

    return(result);
}

internal_api inline bool8
token_alphabetical(char A)
{
    bool8 result = (((A >= 'a') && (A <= 'z')) || 
                    ((A >= 'A') && (A <= 'Z')));
    return(result);
}

internal_api inline bool8
token_numeric(char A)
{
    bool8 result = ((A >= '0') && (A <= '9'));
    return(result);
}

internal_api inline void
eat_whitespace(string_t *current_line)
{
    while(current_line->count > 0)
    {
        if(is_whitespace(current_line))
        {
            c_string_advance_by(current_line, 1);
        }
        else if(current_line->data[0] == '/' &&
                current_line->data[1] == '/')
        {
            c_string_advance_by(current_line, 2);
            while(!is_end_of_line(current_line))
            {
                c_string_advance_by(current_line, 1);
            }
        }
        else if(current_line->data[0] == '/' &&
                current_line->data[1] == '*')
        {
            c_string_advance_by(current_line, 2);
            while(current_line->data[0] && 
                 (current_line->count > 0) && 
                 !((current_line->data[0] == '*') && 
                   (current_line->data[1] == '/')))
            {
                c_string_advance_by(current_line, 1);
            }

            if(current_line->data[0] == '*')
            {
                c_string_advance_by(current_line, 2);
            }
        }
        else
        {
            break;
        }
    }
}

internal_api preprocessor_token_t
get_next_token(string_t *token_data)
{
    eat_whitespace(token_data);
    
    preprocessor_token_t token = {};
    if(token_data->count == 0)
    {
        token.type = TT_EOF;
        return token;
    }

    token.string = {
        .data  = token_data->data,
        .count = 1 
    };

    char character = token_data->data[0];

    c_string_advance_by(token_data, 1);
    switch(character)
    {
        case ';':  {token.type = TT_Semicolon;        }break;
        case ':':  {token.type = TT_Colon;            }break;
        case '{':  {token.type = TT_OpeningBrace;     }break;
        case '}':  {token.type = TT_ClosingBrace;     }break;
        case '(':  {token.type = TT_OpeningParen;     }break;
        case ')':  {token.type = TT_ClosingParen;     }break;
        case '[':  {token.type = TT_OpenBracket;      }break;
        case ']':  {token.type = TT_ClosingBracket;   }break;
        case ',':  {token.type = TT_Comma;            }break;
        case '<':  {token.type = TT_OpenAngleBracket; }break;
        case '>':  {token.type = TT_CloseAngleBracket;}break;
        case '#':  {token.type = TT_HashTag;          }break;
        case '!':  {token.type = TT_Exclamation;      }break;
        case '\0': {token.type = TT_EOF;              }break;
        case '*':  
        {
            token.type = TT_Asterisk;      
            if(token_data->count > 0 && token.string.data[0] == '/')
            {
                eat_whitespace(token_data);
            }
        }break;
        case '"':  
        {
            byte *at = token_data->data;

            while(token_data->data && 
                  (token_data->data[0] != '"'))
            {
                if((token_data->data[0] == '\\') && (token_data->data[1]))
                {
                    c_string_advance_by(token_data, 2);
                }
                else
                {
                    c_string_advance_by(token_data, 1);
                }
            }

            if(token_data->data[0] == '"')
            {
                c_string_advance_by(token_data, 1);
            }
            u64 token_length = (token_data->data - at);

            token.type = TT_String;
            token.string.count = (u32)token_length;
        }break;
        default:
        {
            if(token_alphabetical((char)token.string.data[0]) || character == '_')
            {
                token.type = TT_Identifier;
                while(token_data->count > 0 && 
                      (token_alphabetical((char)token_data->data[0]) || 
                       token_numeric((char)token_data->data[0]) ||
                       token_data->data[0] == '_' || 
                       token_data->data[0] == '.'))
                {
                    c_string_advance_by(token_data, 1);
                }

                token.string.count = (u32)(token_data->data - token.string.data);
            }
            else
            {
                token.type = TT_Invalid;
                if(token_data->count > 0)
                {
                    c_string_advance_by(token_data, 1);
                }
            }
        }break;
    }

    return(token);
}

internal_api bool8 
append_type_enum_token(preprocessor_token_t type_name_token)
{   
    bool8 was_found = false;

    if(type_name_token.type == TT_Identifier)
    {
        string_t alt_type_name = c_string_concat(&global_context->temporary_arena, type_name_token.string, STR("_t"));
        string_t non_type_name = type_name_token.string;
        non_type_name.count -= 2;

        u64 type_id      = c_fnv_hash_value(type_name_token.string.data, type_name_token.string.count);
        u64 alt_type_id  = c_fnv_hash_value(alt_type_name.data, alt_type_name.count);
        u64 non_typed_id = c_fnv_hash_value(non_type_name.data, non_type_name.count);

        bool8 ID_found = false;
        c_dynarray_for(state.type_ids, id_index)
        {
            u64 ID = state.type_ids[id_index];
            if(ID == type_id || ID == alt_type_id || ID == non_typed_id)
            {
                ID_found = true;
                break;
            }
        }

        if(!ID_found)
        {
            c_dynarray_push(state.type_ids, type_id);
            char buffer[256];
            s32 length = sprintf(buffer, "\tTYPE_%.*s,\n", type_name_token.string.count, type_name_token.string.data);

            string_t type_name_string = {
                .data  = (byte*)buffer,
                .count = (u32)length
            };

            c_string_builder_append_data(&state.type_enum_builder, type_name_string);
        }
        was_found = ID_found;
    }

    return(was_found);
}

internal_api inline preprocessor_token_t
peek_next_token(string_t token_data, u32 times = 1)
{
    preprocessor_token_t result = {};
    for(u32 peek_index = 0;
        peek_index < times;
        ++peek_index)
    {
        result = get_next_token(&token_data);
    }
    return(result);
}

internal_api void
parse_member(preprocessor_token_t structure_name, 
             preprocessor_token_t type_token, 
             string_t            *tokenized_data, 
             string_builder_t    *local_type_info_builder, 
             string_builder_t    *local_const_definition_builder,
             string_builder_t    *struct_member_builder,
             bool8                c_style_struct)
{
    preprocessor_token_t element_identifier;
    preprocessor_token_t type_identifier = type_token;

    bool8 name_found = false;
    bool8 is_pointer = false;
    for(;;)
    {
        preprocessor_token_t token = get_next_token(tokenized_data);
        switch(token.type)
        {
            case TT_Asterisk:
            {
                is_pointer = true;
            }break;
            case TT_Identifier:
            {
                bool8 is_constant         = c_string_compare(type_identifier.string, STR("const"));
                bool8 is_volatile         = c_string_compare(type_identifier.string, STR("volatile"));
                bool8 is_dynarray         = c_string_compare(type_identifier.string, STR("DynArray_t"));
                bool8 is_hash_table       = c_string_compare(type_identifier.string, STR("HashTable_t"));

                // NOTE(Sleepster): If we find a modifier, breakout and advance.
                if(is_constant || is_volatile || is_hash_table || is_dynarray)
                {
                    // NOTE(Sleepster): If we're in here, there's type qualifiers that we must skip for the true type 
                    type_identifier = token;
                    break;
                }

                // NOTE(Sleepster): If we reach here, this is a name or type, save it as the element_identifier.
                if(!name_found)
                {
                    element_identifier = token;
                    name_found = true;
                }
            }break;
            case TT_Semicolon:
            {
                append_type_enum_token(type_identifier);
                if(c_style_struct)
                {
                    string_t struct_name_tail = type_identifier.string;
                    struct_name_tail.data += struct_name_tail.count - 2;
                    struct_name_tail.count = 2;
                    if(!c_string_compare(struct_name_tail, STR("_t")))
                    {
                        type_identifier.string = c_string_concat(&global_context->temporary_arena, type_identifier.string, STR("_t"));
                    }
                }

                // NOTE(Sleepster): format the type information 
                char buffer[8192];

                // NOTE(Sleepster): The most recently found element_identifier is exactly what we need.
                s32 length = sprintf(buffer, "\t\ttype_info_member_t %.*s;\n", element_identifier.string.count, C_STR(element_identifier.string));
                string_t test_string = {
                    .data  = (byte*)buffer,
                    .count = (u32)length
                };
                c_string_builder_append_data(local_type_info_builder, test_string);

                preprocessor_token_t type_identifier_COPY = type_identifier;
                if(is_pointer)
                {
                    type_identifier_COPY.string = c_string_concat(&global_context->temporary_arena, 
                                                                  type_identifier_COPY.string, 
                                                                  STR("*"));
                    is_pointer = false;
                }

                // NOTE(Sleepster): format the const definition information 
                memset(buffer, 0, sizeof(buffer));
                length = sprintf(buffer, "\t\t.%.*s = {.name = \"%.*s\", .type = TYPE_%.*s, .offset = offsetof(%.*s, %.*s), .size = sizeof(%.*s)},\n",
                                 element_identifier.string.count,   C_STR(element_identifier.string),    // initialized name
                                 element_identifier.string.count,   C_STR(element_identifier.string),    // .name
                                 type_identifier.string.count,      C_STR(type_identifier.string),       // .type
                                 structure_name.string.count,       C_STR(structure_name.string),        // .offset structure
                                 element_identifier.string.count,   C_STR(element_identifier.string),    // .offset element name
                                 type_identifier_COPY.string.count, C_STR(type_identifier_COPY.string)); // .size
                test_string = {
                    .data  = (byte*)buffer,
                    .count = (u32)length
                };
                c_string_builder_append_data(struct_member_builder, test_string);

                return;
            }break;
        }
    }
}

// NOTE(Sleepster): structure_type_token is for telling us whether this structure is a struct or a union... 
internal_api preprocessor_token_t 
parse_structure(string_t *tokenized_data, preprocessor_token_t structure_type_token)
{
    string_builder_t local_type_info_builder;
    string_builder_t local_const_definition_builder;
    string_builder_t struct_member_builder;

    u32 member_count = 0;

    c_string_builder_init(&local_type_info_builder, MB(10));
    c_string_builder_init(&local_const_definition_builder, MB(10));
    c_string_builder_init(&struct_member_builder, MB(10));

    // NOTE(Sleepster): Peeking too see if it's an anonymous structure. If it is, just put it inline
    preprocessor_token_t struct_name_token = peek_next_token(*tokenized_data);
    if(struct_name_token.type == TT_OpeningBrace)
    {
        return(struct_name_token);
    }

    struct_name_token = get_next_token(tokenized_data);
    string_t struct_name_tail = struct_name_token.string;
    struct_name_tail.data += struct_name_tail.count - 2;
    struct_name_tail.count = 2;
    if(!c_string_compare(struct_name_tail, STR("_t")))
    {
        struct_name_token.string = c_string_concat(&global_context->temporary_arena, struct_name_token.string, STR("_t"));
    }

    append_type_enum_token(struct_name_token);

    // NOTE(Sleepster): This checks if it's simply a struct declaration. If it is, we're out.
    preprocessor_token_t struct_declaration = peek_next_token(*tokenized_data);
    if(struct_declaration.type != TT_OpeningBrace) 
    {
        return(struct_name_token);
    }

    // NOTE(Sleepster): Build the local version of the struct info 
    char buffer[8192];

    s32 length = sprintf(buffer, "struct type_info_%.*s {\n\tconst char *name;\n\tu32 type;\n\tu32 member_count;\n\tstruct {\n", 
                         struct_name_token.string.count, C_STR(struct_name_token.string));
    string_t test_string = {
        .data  = (byte*)buffer,
        .count = (u32)length
    };
    c_string_builder_append_data(&local_type_info_builder, test_string);

    // NOTE(Sleepster): Do the same thing for the const definition 
    memset(buffer, 0, sizeof(buffer));

    length = sprintf(buffer, "const static type_info_%.*s type_info_%.*s = {\n\t.name = \"%.*s\",\n\t.type = TYPE_%.*s,\n", 
                     struct_name_token.string.count, C_STR(struct_name_token.string),
                     struct_name_token.string.count, C_STR(struct_name_token.string),
                     struct_name_token.string.count, C_STR(struct_name_token.string),
                     struct_name_token.string.count, C_STR(struct_name_token.string));
    test_string = {
        .data  = (byte*)buffer,
        .count = (u32)length
    };
    c_string_builder_append_data(&local_const_definition_builder, test_string);

    bool8 c_style_struct = false;
    // NOTE(Sleepster): Loop until the end of this structure or the file 
    for(;;)
    {
        preprocessor_token_t token = get_next_token(tokenized_data);
        switch(token.type)
        {
            case TT_Identifier:
            {
                // NOTE(Sleepster): If we find a nested structure, parse it.
                if(c_string_compare(token.string, STR("struct")) ||
                   c_string_compare(token.string, STR("union")))
                {
                    // NOTE(Sleepster): If we find this, then we need to make note of the fact that this 
                    //                  structure is not actually owned by the program, it is simply internal to 
                    //                  the structure we are currently in, and thus we should not calculate the offsetof 
                    //                  like "offsetof(thing_inside_this_struct, element)". Instead it should 
                    //                  be "offsetof(struct_thing.thing_inside_this_struct, element)" 

                    // NOTE(Sleepster): Peek ahead twice... This is to check if it's a C style declaration
                    // something like:
                    // typedef struct element
                    // {
                    //      struct element *first_element;
                    // }element_t;
                    //
                    // This is behavior I want to support.
                    preprocessor_token_t peeked = peek_next_token(*tokenized_data, 2);
                    if(peeked.type != TT_OpeningBrace)
                    {
                        c_style_struct = true;
                        break;
                    }

                    // NOTE(Sleepster): After we return from parse_structure, we've left the TT_Semicolon for the structure, just eat it.
                    preprocessor_token_t nested_struct_name = parse_structure(tokenized_data, token);
                    if(nested_struct_name.type != TT_OpeningBrace)
                    {
                        get_next_token(tokenized_data);
                    }
                    break;
                }

                string_t struct_name_tail = struct_name_token.string;
                struct_name_tail.data += struct_name_tail.count - 2;
                struct_name_tail.count = 2;
                if(!c_string_compare(struct_name_tail, STR("_t")))
                {
                    struct_name_token.string = c_string_concat(&global_context->temporary_arena, struct_name_token.string, STR("_t"));
                }

                // NOTE(Sleepster): Parse the member until the TT_Semicolon 
                parse_member(struct_name_token, 
                             token, 
                             tokenized_data, 
                             &local_type_info_builder, 
                             &local_const_definition_builder, 
                             &struct_member_builder,
                             c_style_struct);

                c_style_struct = false;
                member_count++;
            }break;
            case TT_ClosingBrace:
            {
                ZeroMemory(buffer, sizeof(buffer));

                length = sprintf(buffer, "\t.member_count = %d,\n\t.members = {\n", member_count); 
                test_string = {
                    .data  = (byte*)buffer,
                    .count = (u32)length
                };
                c_string_builder_append_data(&local_const_definition_builder, test_string);

                string_t member_string = c_string_builder_get_current_string(&struct_member_builder);
                c_string_builder_append_data(&local_const_definition_builder, member_string);

                c_string_builder_append_data(&local_const_definition_builder, STR("\t}\n};\n\n"));
                c_string_builder_append_data(&local_type_info_builder, STR("\t}members;\n};\n\n"));
            };
            case TT_EOF:
            {
                goto end;
            }break;
        }
    }
end: 
    string_t type_info_builder_string = c_string_builder_get_current_string(&local_type_info_builder);
    c_string_builder_append_data(&state.struct_info_builder, type_info_builder_string);

    string_t const_def_builder_string = c_string_builder_get_current_string(&local_const_definition_builder);
    c_string_builder_append_data(&state.struct_const_definition_builder, const_def_builder_string);

    return(struct_name_token);
}

VISIT_FILES(generate_file_metadata)
{
    string_t filename = visit_file_data->filename;
    string_t file_ext = c_string_get_file_ext_from_path(filename);
    if(!c_string_compare(file_ext, STR(".h")))
    {
        return;
    }
    if(c_string_compare(filename, STR("preprocessor_type_data.h"))) return;
    if(c_string_compare(filename, STR("c_math.h"))) return;

    string_t file_desc = {
        .count = 4,
        .data  = filename.data
    };
    if(c_string_compare(file_desc, STR("sys_")))
    {
        return;
    }

    string_t file_data = c_file_read_entirety(filename);
    Assert(file_data.data);
    Assert(file_data.count > 0);

    // NOTE(Sleepster): Get tokens for file... 
    while(file_data.count > 0)
    {
        // NOTE(Sleepster): We need to set this loop up so that it works like this:
        // - Look for the struct / union tag
        // - If we find it, parse the structure. Inside parse_structure, we need to check each line for a member. 
        //   If we find a modifier like const or volatile, we need to more forward once

        preprocessor_token_t token = get_next_token(&file_data);
        switch(token.type)
        {
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("GENERATE_TYPE_INFO")))
                {
                    get_next_token(&file_data);
                    get_next_token(&file_data);
                    // NOTE(Sleepster): With this, we know this is an item we wish to generate metadata for... 
                    parse_structure(&file_data, token);
                }
            }break;
            case TT_Invalid:
            {
            }break;
            case TT_EOF:
            {
                goto end;
            }break;
        }
    }
end:
    c_global_context_reset_temporary_data();
}

int 
main(int argc, char **argv)
{
    c_global_context_init();

    c_string_builder_init(&state.type_enum_builder,               MB(200));
    c_string_builder_init(&state.struct_const_definition_builder, MB(200));
    c_string_builder_init(&state.struct_info_builder,             MB(200));
    state.type_ids = c_dynarray_create(u64);

    c_string_builder_append_data(&state.type_enum_builder, STR("// THIS IS GENERATED BY THE PREPROCESSOR\n// THIS IS THE RTTI FOR THE ENTIRE PROGRAM\n\n"));
    c_string_builder_append_data(&state.type_enum_builder, STR("#ifndef GENERATED_PROGRAM_TYPES_H\n#define GENERATED_PROGRAM_TYPES_H\n\n#include <preprocessor_type_data.h>\n\n"));
    c_string_builder_append_data(&state.type_enum_builder, STR("enum GENERATED_program_types_t {\n"));

    // TODO(Sleepster): feed a directory to this 
    //string_t file_data = c_file_read_entirety(STR("tests/GENERATED_test.h"));
    //string_t file_data = c_file_read_entirety(STR("tests/GENERATED_test2.h"));
    //string_t file_data = c_file_read_entirety(STR("c_globals.h"));
    //fprintf(stdout, "%s", C_STR(file_data));

#if 1 
    visit_file_data_t visit_info = c_directory_create_visit_data(generate_file_metadata, false, null);
    c_directory_visit(STR("../code"), &visit_info);
#else
    string_t file_data = c_file_read_entirety(STR("tests/GENERATED_test2.h"));
    Assert(file_data.data);
    Assert(file_data.count > 0);

    // NOTE(Sleepster): Get tokens for file... 
    while(file_data.count > 0)
    {
        // NOTE(Sleepster): We need to set this loop up so that it works like this:
        // - Look for the struct / union tag
        // - If we find it, parse the structure. Inside parse_structure, we need to check each line for a member. 
        //   If we find a modifier like const or volatile, we need to more forward once

        preprocessor_token_t token = get_next_token(&file_data);
        switch(token.type)
        {
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("GENERATE_TYPE_INFO")))
                {
                    get_next_token(&file_data);

                    // NOTE(Sleepster): With this, we know this is an item we wish to generate metadata for... 
                    parse_structure(&file_data, token);
                }
            }break;
            case TT_Invalid:
            {
            }break;
            case TT_EOF:
            {
                goto end;
            }break;
        }
    }
end:
    c_global_context_reset_temporary_data();
#endif
    
    c_string_builder_append_data(&state.type_enum_builder, STR("};\n"));

    string_t builder_string = c_string_builder_get_current_string(&state.type_enum_builder);
    fprintf(stdout, "%s\n", C_STR(builder_string));

    builder_string = c_string_builder_get_current_string(&state.struct_info_builder);
    fprintf(stdout, "%s\n", C_STR(builder_string));

    c_string_builder_append_data(&state.struct_const_definition_builder, STR("#endif // GENERATED_PROGRAM_TYPES_H\n"));

    builder_string = c_string_builder_get_current_string(&state.struct_const_definition_builder);
    fprintf(stdout, "%s\n", C_STR(builder_string));


    return(0);
}
