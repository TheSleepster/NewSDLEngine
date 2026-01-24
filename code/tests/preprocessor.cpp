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
    TT_EOF,

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
get_next_token(string_t file_data)
{
    preprocessor_token_t result = {};
    string_t current_line = c_string_read_line(&file_data);

    //string_t current_line_copy = c_string_make_copy(&global_context->temporary_arena, current_line);
    //current_line_copy.data[current_line_copy.count] = '\0';
    //log_info("%s", C_STR(current_line_copy));

    while(current_line.data)
    {
        eat_whitespace(&current_line);
        char character = *current_line.data;

        result.string = {
            .data  = current_line.data,
            .count = 1 
        };

        c_string_advance_by(&current_line, 1);
        switch(character)
        {
            // TODO(Sleepster): Closing bracket
            case ';':  {result.type = TT_Semicolon;        }break;
            case '{':  {result.type = TT_OpeningBrace;     }break;
            case '}':  {result.type = TT_ClosingBrace;     }break;
            case '(':  {result.type = TT_OpeningParen;     }break;
            case ')':  {result.type = TT_ClosingParen;     }break;
            case '[':  {result.type = TT_OpenBracket;      }break;
            case ']':  {result.type = TT_ClosingBracket;   }break;
            case ',':  {result.type = TT_Comma;            }break;
            case '<':  {result.type = TT_OpenAngleBracket; }break;
            case '>':  {result.type = TT_CloseAngleBracket;}break;
            case '\0': {result.type = TT_EOF;              }break;
            case '*':  
                       {
                           // TODO(Sleepster): Comments 
                           //byte *at = result.string.data;
                           result.type = TT_Asterisk;      
                       }break;
            case '"':  
                       {
                           byte *at = current_line.data;

                           while(current_line.data        && 
                                 (current_line.data[0] != '"'))
                           {
                               if((current_line.data[0] == '\\') && (current_line.data[1]))
                               {
                                   c_string_advance_by(&current_line, 1);
                               }
                               c_string_advance_by(&current_line, 1);
                           }

                           u64 token_length = (current_line.data - at);

                           result.type = TT_String;
                           result.string.count = (u32)token_length;

                           if(current_line.data[0] == '"')
                           {
                               c_string_advance_by(&current_line, 1);
                           }
                       }break;
            default:
                       {
                           if(token_alphabetical((char)current_line.data[0]))
                           {
                               result.type = TT_Identifier;

                               byte *at = result.string.data;
                               while((result.string.data) && 
                                     (token_alphabetical((char)current_line.data[0]) || 
                                      (token_numeric((char)current_line.data[0])) ||
                                      (current_line.data[0] == '_') || 
                                      (current_line.data[0] == '.')))
                               {
                                   c_string_advance_by(&current_line, 1);
                               }

                               u64 token_length = (current_line.data - at);

                               result.type = TT_String;
                               result.string.count = (u32)token_length;
                           }
#if 0
                           else if(result_numeric(current_line.data[0]))
                           {
                           }
#endif
                           else
                           {
                               result.type = TT_Invalid;
                               if(current_line.count > 0)
                               {
                                   c_string_advance_by(&current_line, 1);
                               }
                           }
                       }break;
        }
        if(!is_end_of_line(&current_line))
        {
        }
        else
        {
            break;
        }
    }
    c_global_context_reset_temporary_data();

    return(result);
}

int 
main(void)
{
    c_global_context_init();

    string_t file_data = c_file_read_entirety(STR("tests/preprocessor.cpp"));
    //fprintf(stdout, "%s", C_STR(file_data));
    Assert(file_data.data);
    Assert(file_data.count > 0);

    // NOTE(Sleepster): Get tokens for file... 
    while(file_data.count > 0)
    {
        preprocessor_token_t token = get_next_token(file_data);
        switch(token.type)
        {
            case TT_Invalid:
            {
            }break;
            default:
            {
                printf("Token type is: '%d'... string is: '%.*s'...\n", token.type, token.string.count,  C_STR(token.string));
            }break;
        }
    }
    // NOTE(Sleepster): Get tokens for file... 

    return(0);
}
