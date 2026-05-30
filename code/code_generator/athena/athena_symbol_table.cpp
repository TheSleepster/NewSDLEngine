/* ========================================================================
   $File: athena_symbol_table.cpp $
   $Date: May 26 2026 04:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_symbol_table.h"

internal_api void
symbol_table_init(void)
{
    c_hash_table_init(&g_symbol_table.macro_table, 2048);

    g_symbol_table.keywords = c_dynarray_create(language_keyword_t);
    string_t default_keyword_strings[] = {
#define X(string, enum, token_type) STR(string),
        DEFAULT_KEYWORD_LIST(X)
#undef X
    };

    keywords_t default_keyword_enums[] = {
#define X(string, enum, token_type) enum,
        DEFAULT_KEYWORD_LIST(X)
#undef X
    };
    
    for(u32 index = 0;
        index < ArrayCount(default_keyword_strings);
        ++index)
    {
        language_keyword_t keyword = {};
        keyword.identifier         = default_keyword_strings[index];
        keyword.keyword_id         = default_keyword_enums[index];

        c_dynarray_push(g_symbol_table.keywords, keyword);
    }
}

internal_api language_keyword_t*
symbol_table_get_keyword(string_t string)
{
    language_keyword_t *result = null;
    c_dynarray_for(g_symbol_table.keywords, keyword_index)
    {
        language_keyword_t *keyword = g_symbol_table.keywords + keyword_index;
        if(c_string_compare(keyword->identifier, string))
        {
            result = keyword;
            break;
        }
    }

    if(result == null)
    {
        // NOTE(Sleepster): Should be invalid 
        result = g_symbol_table.keywords;
        Expect(result->keyword_id == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}

internal_api lexer_token_stream_t 
symbol_table_substitute_macro_arguments(lexer_t *lexer, lexer_token_t last_token, macro_info_t *macro_info)
{
    lexer_token_stream_t result = {};

    // NOTE(Sleepster): This right now is completely wrong.
    // Firstly, this function should just not be getting called when it is.
    // Secondly, this function should operate on TWO streams at the same time, eating from both and replacing the saved macro's arguments
    // with the corresponding argument from the invoked macro. like so:
    // #define item(name) item_##name
    //
    // In the event of this invocation:
    // item(gloves);
    //
    // We should see:
    // item_gloves
    
    lexer_token_t next_expansion_token = lexer_peek_token(lexer);
    // NOTE(Sleepster): If the macro takes arguments... 
    if(next_expansion_token.token_type == TOKEN_TYPE_OPEN_PAREN && last_token.data.data[last_token.data.count] != ' ')
    {
        Expect(next_expansion_token.token_type == TOKEN_TYPE_OPEN_PAREN,
               "When attempting to substitute arguments of a macro, we failed to find the invocation (which is an '(') and instead were left with: '%.*s'...\n",
               fprint_token(next_expansion_token));
        next_expansion_token = lexer_get_next_token(lexer);

        result = lexer_copy_token_stream(&macro_info->expansion_token_stream);

        // NOTE(Sleepster): Go through each of the arguments and find every token that matches that argument, replace this
        // with the actual argument from the macro.
    
        u32 macro_argument_index = 0;
        while(next_expansion_token.token_type != TOKEN_TYPE_CLOSE_PAREN)
        {
            // NOTE(Sleepster): Get our argument string
            next_expansion_token = lexer_get_next_token(lexer);
            Expect(next_expansion_token.token_type != TOKEN_TYPE_COMMA,
                   "Expected to find another argument when expanding this macro, instead found: '%.*s'...\n",
                   fprint_token(next_expansion_token));

            string_t macro_argument = macro_info->arguments[macro_argument_index];
            for(u32 token_index = 0;
                token_index < macro_info->expansion_token_stream.buffered_token_count;
                ++token_index)
            {
                lexer_token_t *macro_token = result.token_buffer + token_index;
                if(c_string_compare(macro_token->data, macro_argument))
                {
                    *macro_token = next_expansion_token;
                    ++macro_argument_index;

                    break;
                }
            }
            lexer_token_t check_token = lexer_peek_token(lexer, 1);
            Expect(check_token.token_type == TOKEN_TYPE_COMMA || check_token.token_type == TOKEN_TYPE_CLOSE_PAREN,
                   "Invalid token when parsing the end of this macro invocation's argument string... token found was: '%.*s'...\n",
                   fprint_token(check_token));
            lexer_get_next_token(lexer);
            if(check_token.token_type == TOKEN_TYPE_CLOSE_PAREN)
            {
                break;
            }
        }
    }
    // NOTE(Sleepster): If it does not... 
    else
    {
        result = macro_info->expansion_token_stream;
    }

    return(result);
}

internal_api lexer_token_t
symbol_table_get_next_lexer_token(lexer_t *lexer)
{
    lexer_token_t result;
    result = lexer_get_next_token(lexer);
    if(result.token_type == TOKEN_TYPE_EOF)
    {
        lexer_pop_token_stream(lexer, false);
        result = lexer_get_next_token(lexer);
    }

    macro_info_t *macro = null;
    TicketMutexScope(&g_symbol_table.macro_table_mutex)
    {
        macro = c_hash_table_get_value_ptr(&g_symbol_table.macro_table, result.data);
        if(macro->is_set)
        {
            lexer_token_stream_t macro_stream = symbol_table_substitute_macro_arguments(lexer, result, macro);
            lexer_push_token_stream(lexer, &macro_stream);

            result = lexer_get_next_token(lexer);
            if(lexer->current_stream->string.count == 0)
            {
                lexer_pop_token_stream(lexer, false);
            }
        }
    }
    
    return(result);
}
