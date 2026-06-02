/* ========================================================================
   $File: athena.cpp $
   $Date: May 26 2026 10:56 am $
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

#include "athena_lexer.h"

thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;

// ATHENA FILES
#include "athena_lexer.h"

internal_api inline u64 type_id_from_identifier(string_t string, u64 modular = 2048);
internal_api u64        register_typename(string_t type_name, u64 alias_id = INVALID_ID);

#include "athena_lexer.cpp"
#include "athena_symbol_table.cpp"
#include "athena_ast.cpp"

internal_api inline u64
type_id_from_identifier(string_t string, u64 modular)
{
    u64 result = 0;
    Expect(string.count > 0, "String passed to 'type_id_from_identifier()' was of size 0...\n");

    result  = c_fnv_hash_value(string.data, string.count);
    if(modular > 0) result %= modular;

    return(result);
}

internal_api u64 
register_typename(string_t type_name, u64 alias_id)
{
    u64 type_id       = type_id_from_identifier(type_name);
    code_type_t *type = null;
    TicketMutexScope(&g_symbol_table.type_table_mutex)
    {
        type = g_symbol_table.type_table.data + type_id;;
        if(!type->is_set)
        {
            type->is_set     = true;
            type->identifier = c_string_make_copy(&permanent_arena, type_name);
            type->ID         = type_id;
            type->alias_of   = alias_id;
        }
    }

    return(type_id);
} 

internal_api void
parse_macro_info(lexer_t *lexer, macro_info_t *macro_info, lexer_token_t name_token)
{
    macro_info->name      = c_string_make_copy(&permanent_arena, name_token.data);
    macro_info->name_hash = ((c_fnv_hash_value(name_token.data.data, name_token.data.count)) % 2048);

    string_builder_t temp_builder;
    c_string_builder_init(&temp_builder, MB(10));
    defer(c_string_builder_deinit(&temp_builder));

    // NOTE(Sleepster): If the macro takes arguments 
    lexer_token_t token = lexer_get_next_token(lexer);

    language_keyword_t *keyword = symbol_table_get_keyword(token.data);
    if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
    {
        language_keyword_t new_keyword = {};
        new_keyword.identifier = macro_info->name;
        new_keyword.keyword_id = keyword->keyword_id;
        c_dynarray_push(g_symbol_table.keywords, new_keyword);
    }

    if(token.token_type == TOKEN_TYPE_OPEN_PAREN)
    {
        char separator = macro_info->name.data[macro_info->name.count];
        lexer_push_bookmark(lexer, token);
        if(separator != ' ')
        {
            // NOTE(Sleepster): Determine how many arguments the macro takes 
            while(token.token_type != TOKEN_TYPE_CLOSE_PAREN)
            {
                token = lexer_get_next_token(lexer);
                if(token.token_type == TOKEN_TYPE_IDENT)
                {
                    lexer_token_t peek_token = lexer_peek_token(lexer);
                    if(peek_token.token_type == TOKEN_TYPE_COMMA || peek_token.token_type == TOKEN_TYPE_CLOSE_PAREN)
                    {
                        ++macro_info->argument_count;
                    }
                }
            }
            token = lexer_pop_bookmark(lexer);

            // NOTE(Sleepster): Parse the arguments out of the macro's arg list 
            if(macro_info->argument_count > 0)
            {
                u32 argument_index = 0;

                macro_info->arguments = c_arena_push_array(&permanent_arena, string_t, macro_info->argument_count);
                while(token.token_type != TOKEN_TYPE_CLOSE_PAREN)
                {
                    token = lexer_get_next_token(lexer);
                    if(token.token_type == TOKEN_TYPE_IDENT)
                    {
                        lexer_token_t peek_token = lexer_peek_token(lexer);
                        if(peek_token.token_type == TOKEN_TYPE_COMMA || peek_token.token_type == TOKEN_TYPE_CLOSE_PAREN)
                        {
                            macro_info->arguments[argument_index++] = c_string_make_copy(&permanent_arena, token.data);
                        }
                    }
                }
                token = lexer_get_next_token(lexer);
            }
        }
    }
    c_string_builder_append_data(&temp_builder, token.data);

    // NOTE(Sleepster): We can fill out the rest of the macro info here... 
    lexer_push_bookmark(lexer, token);
    for(;;)
    {
        string_t line = lexer_eat_lines(&transient_arena, lexer, 1);
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
    lexer_pop_bookmark(lexer);

    // NOTE(Sleepster): Create the token stream's token buffer
    macro_info->expansion_string = c_string_make_copy(&permanent_arena, c_string_builder_get_current_string(&temp_builder));
    macro_info->expansion_token_stream = init_token_stream_from_string(macro_info->expansion_string);

    lexer_push_token_stream(lexer, &macro_info->expansion_token_stream);

    u32 token_count = 0;
    while(lexer->current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(lexer);
        if(token.token_type != TOKEN_TYPE_BACKSLASH)
        {
            ++token_count;
        }
    }
    lexer_reset_token_stream(lexer->current_stream);

    Expect(token_count > 0, "Somehow when parsing the token stream for the macro: '%.*s', token_count was 0...\n", fprint_token(name_token));
    macro_info->expansion_token_stream.token_buffer         = c_arena_push_array(&permanent_arena, lexer_token_t, token_count);
    macro_info->expansion_token_stream.buffered_token_count = token_count;

    u32 token_index = 0;
    while(lexer->current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(lexer);
        if(token.token_type != TOKEN_TYPE_BACKSLASH)
        {
            macro_info->expansion_token_stream.token_buffer[token_index++] = token;
        }
    }

    lexer_pop_token_stream(lexer, false);
    macro_info->is_set = true;
}

internal_api lexer_token_t
handle_macro_expansion(lexer_t *lexer, bool8 record_macro)
{
    lexer_token_t token = lexer_get_next_token(lexer);
    if(c_string_compare(token.data, STR("define")))
    {
        if(record_macro)
        {
            lexer_token_t name_token = lexer_get_next_token(lexer);
            TicketMutexScope(&g_symbol_table.macro_table_mutex)
            {
                macro_info_t *macro_info = c_hash_table_get_value_ptr(&g_symbol_table.macro_table, name_token.data);
                if(!macro_info->is_set)
                {
                    parse_macro_info(lexer, macro_info, name_token);
                    printf("========== MACRO DEFINITION ========\n");
                    printf("Macro: '%.*s'...\n", fprint_string(macro_info->name));
                    printf("Expansion: '%.*s'...\n", fprint_string(macro_info->expansion_string));
                    printf("Argument Count: '%d'...\n", macro_info->argument_count);
                    for(u32 index = 0;
                        index < macro_info->argument_count;
                        ++index)
                    {
                        printf("\tArgument: '%.*s'...\n", fprint_string(macro_info->arguments[index]));
                    }
                    printf("====================================\n");

                    language_keyword_t *keyword = symbol_table_get_keyword(macro_info->expansion_string);
                    if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
                    {
                        c_dynarray_push(g_symbol_table.keywords, token.data);
                        printf("Found token: '%.*s' which is a #define'd alias for the language keyword: '%.*s'...\n",
                               fprint_string(macro_info->name), fprint_string(keyword->identifier));
                    }
                }
            }
        }
        else
        {
            // NOTE(Sleepster): Just eat the macro
            for(;;)
            {
                string_t line = lexer_eat_lines(&transient_arena, lexer, 1);
                s32 backslash = c_string_find_first_char_from_right(line, '\\');
                if(backslash == -1)
                {
                    break;
                }
            }
        }
    }
    else if(c_string_compare(token.data, STR("if")))
    {
        lexer_token_t if_token = lexer_get_next_token(lexer);
        Expect(if_token.token_type == TOKEN_TYPE_NUMBER, "Currently the only item supported after a '#if ' is a number...\n");

        u32 number = c_string_read_uint(if_token.data);
        if(number == 0)
        {
            while(!c_string_compare(token.data, STR("endif")))
            {
                token = lexer_get_next_token(lexer);
            }
        }
    }

    return(token);
}

int
main(int argc, char **argv)
{
    symbol_table_init();

    permanent_arena = c_arena_create(MB(10));
    transient_arena = c_arena_create(MB(10));

    char **requested_filename = c_program_flag_add_string("-filename", null, "This is the file we wish to parse...\n");
    Expect(argc > 1, "You must pass a file to parse...\n");
    c_program_flag_parse_args(argc, argv);

    string_t filename  = STR(*requested_filename);
    string_t file_data = c_file_read_entirety(filename);

    // NOTE(Sleepster): Gather all the macros and all the types in the file...
    lexer_t lexer = lexer_create(file_data);
    while(lexer.current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(&lexer);
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(&lexer, true);
            }break;
            case TOKEN_TYPE_TYPEDEF:
            {
                // TODO(Sleepster): Just parse the typedefs... not ALL types.
                lexer_token_t peek_token = lexer_peek_token(&lexer);
                if(peek_token.token_type == TOKEN_TYPE_IDENT || peek_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    lexer_token_t next_token = lexer_peek_token(&lexer, 2);
                    if(next_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                    {
                        next_token = lexer_peek_token(&lexer, 3);
                        Expect(next_token.token_type == TOKEN_TYPE_IDENT,
                               "Expected to find an identifier following a 'typedef' expression, failed to find it... Instead found: '%.*s'... Currently only the type aliasing of simple types like the aforementioned notation is supported...\n",
                               fprint_token(next_token));
                    }

                    u64 main_type_ID = register_typename(peek_token.data);
                    register_typename(next_token.data, main_type_ID);
                    printf("[PREPARSE]: FOUND TYPE ALIAS: '%.*s' OF TYPE: '%.*s'...\n",
                           fprint_token(next_token), fprint_token(peek_token));
                }
            }break;
        }
    }

    // NOTE(Sleepster): Parse the rest of the file using the macros to intercept token streams.
    lexer_reset_token_stream(lexer.current_stream);
    for(;;)
    {
        lexer_token_t token = symbol_table_get_next_lexer_token(&lexer);
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(&lexer, false);
            }break;
            case TOKEN_TYPE_TYPEDEF:
            {
                generate_typedef_AST(&lexer);
            }break;
            case TOKEN_TYPE_STRUCT:
            case TOKEN_TYPE_UNION:
            case TOKEN_TYPE_CLASS:
            {
                generate_structure_AST(&lexer);
            }break;
            case TOKEN_TYPE_ENUM:
            {
                generate_enum_AST(&lexer);
            }break;
            case TOKEN_TYPE_INLINE:
            case TOKEN_TYPE_STATIC:
            case TOKEN_TYPE_IDENT:
            {
            }break;
            case TOKEN_TYPE_EOF:
            {
                goto done;
            }break;
            default:
            {
            }break;
        }
    }

done:
    return(0);
}
