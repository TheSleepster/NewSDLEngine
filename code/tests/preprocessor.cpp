/* ========================================================================
   $File: preprocessor.cpp $
   $Date: January 23 2026 09:01 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>
#include <c_file_api.h>
#include <c_string.h>

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

#define INSPECT

typedef struct test_element 
{
    u32 test_element1;
}test_element_t;

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
    string_t token_data;
}preprocessor_state_t;

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
parse_struct_member(string_t *tokenized_data)
{
    preprocessor_token_t token = get_next_token(tokenized_data);
    switch(token.type)
    {
        case TT_Asterisk: 
        {
            parse_struct_member(tokenized_data);
        }break;
        case TT_Identifier:
        {
            printf("\t\"%.*s\"\n", token.string.count,  C_STR(token.string));
        };
    }
}

internal_api void
parse_structure(string_t *tokenized_data)
{
    preprocessor_token_t token = get_next_token(tokenized_data);
    if(token.type == TT_OpeningBrace)
    {
        for(;;)
        {
            token = get_next_token(tokenized_data);
            if(token.type == TT_ClosingBrace)
            {
                break;
            }
            else
            {
                parse_struct_member(tokenized_data);
            }
        }
    }
}

int 
main(void)
{
    c_global_context_init();

    string_t file_data = c_file_read_entirety(STR("r_vulkan_types.h"));
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
                    printf("const char* %.*s_member_names[] = {\n", token.string.count,  C_STR(token.string));
                    parse_structure(&file_data);
                    printf("};\n");
                }
            }break;
        }
        c_global_context_reset_temporary_data();
    }
    // NOTE(Sleepster): Get tokens for file... 
end:
    return(0);
}
