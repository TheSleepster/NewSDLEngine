#if !defined(ATHENA_SYMBOL_TABLE_H)
/* ========================================================================
   $File: athena_symbol_table.h $
   $Date: May 30 2026 08:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_SYMBOL_TABLE_H

#define DEFAULT_KEYWORD_LIST(X)                                   \
    X("Invalid",   TOKEN_KEYWORD_INVALID,   TOKEN_TYPE_UNKNOWN)   \
    X("struct",    TOKEN_KEYWORD_STRUCT,    TOKEN_TYPE_STRUCT)    \
    X("union",     TOKEN_KEYWORD_UNION,     TOKEN_TYPE_UNION)     \
    X("enum",      TOKEN_KEYWORD_ENUM,      TOKEN_TYPE_ENUM)      \
    X("static",    TOKEN_KEYWORD_STATIC,    TOKEN_TYPE_STATIC)    \
    X("extern",    TOKEN_KEYWORD_EXTERN,    TOKEN_TYPE_EXTERN)    \
    X("inline",    TOKEN_KEYWORD_INLINE,    TOKEN_TYPE_INLINE)    \
    X("volatile",  TOKEN_KEYWORD_VOLATILE,  TOKEN_TYPE_VOLATILE)  \
    X("const",     TOKEN_KEYWORD_CONST,     TOKEN_TYPE_CONST)     \
    X("constexpr", TOKEN_KEYWORD_CONSTEXPR, TOKEN_TYPE_CONSTEXPR) \
    X("auto",      TOKEN_KEYWORD_AUTO,      TOKEN_TYPE_AUTO)      \
    X("class",     TOKEN_KEYWORD_CLASS,     TOKEN_TYPE_CLASS)     \
    X("public",    TOKEN_KEYWORD_PUBLIC,    TOKEN_TYPE_PUBLIC)    \
    X("private",   TOKEN_KEYWORD_PRIVATE,   TOKEN_TYPE_PRIVATE)   \
    X("protected", TOKEN_KEYWORD_PROTECTED, TOKEN_TYPE_PROTECTED) \
    X("typedef",   TOKEN_KEYWORD_TYPEDEF,   TOKEN_TYPE_TYPEDEF)   \
    X("template",  TOKEN_KEYWORD_TEMPLATE,  TOKEN_TYPE_TEMPLATE)  \
    X("namespace", TOKEN_KEYWORD_NAMESPACE, TOKEN_TYPE_NAMESPACE) \
    X("using",     TOKEN_KEYWORD_USING,     TOKEN_TYPE_USING)

enum keywords_t
{
#define X(string, enum, token_type) enum,
    DEFAULT_KEYWORD_LIST(X)
#undef X
};

#define DEFAULT_PRIMITIVE_TYPES_LIST(X) \
    X("unsigned int") \
    X("unsigned char") \
    X("short") \
    X("long") \
    X("int") \
    X("char") \
    X("float") \
    X("double") \
    X("void") \
    X("bool") \
    X("int8_t") \
    X("int16_t")  \
    X("int32_t")  \
    X("int64_t")  \
    X("uint8_t")  \
    X("uint16_t") \
    X("uint32_t") \
    X("uint64_t") \
    X("size_t") \

struct language_keyword_t
{
    string_t identifier;
    u32      keyword_id;
};

struct macro_info_t
{
    bool8                is_set;

    u64                  name_hash;
    string_t             name;
    string_t             expansion_string;
    lexer_token_stream_t expansion_token_stream;

    string_t            *arguments;
    u32                  argument_count;
};

#define CODE_TYPE_METATYPE_LIST(X) \
    X(CODE_TYPE_UNDEFINED, "CODE_TYPE_UNDEFINED") \
    X(CODE_TYPE_PRIMITIVE, "CODE_TYPE_PRIMITIVE") \
    X(CODE_TYPE_STRUCTURE, "CODE_TYPE_STRUCTURE") \
    X(CODE_TYPE_ENUM,      "CODE_TYPE_ENUM") \
    X(CODE_TYPE_LAMBDA,    "CODE_TYPE_LAMBDA") \

enum code_type_metatype_t
{
#define X(enum, string) enum,
    CODE_TYPE_METATYPE_LIST(X)
#undef X
};

struct code_type_t
{
    bool8        is_registered;
    bool8        type_inferred;

    string_t     identifier;
    u64          ID;
    u64          scope_ID;
    u64          alias_of = INVALID_ID;

    u32          code_metatype;
    AST_node_t  *type_info_AST;
    code_type_t *next_overload;
};

// TODO(Sleepster): We may want to make the macro_data, the type table, and the structure/enum data a GLOBAL table seperate from the symbol_table
struct symbol_table_t
{
    // NOTE(Sleepster): Maps macro declarations to their values... 
    ticket_mutex_t                       macro_table_mutex;
    hash_table_t<macro_info_t>           macro_table;

    // NOTE(Sleepster): "sparse" array
    ticket_mutex_t                       type_table_mutex;
    hash_table_t<code_type_t>            type_table;

    // NOTE(Sleepster): In case you want to add more keywords besides those added, this is
    // a get_keyword(token.string)dynamic array.
    DynArray_t(language_keyword_t)       keywords;
    DynArray_t(code_type_t*)             primitives;

    ticket_mutex_t                       constant_table_mutex;
    hash_table_t<AST_expression_value_t> constants_table;

    ticket_mutex_t                       type_table_indices_mutex;
    DynArray_t(u32)                      valid_type_table_indices;

    ticket_mutex_t                       AST_structures_mutex;
    DynArray_t(AST_node_t*)              structures;

    ticket_mutex_t                       AST_enums_mutex;
    DynArray_t(AST_node_t*)              enums;

    ticket_mutex_t                       AST_lambdas_mutex;
    DynArray_t(AST_node_t*)              lambdas;
};

global_variable symbol_table_t g_symbol_table;

internal_api void                 symbol_table_init(void);
internal_api language_keyword_t  *symbol_table_get_keyword(string_t string);
internal_api lexer_token_stream_t symbol_table_substitute_macro_arguments(lexer_t *lexer, lexer_token_t last_token, macro_info_t *macro_info);
internal_api lexer_token_t        symbol_table_get_next_lexer_token(lexer_t *lexer);

#endif // ATHENA_SYMBOL_TABLE_H

