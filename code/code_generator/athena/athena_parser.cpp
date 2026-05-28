/* ========================================================================
   $File: athena_parser.cpp $
   $Date: May 26 2026 04:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define DEFAULT_KEYWORD_LIST(X)             \
    X("Invalid",   TOKEN_KEYWORD_INVALID)   \
    X("struct",    TOKEN_KEYWORD_STRUCT)    \
    X("union",     TOKEN_KEYWORD_UNION)     \
    X("enum",      TOKEN_KEYWORD_ENUM)      \
    X("static",    TOKEN_KEYWORD_STATIC)    \
    X("extern",    TOKEN_KEYWORD_EXTERN)    \
    X("inline",    TOKEN_KEYWORD_INLINE)    \
    X("volatile",  TOKEN_KEYWORD_VOLATILE)  \
    X("const",     TOKEN_KEYWORD_CONST)     \
    X("auto",      TOKEN_KEYWORD_AUTO)      \
    X("class",     TOKEN_KEYWORD_CLASS)     \
    X("public",    TOKEN_KEYWORD_PUBLIC)    \
    X("private",   TOKEN_KEYWORD_PRIVATE)   \
    X("protected", TOKEN_KEYWORD_PROTECTED) \
    X("typedef",   TOKEN_KEYWORD_TYPEDEF)   \
    X("using",     TOKEN_KEYWORD_USING)

enum keywords_t
{
#define X(string, enum) enum,
    DEFAULT_KEYWORD_LIST(X)
#undef X
};

struct language_keyword_t
{
    string_t identifier;
    u32      keyword_id;
};

// TODO(Sleepster): We may want to make the macro_data, the type table, and the structure/enum data a GLOBAL table seperate from the parser
struct parser_state_t
{
    // NOTE(Sleepster): Maps macro declarations to their values... 
    ticket_mutex_t                 macro_table_mutex;
    HashTable_t(macro_info_t)      macro_table;

    // NOTE(Sleepster): In case you want to add more keywords besides those added, this is
    // a get_keyword(token.string)dynamic array.
    DynArray_t(language_keyword_t) keywords;
};

internal_api void
parser_init(parser_state_t *parser)
{
    c_hash_table_init(&parser->macro_table, 2048);

    parser->keywords = c_dynarray_create(language_keyword_t);
    string_t default_keyword_strings[] = {
#define X(string, enum) STR(string),
        DEFAULT_KEYWORD_LIST(X)
#undef X
    };

    keywords_t default_keyword_enums[] = {
#define X(string, enum) enum,
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

        c_dynarray_push(parser->keywords, keyword);
    }
}


internal_api language_keyword_t*
parser_get_keyword(parser_state_t *parser, lexer_token_t token)
{
    language_keyword_t *result = null;
    c_dynarray_for(parser->keywords, keyword_index)
    {
        language_keyword_t *keyword = parser->keywords + keyword_index;
        if(c_string_compare(keyword->identifier, token.data))
        {
            result = keyword;
            break;
        }
    }

    if(result == null)
    {
        // NOTE(Sleepster): Should be invalid 
        result = parser->keywords;
        Expect(result->keyword_id == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}

internal_api lexer_token_stream_t 
parser_substitute_macro_arguments(lexer_t *lexer, lexer_token_t last_token, macro_info_t *macro_info)
{
    lexer_token_stream_t result = {};
    
    lexer_push_bookmark(lexer, last_token);
    lexer_token_t next_token = lexer_get_next_token(lexer);

    Expect(next_token.token_type == TOKEN_TYPE_OPEN_PAREN,
           "When attempting to substitute arguments of a macro, we failed to find the invocation (which is an '(') and instead were left with: '%.*s'...\n",
           fprint_token(next_token));

    result = lexer_copy_token_stream(&macro_info->expansion_token_stream);
    
    u32 macro_argument_index = 0;
    while(next_token.token_type != TOKEN_TYPE_CLOSE_PAREN)
    {
        // NOTE(Sleepster): Get our argument string
        next_token = lexer_get_next_token(lexer);

        string_t macro_argument = macro_info->arguments[macro_argument_index];
        for(u32 token_index = 0;
            token_index < macro_info->expansion_token_stream.buffered_token_count;
            ++token_index)
        {
            lexer_token_t *macro_token = result.token_buffer + token_index;
            if(c_string_compare(macro_token->data, macro_argument))
            {
                *macro_token = next_token;
                ++macro_argument_index;

                break;
            }
        }
        lexer_token_t check_token = lexer_peek_token(lexer, 1);
        Expect(check_token.token_type == TOKEN_TYPE_COMMA || check_token.token_type == TOKEN_TYPE_CLOSE_PAREN,
               "Invalid token when parsing the end of this macro invocation's argument string... token found was: '%.*s'...\n",
               fprint_token(check_token));
    }
    lexer_pop_bookmark(lexer);

    return(result);
}

internal_api lexer_token_t
parser_get_next_lexer_token(parser_state_t *parser, lexer_t *lexer)
{
    lexer_token_t result;
    result = lexer_get_next_token(lexer);
    if(result.token_type == TOKEN_TYPE_EOF)
    {
        lexer_pop_token_stream(lexer);
    }

    macro_info_t *macro = null;
    TicketMutexScope(&parser->macro_table_mutex)
    {
        macro = c_hash_table_get_value_ptr(&parser->macro_table, result.data);
        if(macro)
        {
            lexer_token_stream_t macro_stream = parser_substitute_macro_arguments(lexer, result, macro);
            lexer_push_token_stream(lexer, &macro_stream);

            result = lexer_get_next_token(lexer);
        }
    }
    
    return(result);
}
