#if !defined(ATHENA_SYMBOL_TABLE_H)
/* ========================================================================
   $File: athena_symbol_table.h $
   $Date: May 30 2026 08:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_SYMBOL_TABLE_H

constexpr u32 MAX_PEEK_AHEAD_TOKENS = 32;

#define DEFAULT_KEYWORD_LIST(X)                                    \
    X("Invalid",   TOKEN_KEYWORD_INVALID,   TOKEN_TYPE_UNKNOWN)    \
    X("struct",    TOKEN_KEYWORD_STRUCT,    TOKEN_TYPE_STRUCT)     \
    X("union",     TOKEN_KEYWORD_UNION,     TOKEN_TYPE_UNION)      \
    X("enum",      TOKEN_KEYWORD_ENUM,      TOKEN_TYPE_ENUM)       \
    X("static",    TOKEN_KEYWORD_STATIC,    TOKEN_TYPE_STATIC)     \
    X("extern",    TOKEN_KEYWORD_EXTERN,    TOKEN_TYPE_EXTERN)     \
    X("inline",    TOKEN_KEYWORD_INLINE,    TOKEN_TYPE_INLINE)     \
    X("volatile",  TOKEN_KEYWORD_VOLATILE,  TOKEN_TYPE_VOLATILE)   \
    X("const",     TOKEN_KEYWORD_CONST,     TOKEN_TYPE_CONST)      \
    X("constexpr", TOKEN_KEYWORD_CONSTEXPR, TOKEN_TYPE_CONSTEXPR)  \
    X("auto",      TOKEN_KEYWORD_AUTO,      TOKEN_TYPE_AUTO)       \
    X("class",     TOKEN_KEYWORD_CLASS,     TOKEN_TYPE_CLASS)      \
    X("public",    TOKEN_KEYWORD_PUBLIC,    TOKEN_TYPE_PUBLIC)     \
    X("private",   TOKEN_KEYWORD_PRIVATE,   TOKEN_TYPE_PRIVATE)    \
    X("protected", TOKEN_KEYWORD_PROTECTED, TOKEN_TYPE_PROTECTED)  \
    X("typedef",   TOKEN_KEYWORD_TYPEDEF,   TOKEN_TYPE_TYPEDEF)    \
    X("template",  TOKEN_KEYWORD_TEMPLATE,  TOKEN_TYPE_TEMPLATE)   \
    X("namespace", TOKEN_KEYWORD_NAMESPACE, TOKEN_TYPE_NAMESPACE)  \
    X("using",     TOKEN_KEYWORD_USING,     TOKEN_TYPE_USING)      \
    X("NULL",      TOKEN_KEYWORD_NULL,      TOKEN_TYPE_NULL)       \
    X("nullptr",   TOKEN_KEYWORD_NULLPTR,   TOKEN_TYPE_NULLPTR)

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
    X("size_t")

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
    string_t     owner_file;
    u64          ID;
    code_type_t *alias_of = null;
    AST_node_t  *type_data;

    u32          code_metatype;
};

struct language_info_t
{
    dynarray_t<language_keyword_t> keywords;
    dynarray_t<code_type_t>        language_primitive_types;
};

struct declaration_context_t
{
    string_t                   lexical_scope;
    u64                        context_ID;

    hash_table_t<code_type_t*> local_types;
    hash_table_t<AST_node_t*>  code_decls;
    hash_table_t<AST_node_t*>  enum_symbols;

    declaration_context_t     *parent_scope;
};

struct code_attribute_t
{
    string_t name;
    bool8    is_template;
    struct {
        AST_node_t *arguments;
        u32         argument_count;
    }template_data;
};

// TODO(Sleepster): With subarenas, all parsers can just be children of a parent arena and thus share the same lifetime as the parent.
struct parser_t
{
    memory_arena_t                     arena;
    memory_arena_t                     temp_allocator;
    string_t                           filename;

    dynarray_t<code_attribute_t>       current_attribute_list;
    dynarray_t<declaration_context_t*> decl_context_stack;
    dynarray_t<declaration_context_t>  recorded_decl_contexts;
    declaration_context_t             *active_decl_context;

    lexer_t                            lexer;
    lexer_token_t                      peek_ahead_buffer[MAX_PEEK_AHEAD_TOKENS];
    u32                                token_buffer_head;
    u32                                buffered_token_count;

    hash_table_t<macro_info_t>         macro_table;
};

struct symbol_table_t 
{
    bool8                             is_initialized;

    parser_t                         *file_parsers;
    u32                               file_count;
    volatile u32                      next_parser_index;
        
    hash_table_t<macro_info_t>        defined_global_macro_table;
    hash_table_t<code_type_t*>        type_table;
    dynarray_t<declaration_context_t> declaration_contexts;
};


global_variable language_info_t g_language_info;
global_variable symbol_table_t  g_symbol_table;

internal_api language_keyword_t *get_keyword_from_identifier(string_t identifier);
internal_api void                symbol_table_init(string_t filepath, bool8 recursive);

internal_api lexer_token_t parser_get_next_lexer_token(parser_t *parser);
internal_api lexer_token_t parser_peek_next_lexer_token(parser_t *parser, u32 peek_amount = 1);
#endif // ATHENA_SYMBOL_TABLE_H

