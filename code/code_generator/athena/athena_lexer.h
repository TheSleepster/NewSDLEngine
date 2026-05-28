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
    X(TOKEN_TYPE_EOF, "TOKEN_TYPE_EOF")

enum token_type_t
{
#define X(enum, string) enum,
    TOKEN_TYPE_LIST(X)
#undef X
};

struct lexer_token_t
{
    string_t data;
    u32      token_type;
};

struct lexer_bookmark_t
{
    u32                   read_count;
    byte                 *read_data;
    u32                   line_number;
    lexer_token_t         last_token;

    u32                   buffered_token_count;
    u32                   token_buffer_index;
};

struct lexer_token_stream_t
{
    string_t string;
    u32      line_number;

    byte    *start;
    u32      initial_length;

    // NOTE(Sleepster): The usage of this is optional 
    lexer_token_t *token_buffer;
    u32            buffered_token_count;
    u32            token_buffer_index;

    lexer_bookmark_t bookmarks[MAX_LEXER_BOOKMARKS]; 
    s32              bookmark_count;
};

struct lexer_t
{
    lexer_token_stream_t *current_stream;

    lexer_token_stream_t  token_streams[MAX_LEXER_BOOKMARKS];
    u32                   token_stream_count;
};

internal_api void lexer_push_token_stream(lexer_t *lexer, lexer_token_stream_t *new_stream);
internal_api void lexer_pop_token_stream(lexer_t *lexer);
internal_api lexer_token_stream_t init_token_stream_from_string(string_t string);
#endif
