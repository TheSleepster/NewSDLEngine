/* ========================================================================
   $File: c_tokenizer.cpp $
   $Date: January 31 2026 11:15 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_tokenizer.h>

bool8
c_tokenizer_token_alphabetical(char A)
{
    bool8 result = (((A >= 'a') && (A <= 'z')) || 
                    ((A >= 'A') && (A <= 'Z')));
    return(result);
}

bool8
c_tokenizer_token_numeric(char A)
{
    bool8 result = ((A >= '0') && (A <= '9'));
    return(result);
}

token_data_t 
c_tokenizer_get_next_token(tokenizer_t *tokenizer)
{
    token_data_t result = {};

    u32 new_lines_seen = c_string_eat_whitespace(&tokenizer->data);
    tokenizer->line_count += new_lines_seen;
    if(tokenizer->data.count == 0)
    {
        result.type = TT_EOF;
        return(result);
    }

    result.string = tokenizer->data;
    result.string.count = 1;

    char character = tokenizer->data.data[0];
    c_string_advance_by(&tokenizer->data, 1);
    switch(character)
    {
        case ';':  {result.type = TT_Semicolon;        }break;
        case ':':  {result.type = TT_Colon;            }break;
        case '{':  {result.type = TT_OpeningBrace;     }break;
        case '}':  {result.type = TT_ClosingBrace;     }break;
        case '(':  {result.type = TT_OpeningParen;     }break;
        case ')':  {result.type = TT_ClosingParen;     }break;
        case '[':  {result.type = TT_OpenBracket;      }break;
        case ']':  {result.type = TT_ClosingBracket;   }break;
        case ',':  {result.type = TT_Comma;            }break;
        case '<':  {result.type = TT_OpenAngleBracket; }break;
        case '>':  {result.type = TT_CloseAngleBracket;}break;
        case '#':  {result.type = TT_HashTag;          }break;
        case '!':  {result.type = TT_Exclamation;      }break;
        case '=':  {result.type = TT_Equals;           }break;
        case '\\': {result.type = TT_BackSlash;        }break;
        case '\0': {result.type = TT_EOF;              }break;
        case '|':  {result.type = TT_Seperator;        }break;
        case '*':  
        {
            result.type = TT_Asterisk;
            if(tokenizer->data.count > 0 && tokenizer->data.data[0] == '/')
            {
                c_string_eat_whitespace(&tokenizer->data);
            }
        }break;
        case '"':  
        {
            c_string_advance_by(&result.string, 1);
            byte *at = tokenizer->data.data;

            while(tokenizer->data.count > 0 && 
                  (tokenizer->data.data[0] != '"'))
            {
                if((tokenizer->data.data[0] == '\\') && (tokenizer->data.data[1]))
                {
                    c_string_advance_by(&tokenizer->data, 2);
                }
                else
                {
                    c_string_advance_by(&tokenizer->data, 1);
                }
            }

            if(tokenizer->data.data[0] == '"')
            {
                c_string_advance_by(&tokenizer->data, 1);
            }
            u64 token_length = (tokenizer->data.data - at);

            result.type = TT_Identifier;
            result.string.count = (u32)token_length - 1;
        }break;
        default:
        {
            if(c_tokenizer_token_alphabetical((char)result.string.data[0]) || character == '_')
            {
                result.type = TT_Identifier;
                while(tokenizer->data.count > 0 && 
                      (c_tokenizer_token_alphabetical((char)tokenizer->data.data[0]) || 
                       c_tokenizer_token_numeric((char)tokenizer->data.data[0]) ||
                       tokenizer->data.data[0] == '_' || 
                       tokenizer->data.data[0] == '.'))
                {
                    c_string_advance_by(&tokenizer->data, 1);
                }

                result.string.count = (u32)(tokenizer->data.data - result.string.data);
            }
            else if(c_tokenizer_token_numeric((char)result.string.data[0]))
            {
                result.type = TT_Number;
                while(tokenizer->data.count > 0 &&
                      (c_tokenizer_token_numeric((char)tokenizer->data.data[0]) || 
                      tokenizer->data.data[0] == '.'))
                {
                    c_string_advance_by(&tokenizer->data, 1);
                }

                result.string.count = (u32)(tokenizer->data.data - result.string.data);
            }
            else
            {
                result.type = TT_Invalid;
                if(tokenizer->data.count > 0)
                {
                    c_string_advance_by(&tokenizer->data, 1);
                }
            }
        }break;
    }

    return(result);
}

token_data_t
c_tokenizer_peek_token(tokenizer_t *tokenizer, u32 times)
{
    token_data_t result = {};

    tokenizer_t fake_tokenizer = {};
    fake_tokenizer.data = tokenizer->data;
    for(u32 peek_index = 0;
        peek_index < times;
        ++peek_index)
    {
        result = c_tokenizer_get_next_token(&fake_tokenizer);
    }

    return(result);
}

string_t
c_tokenizer_eat_lines(memory_arena_t *concat_arena, tokenizer_t *tokenizer, u32 line_count)
{
    string_t result = {};

    for(u32 line_index = 0;
        line_index < line_count;
        ++line_index)
    {
        u32 end_line = c_string_find_first_char_from_left_on_line(tokenizer->data, '\n');
        if(end_line != INVALID_ID)
        {
            string_t line = tokenizer->data;
            line.count    = end_line;
            result = c_string_concat(concat_arena, line, result);
            c_string_advance_by(&tokenizer->data, end_line + 1);
        }
    }

    return(result);
}

true_inline void
c_tokenizer_set_bookmark(tokenizer_t *tokenizer, token_data_t token)
{
    tokenizer->read_bookmark         = tokenizer->data.data;
    tokenizer->bookmarked_read_count = tokenizer->data.count;
    tokenizer->bookmarked_token      = token;
    tokenizer->bookmarked_line_count = tokenizer->line_count;
}

true_inline token_data_t 
c_tokenizer_restore_bookmark(tokenizer_t *tokenizer)
{
    token_data_t result   = tokenizer->bookmarked_token;
    tokenizer->data.data  = tokenizer->read_bookmark;
    tokenizer->data.count = tokenizer->bookmarked_read_count;
    tokenizer->line_count = tokenizer->bookmarked_line_count;

    return(result);
}

