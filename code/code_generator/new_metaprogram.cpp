/* ========================================================================
   $File: new_metaprogram.cpp $
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
// (Member count, member names, function parameters and their types, etc.) as RTTI

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

struct macro_data_t
{
    bool8     is_valid;
    // NOTE(Sleepster): Whatever is #define BLAH 
    string_t  macro_name;
    // NOTE(Sleepster): Whatever is after the #define BLAH ... 
    string_t  macro_string;

    string_t *arguments;
    s32       argument_count;
    bool32    uses_va_args;
};

enum code_declaration_flags_t
{
    CODE_DECLARATION_FLAG_POINTER,
};

struct code_declaration_t
{
    // NOTE(Sleepster): (static, extern) 
    string_t  storage_class;

    // NOTE(Sleepster): (inline, _NoReturn, etc.)
    string_t  function_specifiers[4];
    u32       specifier_count;

    // NOTE(Sleepster): (const, volatile) 
    string_t  type_qualifiers[2];
    u32       qualifier_count;

    string_t  return_type;
    u32       return_type_flags;

    string_t  name;
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
    HashTable_t(string)       loaded_file_table;

    // NOTE(Sleepster): ONLY WRITTEN TOO BY THE MAIN THREAD!!! 
    DynArray_t(keyword_t)     keywords;
};

global_variable state_t *g_state;

// TODO(Sleepster): We will need to make these thread local since these are not threadsafe... 
thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;

internal_api void
register_keywords(void)
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


// TODO(Sleepster): If a macro expands to a keyword, add it to the keyword table.

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

    // NOTE(Sleepster): If the macro takes arguments
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

internal_api inline u64
id_from_string(string_t string, u64 modular)
{
    u64 result = 0;
    Assert(string.count > 0);

    result  = c_fnv_hash_value(string.data, string.count);
    if(modular > 0) result %= modular;

    return(result);
}

internal_api u64
register_type(string_t type_name, u64 alias_id)
{
    u64 type_id = id_from_string(type_name, 2048);

    code_type_t *type_declaration = null;
    TicketMutexScope(&g_state->type_table_mutex)
    {
        type_declaration = g_state->type_table.data + type_id;
    }

    // NOTE(Sleepster): If the item being typedeffed does not exist yet in the type table, add it
    if(!type_declaration->is_valid)
    {
        type_declaration->is_valid  = true;
        type_declaration->type_name = c_string_make_copy(&permanent_arena, type_name);
        type_declaration->type_id   = type_id;
        type_declaration->alias_of  = alias_id;
    }

    return(type_id);
}

internal_api void
register_structured_type(tokenizer_t *tokenizer, string_t filename, token_data_t structure_name_token)
{
    string_t structure_name = structure_name_token.string;
    u64 struct_id = register_type(structure_name, INVALID_ID);

    if(structure_name_token.type == TT_OpeningBrace)
    {
        // NOTE(Sleepster): Anonymous structure
        printf("\033[0m");
        printf("anonymous structure found!...\n");

        return;
    }
    else if(structure_name_token.type != TT_Identifier)
    {
        // NOTE(Sleepster): This is an error... 
        Expect(false, "[Line: '%d', File: '%.*s']: Token of: '%.*s' is not valid after the declared name of a structure...\n",
               tokenizer->line_count, filename, fprint_token(structure_name_token));
    }

    // NOTE(Sleepster): Otherwise, parse. 
    token_data_t next_token = c_tokenizer_get_next_token(tokenizer);
    switch(next_token.type)
    {
        case TT_Semicolon:
        {
            return;
        }break;
        case TT_OpeningBrace:
        {
            // NOTE(Sleepster): Eat to the closing brace. 
            while(next_token.type != TT_ClosingBrace)
            {
                next_token = c_tokenizer_get_next_token(tokenizer);

                // NOTE(Sleepster): Handle nested structures
                if(c_string_compare(next_token.string, STR("struct")) || 
                   c_string_compare(next_token.string, STR("union")))
                {
                    token_data_t nested_name = c_tokenizer_get_next_token(tokenizer); 
                    register_structured_type(tokenizer, filename, nested_name);
                }
            }

            // NOTE(Sleepster): Check if this is a C style structure. 
            next_token = c_tokenizer_get_next_token(tokenizer);
            if(next_token.type == TT_Semicolon)
            {
                printf("\033[0m");
                printf("Structure of type: '%.*s'\n", 
                       structure_name.count, C_STR(structure_name));

                return;
            }
            else if(next_token.type == TT_Identifier)
            {
                // NOTE(Sleepster): Record this as an alias of the formerly declared... 
                register_type(next_token.string, struct_id);

                printf("\033[0m");
                printf("Structure of type: '%.*s', with a C style alias of: '%.*s'\n", 
                       structure_name.count, C_STR(structure_name),
                       fprint_token(next_token));
            }
            else
            {
                Expect(false, "[Line: '%d', File: '%.*s']: Token of: '%.*s' is not valid after the closing brace of a structure definition...\n",
                       tokenizer->line_count, filename, fprint_token(next_token));
            }
        }break;
        case TT_Colon:
        {
            // TODO(Sleepster): C++ stuff... 
        }break;
        default:
        {
            Expect(false, "[Line: '%d', File: '%.*s']: Token of: '%.*s' is not valid after the 'struct' keyword...\n",
                   tokenizer->line_count, filename, fprint_token(next_token));
        }break;
    }
}

internal_api code_declaration_t 
parse_declaration(tokenizer_t *tokenizer, token_data_t token, keyword_t *keyword)
{
    code_declaration_t decl = {};
    while(token.type != TT_Semicolon && token.type != TT_OpeningParen)
    {
        switch(keyword->keyword_token)
        {
            case TOKEN_KEYWORD_EXTERN:
            case TOKEN_KEYWORD_STATIC:
            {
                decl.storage_class = c_string_make_copy(&permanent_arena, token.string);
            }break;
            case TOKEN_KEYWORD_INLINE:
            {
                decl.function_specifiers[decl.specifier_count++] = c_string_make_copy(&permanent_arena, token.string);
            }break;
            case TOKEN_KEYWORD_CONST:
            case TOKEN_KEYWORD_VOLATILE:
            {
                decl.type_qualifiers[decl.qualifier_count++] = c_string_make_copy(&permanent_arena, token.string);
            }break;
            default:
            {
                switch(token.type)
                {
                    // NOTE(Sleepster): If it's an identifier, but not a keyword we need to find out where it belongs. 
                    case TT_Identifier:
                    {
                        // NOTE(Sleepster): Check if it's a valid type... 
                        u64 ID = id_from_string(token.string, 2048);

                        code_type_t *type = null;
                        TicketMutexScope(&g_state->type_table_mutex)
                        {
                            type = g_state->type_table.data + ID;
                        }

                        // NOTE(Sleepster): If it is a type... 
                        token_data_t name_token = c_tokenizer_get_next_token(tokenizer);
                        if(name_token.type == TT_Identifier)
                        {
                            token_data_t peek_token = c_tokenizer_peek_token(tokenizer, 1);
                            if(peek_token.type == TT_OpeningParen)
                            {
                                if(!type->is_valid)
                                {
                                    type->type_id   = ID;
                                    type->type_name = token.string;
                                    type->alias_of  = INVALID_ID;
                                    type->is_valid  = true;
                                }

                                // NOTE(Sleepster): If it has arguments 
                                peek_token = c_tokenizer_peek_token(tokenizer, 2);
                                if(peek_token.type == TT_Identifier)
                                {
                                    code_type_t *arg_type = null;
                                    TicketMutexScope(&g_state->type_table_mutex)
                                    {
                                        arg_type = g_state->type_table.data + ID;
                                    }

                                    if(arg_type->is_valid)
                                    {
                                        decl.return_type = c_string_make_copy(&permanent_arena, token.string);

                                        token = c_tokenizer_get_next_token(tokenizer);
                                        decl.name = c_string_make_copy(&permanent_arena, name_token.string);
                                    }
                                }
                                // NOTE(Sleepster): If it does not have arguments 
                                else if(peek_token.type == TT_ClosingParen)
                                {
                                    peek_token = c_tokenizer_peek_token(tokenizer, 3);
                                    if(peek_token.type == TT_Semicolon || peek_token.type == TT_OpeningBrace)
                                    {
                                        decl.return_type = c_string_make_copy(&permanent_arena, token.string);

                                        token = c_tokenizer_get_next_token(tokenizer);
                                        decl.name = c_string_make_copy(&permanent_arena, name_token.string);
                                    }
                                }
                            }
                        }
                    }break;
                }
            }break;
        }

        token   = c_tokenizer_get_next_token(tokenizer);
        keyword = get_keyword(token);
    }
    printf("\033[0m");
    printf("Declaration by name: '%.*s' has a return type of: '%.*s'...\n",
           fprint_string(decl.name), fprint_string(decl.return_type));
    if(decl.storage_class.data != null)
    {
        printf("Declaration has a storage class of: '%.*s'...\n", fprint_string(decl.storage_class));
    }

    for(u32 specifier_index = 0;
        specifier_index < decl.specifier_count;
        ++specifier_index)
    {
        string_t specifier = decl.function_specifiers[specifier_index];
        if(specifier.data != null)
        {
            printf("Declaration has a function specifier of: '%.*s'...\n", fprint_string(specifier));
        }
    }

    for(u32 qualifier_index = 0;
        qualifier_index < decl.qualifier_count;
        ++qualifier_index)
    {
        string_t qualifier = decl.type_qualifiers[decl.qualifier_count];
        if(qualifier.data != null)
        {
            printf("Declaration has a bonus type qualifier of: '%.*s'...\n", fprint_string(qualifier));
        }
    }

    return(decl);
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

    register_keywords();

    string_t filename = STR(*requested_filename);
    string_t file_data = c_file_read_entirety(filename);

    // NOTE(Sleepster): First pass. We are simply going to gather information about types and macros. There will be no type info generation done here... 
    tokenizer_t tokenizer = {file_data};
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
                // NOTE(Sleepster): If it's a type alias 
                if(keyword->keyword_token == TOKEN_KEYWORD_TYPEDEF)
                {
                    // NOTE(Sleepster): This will get the type of the item being typedeffed and then get it's runtime type_id (only for the metaprogram)
                    token_data_t type_name = c_tokenizer_get_next_token(&tokenizer);

                    keyword_t *keyword = get_keyword(type_name);
                    if(keyword)
                    {
                        if(keyword->keyword_token == TOKEN_KEYWORD_STRUCT ||
                           keyword->keyword_token == TOKEN_KEYWORD_UNION)
                        {
                            type_name = c_tokenizer_get_next_token(&tokenizer);
                            register_structured_type(&tokenizer, filename, type_name);
                        }
                        else if(keyword->keyword_token == TOKEN_KEYWORD_ENUM)
                        {
                            token_data_t enum_type_name = c_tokenizer_get_next_token(&tokenizer);
                            if(enum_type_name.type == TT_Identifier || 
                               enum_type_name.type == TT_OpeningBrace)
                            {
                                register_structured_type(&tokenizer, filename, enum_type_name);
                            }
                            else
                            {
                                Expect(false, "After declaring an enum, you MUST either have open parenthesis or an identifier immediately following the 'enum' keyword...\n");
                            }
                        }
                        else 
                        {
                            // TODO(Sleepster): Check this is not a function typedef
                            //
                            // typedef void function(int argument, int argument);
                            u64 type_id = register_type(type_name.string, INVALID_ID);

                            // NOTE(Sleepster): Now record the alias of said type
                            token_data_t type_alias = c_tokenizer_get_next_token(&tokenizer);
                            Expect(type_alias.type == TT_Identifier, "Token of type: '%.*s' is not allowed inside a typedef...\n",
                                   type_alias.string.count, C_STR(type_alias.string));

                            register_type(type_name.string, type_id);

                            printf("\033[0m");
                            printf("Typedef from type: '%.*s' to type: '%.*s'\n", 
                                   fprint_token(type_name),
                                   fprint_token(type_alias));
                        }
                    }
                }
                // NOTE(Sleepster): If it's a structure 
                else if(keyword->keyword_token == TOKEN_KEYWORD_STRUCT ||
                        keyword->keyword_token == TOKEN_KEYWORD_UNION)
                {
                    // TODO(Sleepster): in cases like the innards of macros or anonymous structure... this will barf.
                    token_data_t structure_name = c_tokenizer_get_next_token(&tokenizer);
                    Expect(structure_name.type == TT_Identifier, "You must have an identifier after declaring a structure...\n");

                    register_structured_type(&tokenizer, filename, structure_name);
                }
                // NOTE(Sleepster): If it's a enum
                else if(keyword->keyword_token == TOKEN_KEYWORD_ENUM)
                {
                    token_data_t enum_type_name = c_tokenizer_get_next_token(&tokenizer);
                    if(enum_type_name.type == TT_Identifier || 
                       enum_type_name.type == TT_OpeningBrace)
                    {
                        register_structured_type(&tokenizer, filename, enum_type_name);
                    }
                    else
                    {
                        Expect(false, "After declaring an enum, you MUST either have open parenthesis or an identifier immediately following the 'enum' keyword...\n");
                    }
                }
            }break;
        }
    }

    tokenizer = {file_data};
    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        if(token.type == TT_Identifier)
        {
            keyword_t *keyword = get_keyword(token);
            // NOTE(Sleepster): If it's some other identifier we can't easily discern 
            // for function declarations it looks like this:
            //
            // [storage_class] [qualifiers] [specifiers] [type] [function name]()
            parse_declaration(&tokenizer, token, keyword);
        }
    }
    printf("File is '%d' lines...\n", tokenizer.line_count);
    c_arena_reset(&transient_arena);

    return(0);
}

#if 0

METAPROGRAM_NOTE(CONSOLE_COMMAND) 
int
command_add(int A, int B) 
{
    return(A + B);
}
#endif
