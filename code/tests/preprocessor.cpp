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

#include <GENERATED_test.h>

#define INSPECT

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

    string_builder_t  type_enum_builder;
    DynArray_t(u64)   type_ids;
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
    (void)state;

    eat_whitespace(token_data);
    preprocessor_token_t token = {};

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
            // TODO(Sleepster): Comments 
            //byte *at = token.string.data;
            token.type = TT_Asterisk;      
            if(token_data->data[1] == '/')
            {
                eat_whitespace(token_data);
            }
        }break;
        case '"':  
        {
            byte *at = token_data->data;

            while(token_data->data        && 
                  (token_data->data[0] != '"'))
            {
                if((token_data->data[0] == '\\') && (token_data->data[1]))
                {
                    c_string_advance_by(token_data, 1);
                }
                c_string_advance_by(token_data, 1);
            }

            u64 token_length = (token_data->data - at);

            token.type = TT_String;
            token.string.count = (u32)token_length;

            if(token_data->data[0] == '"')
            {
                c_string_advance_by(token_data, 1);
            }
        }break;
        default:
        {
            if(token_alphabetical((char)token.string.data[0]))
            {
                token.type = TT_Identifier;

                byte *at = token.string.data;
                while((token.string.data) && 
                      (token_alphabetical((char)token.string.data[0]) || 
                      (token_numeric((char)token.string.data[0])) ||
                      (token.string.data[0] == '_') || 
                      (token.string.data[0] == '.')))
                {
                    c_string_advance_by(&token.string, 1);
                }

                u64 token_length = (token.string.data - at);
                token.string.count = (u32)token_length;
                token.string.data  = at;

                c_string_advance_by(token_data, token.string.count);
            }
#if 0
            else if(token_numeric(token_data->data[0]))
            {
            }
#endif
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

internal_api void
append_type_enum_token(preprocessor_token_t type_name_token)
{
    if(type_name_token.type == TT_Identifier)
    {
        Assert(type_name_token.string.count > 1);

        string_t typedef_type_name = c_string_sub_from_left(type_name_token.string, type_name_token.string.count - 2);
        string_t alt_type_name     = c_string_concat(&global_context->temporary_arena, type_name_token.string, STR("_t"));

        u64 type_id = c_fnv_hash_value(type_name_token.string.data, type_name_token.string.count);
        u64 alt_type_id = c_fnv_hash_value(alt_type_name.data, alt_type_name.count);
        u64 typedef_type_id = c_fnv_hash_value(typedef_type_name.data, typedef_type_name.count);

        bool8 ID_found = false;
        c_dynarray_for(state.type_ids, id_index)
        {
            u64 ID = state.type_ids[id_index];
            if(ID == type_id || ID == alt_type_id || ID == typedef_type_id)
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
    }
}

internal_api void
append_struct_member_to_definition(preprocessor_token_t member_type, preprocessor_token_t member_name)
{
    char buffer[256];
    s32 length = sprintf(buffer, "\t\ttype_info_member_t %.*s;\n",
                         member_name.string.count, C_STR(member_name.string));

    string_t struct_info_string = {
        .data  = (byte*)buffer,
        .count = (u32)length
    };
    c_string_builder_append_data(&state.struct_info_builder, struct_info_string);
}

internal_api void
parse_struct_member(string_t *tokenized_data, bool8 is_pointer, preprocessor_token_t struct_name_token, preprocessor_token_t type_token)
{
    preprocessor_token_t last_token = type_token;
    preprocessor_token_t token = get_next_token(tokenized_data);
    switch(token.type)
    {
        case TT_Asterisk: 
        {
            is_pointer = true;
            parse_struct_member(tokenized_data, is_pointer, struct_name_token, last_token);
        }break;
        case TT_Identifier:
        {
            append_type_enum_token(type_token);
            append_struct_member_to_definition(type_token, token);

            printf("\t\t.%.*s = {\"%.*s\", %s, type_%.*s, offsetof(%.*s, %.*s), sizeof(%.*s)},\n", 
                   token.string.count, C_STR(token.string), 
                   token.string.count, C_STR(token.string), 
                   is_pointer ? "true" : "false",
                   type_token.string.count, C_STR(type_token.string),
                   struct_name_token.string.count, C_STR(struct_name_token.string),
                   token.string.count, C_STR(token.string),
                   token.string.count, C_STR(token.string));
            is_pointer = false;
        }break;
    }
}

internal_api s32 
parse_structure(string_t *tokenized_data, preprocessor_token_t struct_name_token)
{   
    s32 member_count = 0;

    preprocessor_token_t token = get_next_token(tokenized_data);
    if(token.type == TT_OpeningBrace)
    {
        char buffer[256];
        s32 length = sprintf(buffer, "struct type_info_%.*s_t {\n\tconst char *name;\n\tu32 type;\n\tstruct members {\n", 
                             struct_name_token.string.count, C_STR(struct_name_token.string));
        
        string_t struct_info_string = {
            .data  = (byte*)buffer,
            .count = (u32)length
        };
        c_string_builder_append_data(&state.struct_info_builder, struct_info_string);

        printf("const static type_info_%.*s_t type_info_%.*s = {\n", 
               struct_name_token.string.count, C_STR(struct_name_token.string), 
               struct_name_token.string.count, C_STR(struct_name_token.string));

        printf("\t\"%.*s\",\n\tTYPE_%.*s,\n", 
               struct_name_token.string.count, 
               C_STR(struct_name_token.string),
               struct_name_token.string.count, 
               C_STR(struct_name_token.string));

        printf("\t.members = {\n");
        for(;;)
        {
            token = get_next_token(tokenized_data);
            if(token.type == TT_ClosingBrace)
            {
                token = get_next_token(tokenized_data);
                append_type_enum_token(token);

                c_string_builder_append_data(&state.struct_info_builder, STR("\t};\n\tu32 member_count\n};"));
                break;
            }
            else
            {
                parse_struct_member(tokenized_data, false, struct_name_token, token);
                ++member_count;
            }
        }
        printf("\t},\n");
    }

    return(member_count);
}

int 
main(int argc, char **argv)
{
    c_global_context_init();

    file_t type_enum_file = c_file_open(STR("GENERATED_program_types_enum.h"), true);

    c_string_builder_init(&state.type_enum_builder,   MB(200));
    c_string_builder_init(&state.struct_info_builder, MB(200));
    state.type_ids = c_dynarray_create(u64);

    c_string_builder_append_data(&state.type_enum_builder, STR("// THIS IS GENERATED BY THE PREPROCESSOR\n// THIS IS THE RTTI FOR THE ENTIRE PROGRAM\n\n"));
    c_string_builder_append_data(&state.type_enum_builder, STR("#ifndef GENERATED_PROGRAM_TYPES_H\n#define GENERATED_PROGRAM_TYPES_H\n\n"));
    c_string_builder_append_data(&state.type_enum_builder, STR("enum GENERATED_program_types_t {\n"));

    // TODO(Sleepster): feed a directory to this 
    //string_t file_data = c_file_read_entirety(STR("r_vulkan_types.h"));
    string_t file_data = c_file_read_entirety(STR("c_globals.h"));
    //fprintf(stdout, "%s", C_STR(file_data));
    Assert(file_data.data);
    Assert(file_data.count > 0);

    // NOTE(Sleepster): Get tokens for file... 
    while(file_data.count > 0)
    {
        preprocessor_token_t token = get_next_token(&file_data);
        switch(token.type)
        {
            case TT_Invalid:
            {
            }break;
            case TT_EOF:
            {
                goto end;
            }break;
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("struct")))
                {
                    token = get_next_token(&file_data);
                    s32 member_count = parse_structure(&file_data, token);
                    if(member_count > 0)
                    {
                        printf("\t%d\n", member_count);
                        printf("};\n");
                    }
                }
            }break;
        }
        c_global_context_reset_temporary_data();
    }
    // NOTE(Sleepster): Get tokens for file... 
end:
    c_string_builder_append_data(&state.type_enum_builder, STR("};\n"));
    c_string_builder_append_data(&state.type_enum_builder, STR("#endif // GENERATED_PROGRAM_TYPES_H\n"));
    string_t builder_string = c_string_builder_get_current_string(&state.type_enum_builder);
    fprintf(stdout, "%s", C_STR(builder_string));


    builder_string = c_string_builder_get_current_string(&state.struct_info_builder);
    fprintf(stdout, "%s\n", C_STR(builder_string));
    c_string_builder_reset(&state.struct_info_builder);


    c_string_builder_flush_to_file(&type_enum_file, &state.type_enum_builder);

    return(0);
}
