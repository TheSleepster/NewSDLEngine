#if !defined(C_TOKENIZER_H)
/* ========================================================================
   $File: c_tokenizer.h $
   $Date: January 31 2026 11:33 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_TOKENIZER_H
#include <c_types.h>
#include <c_base.h>
#include <c_string.h>

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
    TT_Equals,
    TT_Dash,
    TT_BackSlash, // we only care about these if we're inside a macro
    TT_Seperator,
    TT_Number,
    TT_EOF,

    TT_Error,
    TT_Identifier,
    TT_Count
}preprocessor_token_type_t;

typedef struct token_data
{
    preprocessor_token_type_t type;
    string_t                  string;
}token_data_t;

typedef struct tokenizer
{
    string_t     data;
    u32          line_count;

    u32          bookmarked_read_count;
    byte        *read_bookmark;
    token_data_t bookmarked_token;
}tokenizer_t;

// NOTE(Sleepster): This will eat portions of the string and give you back the eaten bits 
token_data_t c_tokenizer_get_next_token(tokenizer_t *tokenizer);
// NOTE(Sleepster): This on the other hand, will look ahead, but not consume
token_data_t c_tokenizer_peek_token(tokenizer_t *tokenizer, u32 times = 1);
string_t     c_tokenizer_eat_lines(memory_arena_t *concat_arena, tokenizer_t *tokenizer, u32 line_count);

bool8 c_tokenizer_token_numeric(char A);
bool8 c_tokenizer_token_alphabetical(char A);

true_inline void         c_tokenizer_set_bookmark(tokenizer_t *tokenizer, token_data_t token);
true_inline token_data_t c_tokenizer_restore_bookmark(tokenizer_t *tokenizer);

#define fprint_token(token) (token).string.count, C_STR((token).string)

#endif // C_TOKENIZER_H

