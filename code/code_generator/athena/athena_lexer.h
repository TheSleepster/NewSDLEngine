#if !defined(ATHENA_LEXER_H)
/* ========================================================================
   $File: athena_lexer.h $
   $Date: Wed, 27 May 26: 05:11PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_LEXER_H

constexpr u32 MAX_LEXER_BOOKMARKS = 10;
#define fprint_token(token) token.data.count, token.data.data

struct lexer_token_stream_t;
// TODO(Sleepster): For this token list, when it comes to using keywords we are required in this case
// to define the unique token type here again instead of just the DEFAULT_KEYWORD_LIST. This is bad
//
// maybe make these bitflags? Just so we can do something like:
//
// token.token_type & TOKEN_TYPE_OPERATOR
//
// and 
//
// token.token_type & TOKEN_TYPE_BITSHIFT_LEFT
//
// Too show that this token is both an operator and a bitshift left.
#define TOKEN_TYPE_LIST(X) \
    X(TOKEN_TYPE_UNKNOWN, "TOKEN_TYPE_UNKNOWN") \
    X(TOKEN_TYPE_ARROW_OPERATOR, "TOKEN_TYPE_ARROW_OPERATOR") \
    X(TOKEN_TYPE_IDENT, "TOKEN_TYPE_IDENT") \
    X(TOKEN_TYPE_NUMBER, "TOKEN_TYPE_NUMBER") \
    X(TOKEN_TYPE_OPEN_PAREN, "TOKEN_TYPE_OPEN_PAREN") \
    X(TOKEN_TYPE_CLOSE_PAREN, "TOKEN_TYPE_CLOSE_PAREN") \
    X(TOKEN_TYPE_OPEN_BRACE, "TOKEN_TYPE_OPEN_BRACE") \
    X(TOKEN_TYPE_CLOSE_BRACE, "TOKEN_TYPE_CLOSE_BRACE") \
    X(TOKEN_TYPE_ASTERISK, "TOKEN_TYPE_ASTERISK") \
    X(TOKEN_TYPE_XOR, "TOKEN_TYPE_XOR") \
    X(TOKEN_TYPE_SEMICOLON, "TOKEN_TYPE_SEMICOLON") \
    X(TOKEN_TYPE_QUOTATION, "TOKEN_TYPE_QUOTATION") \
    X(TOKEN_TYPE_BANG, "TOKEN_TYPE_BANG") \
    X(TOKEN_TYPE_PERCENT, "TOKEN_TYPE_PERCENT") \
    X(TOKEN_TYPE_FORWARD_SLASH, "TOKEN_TYPE_FORWARD_SLASH") \
    X(TOKEN_TYPE_BACKSLASH, "TOKEN_TYPE_BACKSLASH") \
    X(TOKEN_TYPE_OPEN_BRACKET, "TOKEN_TYPE_OPEN_BRACKET") \
    X(TOKEN_TYPE_CLOSE_BRACKET, "TOKEN_TYPE_CLOSE_BRACKET") \
    X(TOKEN_TYPE_TILDE, "TOKEN_TYPE_TILDE") \
    X(TOKEN_TYPE_COMMA, "TOKEN_TYPE_COMMA") \
    X(TOKEN_TYPE_PERIOD, "TOKEN_TYPE_PERIOD") \
    X(TOKEN_TYPE_ELIPSES, "TOKEN_TYPE_ELIPSES") \
    X(TOKEN_TYPE_GREATER_THAN, "TOKEN_TYPE_GREATER_THAN") \
    X(TOKEN_TYPE_BITSHIFT_RIGHT, "TOKEN_TYPE_BITSHIFT_RIGHT") \
    X(TOKEN_TYPE_LESS_THAN, "TOKEN_TYPE_LESS_THAN") \
    X(TOKEN_TYPE_BITSHIFT_LEFT, "TOKEN_TYPE_BITSHIFT_LEFT") \
    X(TOKEN_TYPE_LESS_EQUAL, "TOKEN_TYPE_LESS_EQUAL") \
    X(TOKEN_TYPE_GREATER_EQUAL, "TOKEN_TYPE_GREATER_EQUAL") \
    X(TOKEN_TYPE_EQUALS, "TOKEN_TYPE_EQUALS") \
    X(TOKEN_TYPE_EQUAL_EQUAL, "TOKEN_TYPE_EQUAL_EQUAL") \
    X(TOKEN_TYPE_AND, "TOKEN_TYPE_AND") \
    X(TOKEN_TYPE_ANDAND, "TOKEN_TYPE_ANDAND") \
    X(TOKEN_TYPE_OR, "TOKEN_TYPE_OR") \
    X(TOKEN_TYPE_OROR, "TOKEN_TYPE_OROR") \
    X(TOKEN_TYPE_COLON, "TOKEN_TYPE_COLON") \
    X(TOKEN_TYPE_DOUBLE_COLON, "TOKEN_TYPE_DOUBLE_COLON") \
    X(TOKEN_TYPE_DASH, "TOKEN_TYPE_DASH") \
    X(TOKEN_TYPE_MINUS_MINUS, "TOKEN_TYPE_MINUS_MINUS") \
    X(TOKEN_TYPE_PLUS, "TOKEN_TYPE_PLUS") \
    X(TOKEN_TYPE_PLUSPLUS, "TOKEN_TYPE_PLUS_PLUS") \
    X(TOKEN_TYPE_POUND, "TOKEN_TYPE_POUND") \
    X(TOKEN_TYPE_PASTE_OPERATOR, "TOKEN_TYPE_PASTE_OPERATOR") \
    X(TOKEN_TYPE_TERNARY_IF, "TOKEN_TYPE_TERNARY_IF") \
    X(TOKEN_TYPE_LITERAL, "TOKEN_TYPE_LITERAL") \
    X(TOKEN_TYPE_STRUCT, "TOKEN_TYPE_STRUCT")    \
    X(TOKEN_TYPE_UNION, "TOKEN_TYPE_UNION")     \
    X(TOKEN_TYPE_ENUM, "TOKEN_TYPE_ENUM")      \
    X(TOKEN_TYPE_STATIC, "TOKEN_TYPE_STATIC")    \
    X(TOKEN_TYPE_EXTERN, "TOKEN_TYPE_EXTERN")    \
    X(TOKEN_TYPE_INLINE, "TOKEN_TYPE_INLINE")    \
    X(TOKEN_TYPE_VOLATILE, "TOKEN_TYPE_VOLATILE")  \
    X(TOKEN_TYPE_CONST, "TOKEN_TYPE_CONST")     \
    X(TOKEN_TYPE_AUTO, "TOKEN_TYPE_AUTO")      \
    X(TOKEN_TYPE_CLASS, "TOKEN_TYPE_CLASS")     \
    X(TOKEN_TYPE_PUBLIC, "TOKEN_TYPE_PUBLIC")    \
    X(TOKEN_TYPE_PRIVATE, "TOKEN_TYPE_PRIVATE")   \
    X(TOKEN_TYPE_PROTECTED, "TOKEN_TYPE_PROTECTED") \
    X(TOKEN_TYPE_TYPEDEF, "TOKEN_TYPE_TYPEDEF")   \
    X(TOKEN_TYPE_TEMPLATE, "TOKEN_TYPE_TEMPLATE") \
    X(TOKEN_TYPE_NAMESPACE, "TOKEN_TYPE_NAMESPACE") \
    X(TOKEN_TYPE_CONSTEXPR, "TOKEN_TYPE_CONSTEXPR") \
    X(TOKEN_TYPE_USING, "TOKEN_TYPE_USING") \
    X(TOKEN_TYPE_NULL, "TOKEN_TYPE_NULL") \
    X(TOKEN_TYPE_NULLPTR, "TOKEN_TYPE_NULLPTR") \
    X(TOKEN_TYPE_EOF, "TOKEN_TYPE_EOF")

enum token_type_t
{
#define X(enum, string) enum,
    TOKEN_TYPE_LIST(X)
#undef X
};

enum token_flags_t
{
    TOKEN_FLAG_NONE            = BIT(0),
    TOKEN_FLAG_BINARY_OPERATOR = BIT(1),
};

struct lexer_token_t
{
    string_t data;
    u32      token_type;
    u32      token_flags;
};

struct lexer_bookmark_t
{
    u32                   read_count;
    byte                 *read_data;
    u32                   line_number;
    u32                   token_buffer_index;
    u32                   next_token_stream;

    lexer_token_t         last_token;
    lexer_token_stream_t *stream;
};

struct lexer_token_stream_t 
{
    string_t         string;
    u32              line_number;

    byte            *start;
    u32              initial_length;

    // NOTE(Sleepster): The usage of this is optional 
    lexer_token_t   *token_buffer;
    u32              buffered_token_count;
    u32              token_buffer_index;

    lexer_bookmark_t bookmarks[MAX_LEXER_BOOKMARKS]; 
    s32              bookmark_count;

    lexer_token_t    last_token;
};

struct lexer_t
{
    lexer_token_stream_t *current_stream;
    lexer_token_stream_t *secondary_stream;

    lexer_token_stream_t  token_streams[MAX_LEXER_BOOKMARKS];
    s32                   next_token_stream;
};

internal_api void lexer_push_token_stream(lexer_t *lexer, lexer_token_stream_t *new_stream);
internal_api void lexer_pop_token_stream(lexer_t *lexer);
internal_api void lexer_init_token_stream_from_string(lexer_token_stream_t *stream, string_t string);
internal_api void lexer_init_token_stream_from_token_array(memory_arena_t *arena, lexer_token_stream_t *stream, lexer_token_t *tokens, u32 token_count);
#endif
