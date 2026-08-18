/* ========================================================================
   $File: athena_lexer.cpp $
   $Date: May 26 2026 02:38 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_lexer.h"
#include "athena_symbol_table.h"

const char *
lexer_token_type_to_string(lexer_token_t *token)
{
    switch(token->token_type)
    {
#define X(enum, string) case enum: {return((string));}break;
    TOKEN_TYPE_LIST(X)
#undef X
        default:
        {
            return("TOKEN_TYPE_UNKNOWN");
        }break;
    }
}

internal_api bool8
lexer_is_token_alphabetical(char A)
{
    bool8 result = (((A >= 'a') && (A <= 'z')) || 
                    ((A >= 'A') && (A <= 'Z')));
    return(result);
}

internal_api bool8
lexer_is_token_numeric(char A)
{
    bool8 result = ((A >= '0') && (A <= '9'));
    return(result);
}

internal_api true_inline bool8
lexer_stream_eat_repeating_character(lexer_token_stream_t *token_stream, lexer_token_t *token)
{
    bool8 result = false;

    string_t stream = token_stream->string;
    if(stream.data[0] == stream.data[1])
    {
        // NOTE(Sleepster): 
        // This is fine since all the repeating tokens like '&&' are just one off from their 
        // single variant. Example: '&' = 10, '&&' = 11
        token->data.count = 2;
        ++token->token_type;

        result = true;
    }

    return(result);
}

internal_api lexer_token_t
lexer_get_next_token_from_stream(lexer_token_stream_t *token_stream)
{
    lexer_token_t result = {};

    // NOTE(Sleepster): If the stream has a token buffer, we will always pull from the buffer instead... 
    if(!token_stream->token_buffer)
    {
        u32 new_line_count = c_string_eat_whitespace(&token_stream->string);
        token_stream->line_number += new_line_count;
        if(token_stream->string.count != 0)
        {
            result.data       = token_stream->string;
            result.data.count = 1;

            char character = token_stream->string.data[0];
            switch(character)
            {
                case ';':  {result.token_type = TOKEN_TYPE_SEMICOLON;     }break;
                case '{':  {result.token_type = TOKEN_TYPE_OPEN_BRACE;    }break;
                case '}':  {result.token_type = TOKEN_TYPE_CLOSE_BRACE;   }break;
                case '(':  {result.token_type = TOKEN_TYPE_OPEN_PAREN;    }break;
                case ')':  {result.token_type = TOKEN_TYPE_CLOSE_PAREN;   }break;
                case '[':  {result.token_type = TOKEN_TYPE_OPEN_BRACKET;  }break;
                case ']':  {result.token_type = TOKEN_TYPE_CLOSE_BRACKET; }break;
                case ',':  {result.token_type = TOKEN_TYPE_COMMA;         }break;
                case '!':  {result.token_type = TOKEN_TYPE_BANG;          }break;
                case '\0': {result.token_type = TOKEN_TYPE_EOF;           }break;
                case '\\': {result.token_type = TOKEN_TYPE_BACKSLASH;     }break;
                case '?':  {result.token_type = TOKEN_TYPE_TERNARY_IF;    }break;
                case '~':  {result.token_type = TOKEN_TYPE_TILDE;         }break;
                case '"':  
                {
                    u32 advance = 1;
                    while(token_stream->string.data[advance] != '"')
                    {
                        ++advance;
                    }

                    result.data.count = advance - 1;
                    result.data.data  = result.data.data + 1;
                    result.token_type = TOKEN_TYPE_LITERAL;
                }break;

                // NOTE(Sleepster): POTENTIALLY COMMENTS
                case '*': 
                {
                    result.token_type   = TOKEN_TYPE_ASTERISK;
                    result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    if(token_stream->string.count > 0 && token_stream->string.data[1] == '/')
                    {
                        result.data.count += c_string_get_whitespace_size(token_stream->string);
                    }               
                }break;
                case '/': 
                {
                    result.token_type = TOKEN_TYPE_FORWARD_SLASH;
                    if(token_stream->string.count > 0 && token_stream->string.data[1] == '/')
                    {
                        result.data.count += c_string_get_whitespace_size(token_stream->string);
                    }
                }break;
                
                // NOTE(Sleepster): REPEATABLE OPERATORS
                case '.':
                {
                    result.token_type = TOKEN_TYPE_PERIOD;
                    if(token_stream->string.data[1] == result.data.data[0] && 
                       token_stream->string.data[2] == result.data.data[0])
                    {
                        result.data.count = 3;
                        result.token_type = TOKEN_TYPE_ELIPSES;
                    }
                }break;
                case '<':
                {
                    result.token_type = TOKEN_TYPE_LESS_THAN;
                    if(lexer_stream_eat_repeating_character(token_stream, &result))
                    {
                        result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    }

                    if(result.token_type != TOKEN_TYPE_BITSHIFT_LEFT)
                    {
                        if(token_stream->string.data[1] == TOKEN_TYPE_EQUALS)
                        {
                            result.token_type   = TOKEN_TYPE_LESS_EQUAL;
                            result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                            result.data.count   = 2;
                        }
                    }
                }break;
                case '>':
                {
                    result.token_type = TOKEN_TYPE_GREATER_THAN;
                    if(lexer_stream_eat_repeating_character(token_stream, &result))
                    {
                        result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    }

                    if(result.token_type != TOKEN_TYPE_BITSHIFT_RIGHT)
                    {
                        if(token_stream->string.data[1] == TOKEN_TYPE_EQUALS)
                        {
                            result.token_type   = TOKEN_TYPE_GREATER_EQUAL;
                            result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                            result.data.count   = 2;
                        }
                    }
                }break;
                case '|':  
                {
                   result.token_type    = TOKEN_TYPE_OR;
                   result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                   lexer_stream_eat_repeating_character(token_stream, &result);
                }break;
                case '&':
                {
                    result.token_type   = TOKEN_TYPE_AND;
                    result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    if(lexer_stream_eat_repeating_character(token_stream, &result))
                    {
                        result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    }
                }break;
                case ':':  
                {
                    result.token_type = TOKEN_TYPE_COLON;
                    lexer_stream_eat_repeating_character(token_stream, &result);
                }break;
                case '=':  
                {
                    result.token_type   = TOKEN_TYPE_EQUALS;
                    result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    if(lexer_stream_eat_repeating_character(token_stream, &result))
                    {
                        result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    }
                }break;
                case '-':
                {
                    result.token_type   = TOKEN_TYPE_DASH;
                    result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    if(lexer_stream_eat_repeating_character(token_stream, &result))
                    {
                        result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    }

                    if(result.token_type == TOKEN_TYPE_DASH && token_stream->string.data[1] == '>')
                    {
                        result.token_type = TOKEN_TYPE_ARROW_OPERATOR;
                        result.data.count = 2;
                    }
                }break;
                case '+':
                {
                    result.token_type   = TOKEN_TYPE_PLUS;
                    result.token_flags |= TOKEN_FLAG_BINARY_OPERATOR;
                    lexer_stream_eat_repeating_character(token_stream, &result);
                }break;
                case '#':  
                {
                    result.token_type = TOKEN_TYPE_POUND;
                    lexer_stream_eat_repeating_character(token_stream, &result);
                }break;
                default:
                {
                    u32 next_char_index = 0;

                    if(lexer_is_token_alphabetical(result.data.data[0]) || character == '_')
                    {
                        result.token_type = TOKEN_TYPE_IDENT;
                        // TODO(Sleepster): DIRTY 
                        while(token_stream->string.count > 0                                          &&
                              next_char_index < token_stream->string.count                            &&
                             (lexer_is_token_alphabetical(token_stream->string.data[next_char_index]) || 
                              lexer_is_token_numeric(token_stream->string.data[next_char_index])      ||
                              token_stream->string.data[next_char_index] == '_'                       || 
                              token_stream->string.data[next_char_index] == '.'))
                        {
                            ++next_char_index;
                        }
                        result.data.count = next_char_index;

                        // NOTE(Sleepster): If we find an identifier, we need to check if it's a keyword.
                        language_keyword_t *keyword = get_keyword_from_identifier(result.data);
                        if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
                        {
                            switch(keyword->keyword_id)
                            {
                                // NOTE(Sleepster): X MACRO to automate this. 
#define X(string, enum, keyword_token_type) \
                                case enum: {result.token_type = keyword_token_type;}break;

                                DEFAULT_KEYWORD_LIST(X);
#undef X
                                default:
                                {
                                    result.token_type = TOKEN_TYPE_IDENT;
                                }
                            }
                        }
                    }
                    else if(lexer_is_token_numeric(result.data.data[0]))
                    {
                        result.token_type = TOKEN_TYPE_NUMBER;
                        while(token_stream->string.count > 0 &&
                              next_char_index < token_stream->string.count &&
                              (lexer_is_token_numeric(result.data.data[next_char_index]) || 
                               token_stream->string.data[next_char_index] == '.'))
                        {
                            ++next_char_index;
                        }
                        result.data.count = next_char_index;
                    }
                    else
                    {
                        result.token_type = TOKEN_TYPE_UNKNOWN;
                    }break;
                }break;
            }

            // TODO(Sleepster): This is dumb but I'm lazy. 
            if(result.token_type == TOKEN_TYPE_LITERAL)
            {
                c_string_advance_by(&token_stream->string, result.data.count + 2);
            }
            else
            {
                c_string_advance_by(&token_stream->string, result.data.count);
            }
        }
        else
        {
            result.token_type = TOKEN_TYPE_EOF;
            result.data       = {};
        }
    }
    else
    {
        // NOTE(Sleepster): In the event that we've read all the tokens from the buffer, we'll simply return an EOF 
        if(token_stream->token_buffer_index < token_stream->buffered_token_count)
        {
            result = token_stream->token_buffer[token_stream->token_buffer_index++];
        }
        else
        {
            result = {.data = {}, .token_type = TOKEN_TYPE_EOF};
        }
    }
    
    token_stream->last_token = result;

    return(result);
}

internal_api lexer_token_t
lexer_get_next_token(lexer_t *lexer)
{
    lexer_token_t result  = {};
    result = lexer_get_next_token_from_stream(lexer->current_stream);
    
    return(result);
}

internal_api lexer_token_t
lexer_peek_token_from_stream(lexer_t *lexer, lexer_token_stream_t *stream, u32 tokens_to_peek_ahead = 1)
{
    lexer_token_t result = {};

    lexer_token_stream_t fake_stream = *stream;
    lexer_push_token_stream(lexer, &fake_stream);
    for(u32 peek_index = 0;
        peek_index < tokens_to_peek_ahead;
        ++peek_index)
    {
        result = lexer_get_next_token(lexer);
    }
    lexer_pop_token_stream(lexer);

    return(result);
}

internal_api lexer_token_t
lexer_peek_token(lexer_t *lexer, u32 tokens_to_peek_ahead = 1)
{
    lexer_token_t token = {};
    token = lexer_peek_token_from_stream(lexer, lexer->current_stream, tokens_to_peek_ahead);

    return(token);
}

internal_api true_inline s32
lexer_push_bookmark(lexer_t *lexer, lexer_token_t token)
{
    lexer_bookmark_t *bookmark = lexer->current_stream->bookmarks + ++lexer->current_stream->bookmark_count;
    Expect(lexer->current_stream->bookmark_count + 1 <= (s32)MAX_LEXER_BOOKMARKS, 
           "The amount of lexer bookmarks is limitted to '%u'... You have gone over that", 
           MAX_LEXER_BOOKMARKS);

    bookmark->read_data          = lexer->current_stream->string.data;
    bookmark->read_count         = lexer->current_stream->string.count;
    bookmark->line_number        = lexer->current_stream->line_number;
    bookmark->last_token         = token;
    bookmark->stream             = lexer->current_stream;
    bookmark->token_buffer_index = lexer->current_stream->token_buffer_index;
    bookmark->next_token_stream  = lexer->next_token_stream;

    return(lexer->current_stream->bookmark_count);
}

internal_api true_inline lexer_token_t 
lexer_pop_bookmark(lexer_t *lexer)
{
    lexer_bookmark_t *bookmark = lexer->current_stream->bookmarks + lexer->current_stream->bookmark_count--;
    Expect(lexer->current_stream->bookmark_count >= 0, 
           "lexer->bookmark_count must be between 0 and 10, somehow you have a number LESS than 0...\n");

    lexer_token_t result = bookmark->last_token;
    lexer->current_stream                     = bookmark->stream;
    lexer->current_stream->string.data        = bookmark->read_data;
    lexer->current_stream->string.count       = bookmark->read_count;
    lexer->current_stream->line_number        = bookmark->line_number;
    lexer->current_stream->token_buffer_index = bookmark->token_buffer_index;
    lexer->next_token_stream                  = bookmark->next_token_stream;

    return(result);
}

internal_api void 
lexer_init_token_stream_from_string(lexer_token_stream_t *stream, string_t string)
{
    stream->string         = string;
    stream->start          = string.data;
    stream->initial_length = string.count;
}

internal_api void
lexer_init_token_stream_from_token_array(memory_arena_t *arena, lexer_token_stream_t *stream, lexer_token_t *tokens, u32 token_count)
{
    ZeroStruct(*stream);

    stream->token_buffer         = c_arena_push_array(arena, lexer_token_t, token_count);
    stream->buffered_token_count = token_count;

    memcpy(stream->token_buffer, tokens, sizeof(lexer_token_t) * token_count);
}

internal_api void
lexer_push_token_stream(lexer_t *lexer, lexer_token_stream_t *new_stream)
{
    lexer_token_stream_t *stream = lexer->token_streams + ++lexer->next_token_stream;

    *stream = *new_stream;

    lexer->secondary_stream = lexer->current_stream;
    lexer->current_stream   = stream;
}

internal_api void
lexer_pop_token_stream(lexer_t *lexer)
{
    s32 next_stream_index = lexer->next_token_stream - 1;
    if(next_stream_index >= 0)
    {
        lexer_token_stream_t *stream  = lexer->token_streams + --lexer->next_token_stream;

        lexer->secondary_stream = lexer->current_stream;
        lexer->current_stream   = stream;
    }
}

internal_api string_t
lexer_eat_lines(memory_arena_t *concat_arena, lexer_t *lexer, u32 line_count)
{
    string_t result = {};
    for(u32 line_index = 0;
        line_index < line_count;
        ++line_index)
    {
        u32 end_line = c_string_find_first_char_from_left_on_line(lexer->current_stream->string, '\n');
        if(end_line != INVALID_ID)
        {
            string_t line = lexer->current_stream->string;
            line.count    = end_line;
            result        = c_string_concat(concat_arena, line, result);
            c_string_advance_by(&lexer->current_stream->string, end_line + 1);

            ++lexer->current_stream->line_number;
        }
    }

    return(result);
}

internal_api void
lexer_reset_token_stream(lexer_token_stream_t *stream)
{
    stream->string.data  = stream->start;
    stream->string.count = stream->initial_length;
    stream->line_number  = 0;
}

internal_api lexer_token_stream_t 
lexer_copy_token_stream(memory_arena_t *arena, lexer_token_stream_t *stream)
{
    lexer_token_stream_t result = {};
    memcpy(&result, stream, sizeof(lexer_token_stream_t));
    if(stream->buffered_token_count > 0)
    {
        result.token_buffer         = c_arena_push_array(arena, lexer_token_t, stream->buffered_token_count);
        result.buffered_token_count = stream->buffered_token_count;
        result.token_buffer_index   = 0;
        for(u32 index = 0;
            index < stream->buffered_token_count;
            ++index)
        {
            lexer_token_t *source      = stream->token_buffer + index;
            lexer_token_t *destination = result.token_buffer  + index;

            *destination = *source;
        }
    }

    return(result);
}

internal_api void 
lexer_create(lexer_t *lexer, string_t string_data)
{
    lexer_init_token_stream_from_string(&lexer->token_streams[0], string_data);
    lexer->current_stream = lexer->token_streams;
}

