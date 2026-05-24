/* ========================================================================
   $File: athena.cpp $
   $Date: May 21 2026 07:46 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_synchronization.h>

#define HASH_TABLE_IMPLEMENTATION
#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION 

#include <c_hash_table.h>
#include <c_file_api.h>
#include <c_string.h>
#include <c_tokenizer.h>
#include <c_program_flag_handler.h>
#include <c_dynarray.h>

#include <p_platform_data.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_global_context.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_tokenizer.cpp>


// ATHENA BUILD FILES

//

// Say we find:
//
// internal_api inline void function(int arg0, float32 arg1);
//
// We would skip internal_api (which is a macro for static) and inline since they are not important for knowing
// what kind of declaration this is. But, when we find the token sequence:
//
// 'void' (type) -> 'function' (identifier) -> '(' (open_paren)
//
// We would begin to parse this as a function, then turn each of these tokens into AST_nodes like so:
//
// 'void' (type) -> 'function' -> (identifier) '(' -> (open_paren) -> 'int' (type) -> 'arg0' (identifier) -> (skip the comma) -> 'float32' (type) -> 'arg1' (identifier) -> ')' (close_brace) -> ';' (end_statement)
//
// which would then be added to the declaration, building it like so:
//
// decl.type  = CODE_DECLARATION_TYPE_STRUCTURE
// decl.name  = 'function'
//
// then just chain each of these AST_node_t onto the  declaration for processing later...
// decl.first = ...;
//
// first_node -> AST_node_t {
// type ->  
//
// };
//
//
//
// If we instead find:
// 
// struct lemons_t
// {
//      float32 apples;
//      float32 oranges;
//      float32 other_things;
// };
//
// 'struct' (keyword) -> 'lemons_t' (typename) -> '{' (start of definition) -> 'float32' (type) -> 'apples' (identifier) -> ...
//
// then it's pretty much the same thing, but I'm torn on whether it's preferrable to do this, or if it's better to just say:
//
// 'struct' (keyword) -> 'lemons_t' (typename) -> '{' (start of definition) 
//
// as a "sub-declaration":
// 'float32' (typename) -> 'apples' (identifier) -> ';' (end of declaration)
//
// Where you would then chain these as nodes all the smae.
//
// decl.name = lemons_t;
// decl.type = CODE_DECLARATION_TYPE_STRUCTURE;

// NOTE(Sleepster): 
//
// By default there is a nasty race condition where we need to SEE an item's declaration first before we can know anything about it...
// this is a drawback of single pass compilers. However, this is where the "multipass" idea comes in. We will:
//
// - Load all files into memory
//
// FIRST PASS, GATHERING OF TYPES AND MACRO DEFINITIONS
// - Read over all files loaded into memory, adding whatever types they find (declared or defined) to the type table.
// - While reading for types, record and store macros
// 
// SECOND PASS
// - With knowledge of what types exist, we can now parse the files for items like function declarations and their arguments
//
// THIRD PASS
// - Use the information from the previous two passes to record all the information related to their declarations
//   (Member count, member names, function parameters and their types, etc.) as RTTI
struct macro_data_t
{
    bool8     is_valid;
    bool8    is_multiline;
    // NOTE(Sleepster): Whatever is #define BLAH 
    string_t  macro_name;
    // NOTE(Sleepster): Whatever is after the #define BLAH ... 
    string_t  macro_string;

    string_t *arguments;
    s32       argument_count;
    bool32    uses_va_args;
};

#define DEFAULT_KEYWORDS(X)               \
    X("Invalid",  TOKEN_KEYWORD_INVALID)  \
    X("struct",   TOKEN_KEYWORD_STRUCT)   \
    X("union",    TOKEN_KEYWORD_UNION)    \
    X("enum",     TOKEN_KEYWORD_ENUM)     \
    X("static",   TOKEN_KEYWORD_STATIC)   \
    X("extern",   TOKEN_KEYWORD_EXTERN)   \
    X("inline",   TOKEN_KEYWORD_INLINE)   \
    X("volatile", TOKEN_KEYWORD_VOLATILE) \
    X("const",    TOKEN_KEYWORD_CONST)    \
    X("auto",     TOKEN_KEYWORD_AUTO)     \
    X("typedef",  TOKEN_KEYWORD_TYPEDEF)

enum lexer_keyword_t
{
#define X(string, enum) enum,
    DEFAULT_KEYWORDS(X)
#undef X
};

struct keyword_t 
{
    string_t        string;
    lexer_keyword_t keyword_token;
};

struct code_type_t
{
    bool8    is_valid;
    string_t type_name;
    u64      type_id;
    u64      alias_of = INVALID_ID;
};

enum AST_type_flags_t
{
    AST_TYPE_FLAG_NONE     = BIT(0),
    AST_TYPE_FLAG_VOLATILE = BIT(1),
    AST_TYPE_FLAG_CONSTANT = BIT(2),
    AST_TYPE_FLAG_STATIC   = BIT(3),
    AST_TYPE_FLAG_ARRAY    = BIT(4),

    AST_NUMBER_FLAG_SIGNED = BIT(5),
    AST_NUMBER_FLAG_FLOAT  = BIT(6)
};

struct AST_type_t 
{
    code_type_t *type;
    u32          type_flags; // is_pointer, is constant, is volatile, etc.
    u32          pointer_count;
    u32          array_size;

    string_t     literal_value;
    union {
        s32      int_value;
        u32      unsigned_value;
        float32  float_value;
        string_t string_value;
    };
};

#define AST_NODE_TYPE_LIST(X) \
    X(AST_NODE_TYPE_INVALID, "AST_NODE_TYPE_INVALID")                \
    X(AST_NODE_TYPE_IDENTIFIER, "AST_NODE_TYPE_IDENTIFIER")          \
    X(AST_NODE_TYPE_STRING_LITERAL, "AST_NODE_TYPE_STRING_LITERAL")  \
    X(AST_NODE_TYPE_NUMBER, "AST_NODE_TYPE_NUMBER")                  \
    X(AST_NODE_TYPE_OPERATOR_EQUALS, "AST_NODE_TYPE_OPERATOR_EQUALS") \
    X(AST_NODE_TYPE_OPERATOR_BITSHIFT_LEFT, "AST_NODE_TYPE_OPERATOR_BITSHIFT_LEFT") \
    X(AST_NODE_TYPE_OPERATOR_BITSHIFT_RIGHT, "AST_NODE_TYPE_OPERATOR_BITSHIFT_RIGHT") \
    X(AST_NODE_TYPE_RETURN_TYPE, "AST_NODE_TYPE_RETURN_TYPE") \
    X(AST_NODE_TYPE_PROCEDURE, "AST_NODE_TYPE_PROCEDURE") \
    X(AST_NODE_TYPE_PROCEDURE_ARGUMENT, "AST_NODE_TYPE_PROCEDURE_ARGUMENT") \
    X(AST_NODE_TYPE_STRUCTURE, "AST_NODE_TYPE_STRUCTURE") \
    X(AST_NODE_TYPE_STRUCTURE_MEMBER, "AST_NODE_TYPE_STRUCTURE_MEMBER") \
    X(AST_NODE_TYPE_ENUM, "AST_NODE_TYPE_ENUM") \
    X(AST_NODE_TYPE_ENUM_MEMBER, "AST_NODE_TYPE_ENUM_MEMBER") \


enum AST_node_type_t
{
#define X(enum, string) enum,
    AST_NODE_TYPE_LIST(X)
#undef X
};

struct AST_node_t 
{
    u32         node_type;
    string_t    assigned_name;
    AST_type_t  type_data;

    u32         line_number;
    string_t    filename;

    AST_node_t *next_sibling;
    AST_node_t *first_child;
};

enum code_declaration_type_t
{
    CODE_DECLARATION_TYPE_INVALID,
    CODE_DECLARATION_TYPE_STRUCTURE,
    CODE_DECLARATION_TYPE_ENUM,
    CODE_DECLARATION_TYPE_PROCEDURE,
};

struct code_declaration_t
{
    string_t            name;
    u32                 type;
    u8                 *start;

    union {
        struct {
            AST_type_t  return_type;

            u32         argument_count;
            AST_node_t *first_argument;
        }procedure_info;

        struct {
            u32         member_count;
            AST_node_t *first_member;
        }structure_info;
    };
};

struct state_t
{
    // NOTE(Sleepster): Maps macro declarations to their values... 
    ticket_mutex_t            macro_table_mutex;
    HashTable_t(macro_data_t) macro_table;

    // NOTE(Sleepster): Maps type names to their actual type (accounts for typedef)... maps string -> string
    ticket_mutex_t            type_table_mutex;
    HashTable_t(code_type_t)  type_table;

    // NOTE(Sleepster): Map types to their definitions. string -> ID
    ticket_mutex_t            type_definition_table_mutex;
    HashTable_t(string_t)     type_definition_table;

    // NOTE(Sleepster): Maps function names to 
    ticket_mutex_t            function_table_mutex;
    HashTable_t(string_t)     function_table;
    
    // NOTE(Sleepster): Stores what files are already loaded... 
    ticket_mutex_t            loaded_file_table_mutex;
    HashTable_t(string_t)     loaded_file_table;

    // NOTE(Sleepster): ONLY WRITTEN TOO BY THE MAIN THREAD!!! 
    DynArray_t(keyword_t)     keywords;
};

global_variable state_t *g_state;

// TODO(Sleepster): We will need to make these thread local since these are not threadsafe... 
thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;


// ============= DEBUG CODE =====================
char *
get_node_type_name(u32 node_type)
{
    switch(node_type)
    {
#define X(enum, string) case enum: { return(string); }break;
        AST_NODE_TYPE_LIST(X)
#undef X
        default: {return("INVALID NODE!");};
    }
}

void
print_node_children(AST_node_t *node)
{
    for(AST_node_t *current_child = node->first_child;
        current_child;
        current_child = current_child->next_sibling)
    {
        if(current_child->node_type != AST_NODE_TYPE_STRUCTURE_MEMBER && 
           current_child->node_type != AST_NODE_TYPE_STRUCTURE        &&
           current_child->node_type != AST_NODE_TYPE_ENUM_MEMBER)
        {
            continue;
        }

        string_t name = STR("Anonymous Structure..."); 
        if(current_child->type_data.type)
        {
            name = current_child->type_data.type->type_name;
        }

        printf("Member:\n\tTypename: '%.*s', Node Type: '%s', Name: '%.*s'...\n",
               fprint_string(name), 
               get_node_type_name(current_child->node_type),
               fprint_string(current_child->assigned_name));

        if(current_child->first_child)
        {
            printf("\033[0m");
            printf("\n*** Nested Declaration: 'AST_NODE_TYPE_STRUCTURE' with a name of: '%.*s' found... members are: ***\n",
                   fprint_string(current_child->assigned_name));

            print_node_children(current_child);
            printf("*** END OF NESTED DECLARATION MEMBERS ***\n");
        }
    }
}

void
print_node_list(AST_node_t *top_level_node)
{
    for(AST_node_t *node = top_level_node;
        node;
        node = node->next_sibling)
    {
        if(node->node_type != AST_NODE_TYPE_STRUCTURE_MEMBER && 
           node->node_type != AST_NODE_TYPE_STRUCTURE        &&
           node->node_type != AST_NODE_TYPE_ENUM)
        {
            continue;
        }

        printf("\033[0m");
        printf("\n=================================================\n");

        printf("Declaration with a name of: '%.*s' found... members are:\n",
               fprint_string(node->assigned_name));

        print_node_children(node);

        printf("=================================================\n");
    }
}
// ============= DEBUG CODE =====================

internal_api void
register_default_keywords(void)
{
    g_state->keywords = c_dynarray_create(keyword_t);

    string_t default_keyword_strings[] = {
#define X(string, enum) STR(string),
        DEFAULT_KEYWORDS(X)
#undef X
    };

    lexer_keyword_t default_keyword_enums[] = {
#define X(string, enum) enum,
        DEFAULT_KEYWORDS(X)
#undef X
    };
    
    for(u32 index = 0;
        index < ArrayCount(default_keyword_strings);
        ++index)
    {
        keyword_t keyword     = {};
        keyword.string        = default_keyword_strings[index];
        keyword.keyword_token = default_keyword_enums[index];
        c_dynarray_push(g_state->keywords, keyword);
    }
}

internal_api keyword_t*
get_keyword(token_data_t token)
{
    keyword_t *result = null;
    c_dynarray_for(g_state->keywords, keyword_index)
    {
        keyword_t *keyword = g_state->keywords + keyword_index;
        if(c_string_compare(keyword->string, token.string))
        {
            result = keyword;
            break;
        }
    }

    if(result == null)
    {
        // NOTE(Sleepster): Should be invalid 
        result = g_state->keywords;
        Expect(result->keyword_token == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}

internal_api inline u64
id_from_string(string_t string, u64 modular)
{
    u64 result = 0;
    Expect(string.count > 0, "String passed to 'id_from_string()' was of size 0...\n");

    result  = c_fnv_hash_value(string.data, string.count);
    if(modular > 0) result %= modular;

    return(result);
}

internal_api u64 
register_typename(string_t type_name, u64 alias_id)
{
    u64 type_id       = id_from_string(type_name, 2048);
    code_type_t *type = null;
    TicketMutexScope(&g_state->type_table_mutex)
    {
        type = g_state->type_table.data + type_id;;
        if(!type->is_valid)
        {
            type->is_valid  = true;
            type->type_name = c_string_make_copy(&permanent_arena, type_name);
            type->type_id   = type_id;
            type->alias_of  = alias_id;
        }
    }

    return(type_id);
} 

internal_api code_type_t*
get_code_type(string_t type_name)
{
    u64 type_id       = id_from_string(type_name, 2048);
    code_type_t *type = null;

    TicketMutexScope(&g_state->type_table_mutex)
    {
        // TODO(Sleepster): The lack of a alias might kill us... oops. 
        type = g_state->type_table.data + type_id;
        if(!type->is_valid)
        {
            type->is_valid  = true;
            type->type_name = c_string_make_copy(&permanent_arena, type_name);
            type->type_id   = type_id;
        }
    }

    return(type);
}

// TODO(Sleepster): If a macro expands to a keyword, add it to the keyword table.
//
// Three cases we have to handle here:
// - First is a macro defined as:
//      #define MACRO (128)
//  should not be read as having arguments.
//
// - Second is that a macro of:
//      #define MACRO() <a bunch of stuff>
//   Should not crash the program since there are no arguments.
//
// - Three is that a mcaro of:
//      #define VARIATRIC(first, second, ...) ...
//   should not crash the program
//
//   Macros are annoying.
//
//
//   When we find a macro we should perhaps parse it into an AST and store the AST of the macro inside the actual
//   macro_data_t so that when we find a macro later in the program's parsing, we can just attach the AST immediately in place of the found 
//   macro identifier. THIS SHOULD ONLY HAPEEN FOR MACROS THAT ARE EXPRESSIONS!!!!! MULTILINE MACROS ARE STILL SPECIAL
internal_api void
parse_macro_information(tokenizer_t *tokenizer, string_t filename, token_data_t macro_name)
{
    token_data_t token = c_tokenizer_get_next_token(tokenizer);

    // NOTE(Sleepster): I cannot wait to get a custom string formatter... 
    Expect(macro_name.type == TT_Identifier, "Invalid token on line: '%d' of file: '%.*s':\nToken: '%.*s' is invalid at this location, it must be an identifier.\n",
           tokenizer->line_count, filename.count, C_STR(filename), token.string.count, C_STR(token.string));

    macro_data_t macro = {};
    macro.is_valid     = true;
    macro.macro_name   = c_string_make_copy(&permanent_arena, macro_name.string);

    // NOTE(Sleepster): Check if the macro really takes arguments or if this is just an expression...
    if(token.type == TT_OpeningParen)
    {
        // NOTE(Sleepster): Peek tokens ahead 
        c_tokenizer_set_bookmark(tokenizer, token);
        while(token.type != TT_ClosingParen)
        {
            // TODO(Sleepster): We need to figure out what to do with __VA_ARGS__ (...)
            token = c_tokenizer_get_next_token(tokenizer);
            if(token.type == TT_Identifier)
            {
                ++macro.argument_count;
            }
        }
        token = c_tokenizer_restore_bookmark(tokenizer);

        // NOTE(Sleepster): allocate string_t * argument_count 
        macro.arguments = c_arena_push_array(&permanent_arena, string_t, macro.argument_count);

        // NOTE(Sleepster): Copy each of the argument strings
        u32 argument_index = 0;
        while(token.type != TT_ClosingParen)
        {
            token = c_tokenizer_get_next_token(tokenizer);
            if(token.type == TT_Identifier)
            {
                macro.arguments[argument_index++] = c_string_make_copy(&permanent_arena, token.string);
            }
        }

        // TODO(Sleepster): I hate this! TOO BAD!!
        string_builder_t temp_builder;
        c_string_builder_init(&temp_builder, MB(10));
        defer(c_string_builder_deinit(&temp_builder));

        Expect(token.type == TT_ClosingParen, "We have failed to parse the macro on line '%d' of file '%.*s' 'up a closing parenthesis... Current token is: '%.*s' which is not valid...\n",
               tokenizer->line_count, filename.count, C_STR(filename), token.string.count, C_STR(token.string))
        // NOTE(Sleepster): Get the rest of the macro on the right side 
        for(;;)
        {
            string_t line = c_tokenizer_eat_lines(&transient_arena, tokenizer, 1);
            c_string_builder_append_data(&temp_builder, line);

            s32 backslash = c_string_find_first_char_from_right(line, '\\');
            if(backslash != -1)
            {
                macro.is_multiline = true;
                c_string_builder_append_data(&temp_builder, STR("\n"));
            }
            else
            {
                break;
            }
        }

        macro.macro_string = c_string_make_copy(&permanent_arena, c_string_builder_get_current_string(&temp_builder));
    }
    // NOTE(Sleepster): If it doesn't take any arguments... 
    else
    {
        string_t macro_line = c_tokenizer_eat_lines(&transient_arena, tokenizer, 1);
        string_t full_macro = c_string_concat(&permanent_arena, token.string, macro_line);

        keyword_t *keyword = get_keyword(token);
        if(keyword->keyword_token != TOKEN_KEYWORD_INVALID)
        {
            keyword_t new_keyword     = {};
            new_keyword.string        = macro_name.string;
            new_keyword.keyword_token = keyword->keyword_token;
            c_dynarray_push(g_state->keywords, new_keyword);
        }

        macro.macro_string  = c_string_make_copy(&permanent_arena, full_macro);
    }

    TicketMutexScope(&g_state->macro_table_mutex)
    {
        c_hash_table_insert_pair(&g_state->macro_table, macro.macro_name, macro);
    }

    printf("\033[0m");
    printf("========== MACRO_DATA ===========\n");
    printf("Macro name:           '%.*s'...\n", macro.macro_name.count,   C_STR(macro.macro_name));
    printf("Macro Argument Count: '%d'...\n",   macro.argument_count);
    for(s32 argument_index = 0;
        argument_index < macro.argument_count;
        ++argument_index)
    {
        string_t argument = macro.arguments[argument_index];
        printf("\tArgument at index: '%d' is: '%.*s'...\n", argument_index, argument.count, C_STR(argument));
    }
    printf("Macro contents:       '%.*s'...\n", macro.macro_string.count, C_STR(macro.macro_string));
    printf("=================================\n\n");
}

internal_api AST_node_t*
AST_get_next_child_node(tokenizer_t *tokenizer, AST_node_t *root)
{
    AST_node_t *new_node  = c_arena_push_struct(&permanent_arena, AST_node_t); 
    new_node->node_type   = AST_NODE_TYPE_INVALID;
    new_node->filename    = tokenizer->filename;
    new_node->line_number = tokenizer->line_count;

    // TODO(Sleepster): maybe a doubly linked list??? Traversal is a bitch...
    if(root->first_child != null)
    {
        for(AST_node_t *current_child = root->first_child;
            current_child;
            current_child = current_child->next_sibling)
        {
            if(!current_child->next_sibling)
            {
                current_child->next_sibling = new_node;
                break;
            }
        }
    }
    else
    {
        root->first_child = new_node;
    }

    return(new_node);
}

internal_api AST_node_t*
AST_get_next_sibling_node(tokenizer_t *tokenizer, AST_node_t *root)
{
    AST_node_t *new_node  = c_arena_push_struct(&permanent_arena, AST_node_t); 
    new_node->node_type   = AST_NODE_TYPE_INVALID;
    new_node->filename    = tokenizer->filename;
    new_node->line_number = tokenizer->line_count;

    // TODO(Sleepster): maybe a doubly linked list??? Traversal is a bitch...
    if(root->next_sibling != null)
    {
        for(AST_node_t *current_child = root->next_sibling;
            current_child;
            current_child = current_child->next_sibling)
        {
            if(!current_child->next_sibling)
            {
                current_child->next_sibling = new_node;
                break;
            }
        }
    }
    else
    {
        root->next_sibling = new_node;
    }

    return(new_node);
}

internal_api void
generate_expression_AST(tokenizer_t *tokenizer, AST_node_t *sibling, token_data_t token)
{
    if(token.type == TT_OpeningParen)
    {
        token = c_tokenizer_get_next_token(tokenizer);
    }

    Expect(token.type == TT_Number || token.type == TT_Identifier, 
           "The token in this expression must be either a number or an identifier (such as a constexpr or a macro), but we instead found: '%.*s' which is invalid...\n", 
           fprint_token(token));

    while(token.type != TT_Semicolon && token.type != TT_ClosingParen && token.type != TT_Comma && token.type != TT_ClosingBrace)
    {
        AST_node_t *new_node = AST_get_next_sibling_node(tokenizer, sibling); 

        AST_type_t *type_data = &new_node->type_data;
        type_data->literal_value = c_string_make_copy(&permanent_arena, token.string);

        switch(token.type)
        {
            case TT_Dash:
            {
                type_data->type_flags   |= AST_NUMBER_FLAG_SIGNED;
                type_data->literal_value = c_string_make_copy(&permanent_arena, token.string);

                token = c_tokenizer_get_next_token(tokenizer);
            }
            case TT_Number:
            {
                new_node->node_type = AST_NODE_TYPE_NUMBER;

                token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
                if(peek_token.string.data[0] == 'f' && peek_token.string.count == 1)
                {
                    type_data->literal_value = c_string_concat(&permanent_arena, type_data->literal_value, peek_token.string);
                    type_data->type_flags   |= AST_NUMBER_FLAG_FLOAT; 
                    type_data->float_value   = c_string_read_float(token.string);

                    token = c_tokenizer_get_next_token(tokenizer);
                }
                else
                {
                    if(type_data->type_flags & AST_NUMBER_FLAG_SIGNED)
                    {
                        type_data->int_value = c_string_read_int(token.string);
                    }
                    else
                    {
                        type_data->unsigned_value = c_string_read_uint(token.string);
                    }

                    token = c_tokenizer_get_next_token(tokenizer);
                }
            }break;
            case TT_OpenAngleBracket:
            {
                token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
                if(peek_token.type == TT_OpenAngleBracket)
                {
                    new_node->node_type = AST_NODE_TYPE_OPERATOR_BITSHIFT_LEFT;
                    type_data->literal_value = c_string_concat(&permanent_arena, type_data->literal_value, peek_token.string);

                    token = c_tokenizer_get_next_token(tokenizer);
                    token = c_tokenizer_get_next_token(tokenizer);
                }
            }break;
            case TT_CloseAngleBracket:
            {
                token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
                if(peek_token.type == TT_CloseAngleBracket)
                {
                    new_node->node_type = AST_NODE_TYPE_OPERATOR_BITSHIFT_RIGHT;
                    type_data->literal_value = c_string_concat(&permanent_arena, type_data->literal_value, peek_token.string);

                    token = c_tokenizer_get_next_token(tokenizer);
                    token = c_tokenizer_get_next_token(tokenizer);
                }
            }break;
        }
        Expect(new_node->node_type != AST_NODE_TYPE_INVALID, "Somehow... this node being parsed by the expression handler is invalid...\n");
    }
}

internal_api void
generate_default_value_AST(tokenizer_t *tokenizer, AST_node_t *base)
{
    AST_node_t *equals = AST_get_next_sibling_node(tokenizer, base);
    equals->node_type = AST_NODE_TYPE_OPERATOR_EQUALS;

    token_data_t token = c_tokenizer_get_next_token(tokenizer);

    // TODO(Sleepster): 
    // If this turns out to be an expression that needs to parsed such as:
    //
    // (1 << 31) 
    //
    // then this node will remain invalid... This might be a problem? Or 
    // it's not and we can just continue to ignore it...
    AST_node_t *next_node = AST_get_next_sibling_node(tokenizer, base); 
    Expect(token.type == TT_Number     || 
           token.type == TT_Identifier || 
           token.type == TT_Dash       ||
           token.type == TT_OpeningParen, 
           "If the operator '=' is found inside of a structure, the token to the immediate right must either be a number or an identifier... Instead we have: '%.*s'\n", fprint_token(token));
    switch(token.type)
    {
        // NOTE(Sleepster): Will deliberately fall through 
        case TT_Dash:
        {
            token = c_tokenizer_get_next_token(tokenizer);
            next_node->type_data.type_flags   |= AST_NUMBER_FLAG_SIGNED;
            next_node->type_data.literal_value = c_string_make_copy(&permanent_arena, token.string);
        }
        case TT_Number:
        {
            // NOTE(Sleepster): If this is a semicolon, then this is easily parseable. 
            token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
            if(peek_token.type == TT_Semicolon || peek_token.type == TT_Comma || peek_token.string.data[0] == 'f')
            {
                next_node->node_type = AST_NODE_TYPE_NUMBER;
                next_node->type_data.literal_value = c_string_concat(&permanent_arena, next_node->type_data.literal_value, token.string);
                token_data_t float_token = c_tokenizer_peek_token(tokenizer);
                if(float_token.string.data[0] == 'f')
                {
                    next_node->type_data.literal_value = c_string_concat(&permanent_arena, next_node->type_data.literal_value, STR("f"));
                    next_node->type_data.type_flags   |= AST_NUMBER_FLAG_FLOAT;

                    token = c_tokenizer_get_next_token(tokenizer);
                }
                token = c_tokenizer_get_next_token(tokenizer);
            }
            else
            {
                // NOTE(Sleepster): Expression. 
                generate_expression_AST(tokenizer, next_node, token);
            }
        }break;
        case TT_OpeningParen:
        {
            // NOTE(Sleepster): Expression 
            generate_expression_AST(tokenizer, next_node, token);
        }break;
        default:
        {
            next_node->node_type = AST_NODE_TYPE_IDENTIFIER;
        }
    }

    //Expect(next_node->node_type != AST_NODE_TYPE_INVALID, "Somehow... this node being parsed by the default value handler is invalid...\n");
}

internal_api void
generate_declaration_AST(tokenizer_t *tokenizer, AST_node_t *new_node, token_data_t *token_out)
{                          
    AST_type_t type_data = {};

    token_data_t type_token = c_tokenizer_get_next_token(tokenizer);
    if(type_token.type == TT_ClosingBrace) return;

    Expect(type_token.type == TT_Identifier, "Token: '%.*s' is inside a structure definition and is in place of a declaration type, in this position it must be an identifier...\n", fprint_token(type_token));

    // NOTE(Sleepster): If we find volatile, it's on the left side... advance 
    if(c_string_compare(type_token.string, STR("volatile")))
    {
        type_data.type_flags |= AST_TYPE_FLAG_VOLATILE;
        type_token = c_tokenizer_get_next_token(tokenizer);
    }
    type_data.type = get_code_type(type_token.string);

    token_data_t name_token = c_tokenizer_get_next_token(tokenizer);
    Expect(name_token.type != TT_ClosingBrace, "When parsing a declaration, we have found a '}'... What???\n");
    Expect(name_token.type == TT_Identifier, "Token: '%.*s' is inside a structure definition, in this position it must be a variable name...\n", fprint_token(name_token));

    // NOTE(Sleepster): If we find it here, it was on the right side... whoever decided that volatile 
    // could go on either side should probably get a happy meal :) (this was an awful decision) 
    if(c_string_compare(name_token.string, STR("volatile")))
    {
        type_data.type_flags |= AST_TYPE_FLAG_VOLATILE;
        type_token = c_tokenizer_get_next_token(tokenizer);
    }

    new_node->assigned_name = name_token.string;
    new_node->type_data     = type_data;

    // NOTE(Sleepster): Check if it's an array or the end of the decl. 
    *token_out = c_tokenizer_get_next_token(tokenizer);
    if(token_out->type == TT_OpenBracket)
    {
        new_node->type_data.type_flags |= AST_TYPE_FLAG_ARRAY;

        token_data_t array_size_token = c_tokenizer_get_next_token(tokenizer);
        if(array_size_token.type == TT_Number)
        {
            new_node->type_data.array_size = (u32)c_string_read_int(array_size_token.string);
            c_tokenizer_get_next_token(tokenizer);
        }
        else if(array_size_token.type == TT_Identifier)
        {
            Expect(false, "Macros in array_sizes is currently not handled...\n");
            c_tokenizer_get_next_token(tokenizer);
        }
        else
        {
            Expect(false, "When parsing the array size of a member... we expected to find either a macro-name or a number... we failed to find either of those...\n");
        }

        *token_out = c_tokenizer_get_next_token(tokenizer);
    }
    else if(token_out->type == TT_Equals)
    {
        // NOTE(Sleepster): Some Structure Default value... 
         generate_default_value_AST(tokenizer, new_node);
        *token_out = c_tokenizer_get_next_token(tokenizer);
    }
    Expect(token_out->type == TT_Semicolon, "Expected a semicolon at the end of this member type->name expression, found: '%.*s' instead...\n", fprint_token(*token_out));
}

internal_api void
generate_structure_AST(tokenizer_t *tokenizer, token_data_t token, AST_node_t *structure)
{
    // NOTE(Sleepster): Declare that this stucture exists... 
    structure->node_type = AST_NODE_TYPE_STRUCTURE;

    u64 type_id = INVALID_ID;
    token_data_t structure_name = c_tokenizer_get_next_token(tokenizer);
    if(structure_name.type == TT_Identifier)
    {
        // NOTE(Sleepster): If it's anonymous, we don't set the name
        structure->assigned_name = c_string_make_copy(&permanent_arena, structure_name.string);

        AST_type_t type = {};
        type.type = get_code_type(structure->assigned_name);

        structure->type_data = type;
        type_id = register_typename(structure->assigned_name, INVALID_ID);

        token = c_tokenizer_get_next_token(tokenizer);
    }
    else
    {
        Expect(structure_name.type == TT_OpeningBrace, "Expected either a structure name or an '{' for an anonymous structure... but instead found: '%.*s'...\n", fprint_token(structure_name));
        token = structure_name;
    }

    // NOTE(Sleepster): Parse out data about each member as AST nodes 
    if(token.type != TT_Semicolon)
    {
        Expect(token.type == TT_OpeningBrace, "Expected token: '{'... instead found: '%.*s'", fprint_token(token));
        for(;;)
        {
            token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
            if(peek_token.type == TT_ClosingBrace)
            {
                // NOTE(Sleepster): Eat the closing }; and leave... 
                token = c_tokenizer_get_next_token(tokenizer);
                break;
            }

            AST_node_t *new_node = AST_get_next_child_node(tokenizer, structure); 

            // NOTE(Sleepster): If it is not a nested structure...
            peek_token = c_tokenizer_peek_token(tokenizer, 1);

            keyword_t *keyword = get_keyword(peek_token);
            if(keyword->keyword_token != TOKEN_KEYWORD_STRUCT &&
               keyword->keyword_token != TOKEN_KEYWORD_UNION)
            {
                generate_declaration_AST(tokenizer, new_node, &token);
            }
            else
            {
                token = c_tokenizer_get_next_token(tokenizer);

                // NOTE(Sleepster): If it is a nested structure...
                new_node->node_type =  AST_NODE_TYPE_STRUCTURE;
                generate_structure_AST(tokenizer, token, new_node);
            }
        }

        token = c_tokenizer_get_next_token(tokenizer);
        Expect(token.type == TT_Semicolon || token.type == TT_Identifier,
               "Token: '%.*s' is invalid here... the token must be either a C style typename or a semicolon...\n",
               fprint_token(token))
        if(token.type == TT_Identifier)
        {
            register_typename(token.string, type_id);
            if(structure->assigned_name.count == 0)
            {
                structure->assigned_name = c_string_make_copy(&permanent_arena, token.string);
            }
        }
    }
}

internal_api void
generate_enum_AST(tokenizer_t *tokenizer, token_data_t token, AST_node_t *new_node)
{
    new_node->node_type = AST_NODE_TYPE_ENUM;

    token_data_t name_token = c_tokenizer_get_next_token(tokenizer);
    if(name_token.type == TT_Identifier)
    {
        // NOTE(Sleepster): Named enum 
        new_node->assigned_name = c_string_make_copy(&permanent_arena, name_token.string);
        register_typename(new_node->assigned_name, INVALID_ID);

        code_type_t *enum_type = get_code_type(new_node->assigned_name);
        new_node->type_data.type = enum_type;
        token = c_tokenizer_get_next_token(tokenizer);
    }
    else
    {
        token = name_token;
    }

    Expect(token.type == TT_OpeningBrace, "Expected '{' when parsing and enum, the token was: '%.*s'...\n", fprint_token(token))

    token = c_tokenizer_get_next_token(tokenizer);
    while(token.type != TT_ClosingBrace)
    {
        AST_node_t *member = AST_get_next_child_node(tokenizer, new_node);
        member->node_type = AST_NODE_TYPE_ENUM_MEMBER;
        member->type_data = new_node->type_data;
        member->assigned_name = c_string_make_copy(&permanent_arena, token.string);

        token = c_tokenizer_get_next_token(tokenizer);
        if(token.type == TT_Equals)
        {
            generate_default_value_AST(tokenizer, member);
            token = c_tokenizer_get_next_token(tokenizer);

            // NOTE(Sleepster): Check if this is a strange enum formatted like:
            //
            // enum blah 
            // {
            //      NUMBER_DECL, <- The comma should get eaten since there is no member after...
            // };
            token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
            if(peek_token.type == TT_ClosingBrace || token.type == TT_Comma)
            {
                token = c_tokenizer_get_next_token(tokenizer);
            }
        }
    }

    printf("\033[0m");
    print_node_list(new_node);
}

internal_api void
generate_typedef_AST(tokenizer_t *tokenizer, token_data_t token, AST_node_t *new_node)
{
    token_data_t type_token = c_tokenizer_get_next_token(tokenizer);
    keyword_t   *keyword    = get_keyword(type_token);
    if(keyword->keyword_token == TOKEN_KEYWORD_STRUCT || 
       keyword->keyword_token == TOKEN_KEYWORD_UNION)
    {
        // NOTE(Sleepster): typedef struct
        generate_structure_AST(tokenizer, token, new_node);
    }
    else if(keyword->keyword_token == TOKEN_KEYWORD_ENUM)
    {
        // NOTE(Sleepster): typedef enum
        generate_enum_AST(tokenizer, token, new_node);
    }
    else
    {
        // NOTE(Sleepster): Simple typedef, we don't even need an AST for these. Just register them immediately.
        token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 2);
        if(peek_token.type == TT_Semicolon)
        {
            token_data_t new_type_token = c_tokenizer_get_next_token(tokenizer);
            Expect(new_type_token.type == TT_Identifier, "The item following typedef: (%.*s) was not a valid token for this location...\n", fprint_token(new_type_token));

            u64 primary_type_id = id_from_string(type_token.string, 2048);
            register_typename(new_type_token.string, primary_type_id);

            printf("\033[0m");
            printf("Registered new typedef by name of: '%.*s' that is an alias of: '%.*s'...\n", fprint_token(new_type_token), fprint_token(type_token));
        }
        else
        {
            Expect(false, "Currently, we only support simple to parse typedefs like 'typedef uint32_t u32'... therefore the token: '%.*s' is not allowed here yet...\n", fprint_token(peek_token));
        }
    }

}

int
main(int argc, char **argv)
{
    char **requested_filename = c_program_flag_add_string("-filename", null, "This is the file we wish to parse...\n");
    Expect(argc > 1, "You must pass a file to parse...\n");
    c_program_flag_parse_args(argc, argv);

    // TODO(Sleepster): Maybe replace malloc here... For now it's fine... 
    g_state = Alloc(state_t);
    ZeroStruct(*g_state);

    c_hash_table_init(&g_state->macro_table,           2048);
    c_hash_table_init(&g_state->type_table,            2048);
    c_hash_table_init(&g_state->type_definition_table, 2048);
    c_hash_table_init(&g_state->function_table,        2048);
    c_hash_table_init(&g_state->loaded_file_table,     2048);

    permanent_arena = c_arena_create(MB(200));
    transient_arena = c_arena_create(MB(50));

    register_default_keywords();

    string_t filename = STR(*requested_filename);
    string_t file_data = c_file_read_entirety(filename);

    // NOTE(Sleepster): First pass. We are simply going to gather information about types and macros. There will be no type info generation done here... 
    tokenizer_t tokenizer = {file_data, filename};
    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                token_data_t macro_type = c_tokenizer_get_next_token(&tokenizer);
                if(c_string_compare(macro_type.string, STR("define")))
                {
                    token_data_t macro_name  = c_tokenizer_get_next_token(&tokenizer);
                    macro_data_t *macro_info = null;
                    TicketMutexScope(&g_state->macro_table_mutex)
                    {
                        macro_info = c_hash_table_get_value_ptr(&g_state->macro_table, macro_name.string);
                    }

                    if(!macro_info->is_valid)
                    {
                        parse_macro_information(&tokenizer, filename, macro_name);
                    }
                }
                if(c_string_compare(macro_type.string, STR("if")))
                {
                    token_data_t if_zeroed = c_tokenizer_get_next_token(&tokenizer);
                    Expect(if_zeroed.type == TT_Number, "We currently do not handle anything other than '#if' in this case...\n");

                    if(c_string_compare(if_zeroed.string, STR("0")))
                    {
                        while(!c_string_compare(token.string, STR("endif")))
                        {
                            token = c_tokenizer_get_next_token(&tokenizer);
                        }
                    }
                }
            }break;
            case TT_Identifier:
            {
                keyword_t *keyword = get_keyword(token);

                AST_node_t new_node   = {};
                new_node.filename    = tokenizer.filename;
                new_node.line_number = tokenizer.line_count;
                switch(keyword->keyword_token)
                {
                    case TOKEN_KEYWORD_STRUCT:
                    {

                        generate_structure_AST(&tokenizer, token, &new_node);
                        print_node_list(&new_node);
                    }break;
                    case TOKEN_KEYWORD_ENUM:
                    {
                        generate_enum_AST(&tokenizer, token, &new_node);
                    }break;
                    case TOKEN_KEYWORD_TYPEDEF:
                    {
                        generate_typedef_AST(&tokenizer, token, &new_node);
                    }break;
                }
            }break;
        }
    }

    printf("File is '%d' lines...\n", tokenizer.line_count);
    c_arena_reset(&transient_arena);

    return(0);
}

#if 0
// NOTE(Sleepster): How we want to eventually have declarations
void
parse_declarations()
{
    c_dynarray_for(g_state->code_declarations, decl_index)
    {
        code_declaration_t *decl = g_state->code_declarations + decl_index;
        if(decl.flags & DECLARATION_TYPE_PROCEDURE)
        {
            // NOTE(Sleepster): Procedures are types... 
            decl.arguments = ;
            decl.number_of_non_default_arguments = ;
            decl.return_type = ;
            decl.name        = ;
        }

        struct type_info_member_t {
            u64 type_id;
            u32 type_modifiers;
            u32 type_flags;
            u32 type_size;
        };

        if(decl.flags & DECLARATION_TYPE_STRUCTURE)
        {
            decl.type_name    = ;
            decl.member_count = ;

            // NOTE(Sleepster): Members can be functions... 
            decl.members      = ;
        }
    }
}


 
// NOTE(Sleepster): How we want to handle notes. 
int
command_add(int A, int B) 
{
    return(A + B);
}METAPROGRAM_NOTE(CONSOLE_COMMAND)
#endif
