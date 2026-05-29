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

thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;

// ATHENA FILES
#include "athena_lexer.cpp"
#include "athena_parser.cpp"

internal_api void
parse_macro_info(parser_state_t *parser, lexer_t *lexer, macro_info_t *macro_info, lexer_token_t name_token)
{
    macro_info->name      = c_string_make_copy(&permanent_arena, name_token.data);
    macro_info->name_hash = ((c_fnv_hash_value(name_token.data.data, name_token.data.count)) % 2048);

    string_builder_t temp_builder;
    c_string_builder_init(&temp_builder, MB(10));
    defer(c_string_builder_deinit(&temp_builder));

    // NOTE(Sleepster): If the macro takes arguments 
    lexer_token_t token = lexer_get_next_token(lexer);

    language_keyword_t *keyword = parser_get_keyword(parser, token);
    if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
    {
        language_keyword_t new_keyword = {};
        new_keyword.identifier = macro_info->name;
        new_keyword.keyword_id = keyword->keyword_id;
        c_dynarray_push(parser->keywords, new_keyword);
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
handle_macro_expansion(lexer_t *lexer, parser_state_t *parser, bool8 record_macro)
{
    lexer_token_t token = lexer_get_next_token(lexer);
    if(c_string_compare(token.data, STR("define")))
    {
        if(record_macro)
        {
            lexer_token_t name_token = lexer_get_next_token(lexer);
            TicketMutexScope(&parser->macro_table_mutex)
            {
                macro_info_t *macro_info = c_hash_table_get_value_ptr(&parser->macro_table, name_token.data);
                if(!macro_info->is_set)
                {
                    parse_macro_info(parser, lexer, macro_info, name_token);
                    printf("Macro: '%.*s'...\n", fprint_string(macro_info->name));
                    printf("Expansion: '%.*s'...\n", fprint_string(macro_info->expansion_string));
                    printf("Argument Count: '%d'...\n", macro_info->argument_count);
                    for(u32 index = 0;
                        index < macro_info->argument_count;
                        ++index)
                    {
                        printf("\tArgument: '%.*s'...\n", fprint_string(macro_info->arguments[index]));
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
    parser_state_t *parser = Alloc(parser_state_t);
    parser_init(parser);

    permanent_arena = c_arena_create(MB(10));
    transient_arena = c_arena_create(MB(10));

    char **requested_filename = c_program_flag_add_string("-filename", null, "This is the file we wish to parse...\n");
    Expect(argc > 1, "You must pass a file to parse...\n");
    c_program_flag_parse_args(argc, argv);

    string_t filename  = STR(*requested_filename);
    string_t file_data = c_file_read_entirety(filename);

    // NOTE(Sleepster): Gather all the macros in the file...  
    lexer_t lexer = lexer_create(file_data);
    while(lexer.current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(&lexer);
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(&lexer, parser, true);
            }break;
        }
    }

    // NOTE(Sleepster): Parse the rest of the file using the macros to intercept token streams.
    lexer_reset_token_stream(lexer.current_stream);
    for(;;)
    {
        lexer_token_t token = parser_get_next_lexer_token(parser, &lexer);
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(&lexer, parser, false);
            }break;
            case TOKEN_TYPE_EOF:
            {
                goto done;
            }break;
            default:
            {
                printf("Token is: '%.*s'...\n", fprint_token(token));
            }break;
        }
    }

done:
    return(0);
}
