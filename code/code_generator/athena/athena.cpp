/* ========================================================================
   $File: athena.cpp $
   $Date: May 26 2026 10:56 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define ATHENA_IMPLEMENTATION 
#include "athena.h"

#include <c_types.h>
#include <c_base.h>
#include <c_synchronization.h>

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION 

// EXPERIMENTAL
#include "dynarray.h"
#include "hash_table.h"
// EXPERIMENTAL

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
#include <c_threadpool.cpp>

#include "athena_lexer.h"

/* TODO:
 *
 *
 * - [ ] Special markers to denote the ignoring of certain code_declarations / files.
 * - [ ] Variadic functions '...' and ##__VA_ARGS__ macros are not handled.
 *
 * - [-] No way of printing the namespace string such as "Namespace is: 'Structure::'...\n"
 * - [-] PURGE THE THREAD ARENAS
 *
 * - [X] #if !defined() header guards bricks the parser, deal with this
 * - [X] #if statements that use macros or constexpr values are invalid. Thus crash the program.
 * - [X] Nested macros are unaccounted for, macro expansion is not recursive and MUST be recursive
 *
 * - [X] Enums must be allowed in expressions.
 * - [X] Templated members such as "hash_table_t<Type> types" would blow up the parser
 * - [X] In the same way as above, templated members like "hash_table_t<Type> *table" would blow up the parser
 * - [X] C++ style [[attributes]] are not handled...
 * - [X] When we find an identifier in the place of an expected number, we should try to find the enum value as well..
 * - [X] No default definition of types like NULL or nullptr
 * - [X] Function overloading. (Right now, function overloading is automatically handled.)
 * - [X] Constexpr values are not parsed
 * - [X] Constructors and deconstructors handled
 */

// ATHENA FILES
#include "athena_lexer.h"
#include "athena_ast.h"
#include "athena_symbol_table.h"

struct athena_state_t
{
    dynarray_t<string_t>  filenames;
    dynarray_t<parser_t*> parser_table; 
};

global_variable athena_state_t state;

thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;

struct AST_node_t;

internal_api inline u64   type_id_from_identifier(string_t string);
internal_api code_type_t* symbol_table_register_typename(string_t type_name, u32 expected_metatype, u64 alias_id = INVALID_ID);

static ticket_mutex_t error_mutex;

internal_api void
report_error(parser_t *parser, char *message, ...)
{
    lexer_t *lexer = &parser->lexer;
    char buffer[8096] = {};

    va_list arg_ptr;
    va_start(arg_ptr, message);
    int length = vsnprintf(buffer, sizeof(buffer), message, arg_ptr);
    va_end(arg_ptr);

    TicketMutexScope(&error_mutex)
    {
        fprintf(stderr, "\033[31m[Athena Error]: File: '%.*s', Line: '%d': %.*s\033[0m\n", 
                fprint_string(parser->filename), 
                lexer->current_stream->line_number + 1, 
                length, 
                buffer);

        fprintf(stderr, "error reported... exiting...\n");
#if 0
        exit(-1);
#else
        AssertBreak;
#endif
    }
}

internal_api inline u64
type_id_from_identifier(string_t string)
{
    u64 result = 0;
    Expect(string.count > 0, "String passed to 'type_id_from_identifier()' was of size 0...\n");

    result = hash_table_hash_key(string);

    return(result);
}

#include "athena_lexer.cpp"
#include "athena_symbol_table.cpp"
#include "athena_ast.cpp"

internal_api void
DEBUG_indent(u32 depth)
{
    for(u32 i = 0; i < depth; ++i)
    {
        printf("  ");
    }
}

internal_api void
DEBUG_print_type_signature(AST_type_t *type)
{
    printf("%.*s ", fprint_string(type->code_type->identifier));
    for(u32 index = 0; 
        index < type->pointer_depth; 
        ++index)
    {
        printf("*");
    }
}

internal_api void
DEBUG_print_lambda_data(AST_node_t *lambda_AST, u32 indent)
{
    DEBUG_indent(indent);
    printf("lambda: %.*s\n", fprint_string(lambda_AST->identifier));

    DEBUG_indent(indent + 1);
    printf("returns: ");
    DEBUG_print_type_signature(&lambda_AST->lambda.return_type->type);
    printf("\n");

    DEBUG_indent(indent + 1);
    printf("params (%d)\n", lambda_AST->lambda.argument_count);

    for(AST_node_t *current_argument = lambda_AST->lambda.first_argument;
        current_argument;
        current_argument = current_argument->next_sibling)
    {
        DEBUG_indent(indent + 2);
        printf("%.*s: ", fprint_string(current_argument->identifier));
        DEBUG_print_type_signature(&current_argument->type);

        if(current_argument->expression.info)
        {
            AST_expression_value_t value = evaluate_expression_AST(current_argument->expression.info);
            printf("= ");

            switch(value.type)
            {
                case AST_EXPRESSION_VALUE_INT:
                {
                    printf("%ld", value.int_value);
                }break;
                case AST_EXPRESSION_VALUE_UNSIGNED:
                {
                    printf("%lu", value.unsigned_value);
                }break;
                case AST_EXPRESSION_VALUE_FLOAT:
                {
                    printf("%f", value.float32_value);
                }break;
                case AST_EXPRESSION_VALUE_DOUBLE:
                {
                    printf("%lf", value.float64_value);
                }break;
                case AST_EXPRESSION_VALUE_LITERAL:
                case AST_EXPRESSION_VALUE_IDENT:
                {
                    printf("%.*s", fprint_string(value.identifier_value));
                }break;
            }
        }

        printf("\n");
    }
}

internal_api void
DEBUG_print_structure_members(AST_node_t *structure, u32 indent)
{
    if(structure->struct_decl.inherited_type_info)
    {
        DEBUG_indent(indent);
        printf("Inherits from: '%.*s'\n", fprint_string(structure->struct_decl.inherited_type_info->identifier));
    }

    DEBUG_indent(indent + 1);
    printf("members: (%d)\n", structure->struct_decl.member_count);

    for(AST_node_t *current_member = structure->struct_decl.first_member;
        current_member;
        current_member = current_member->next_sibling)
    {
        if(current_member->node_type == AST_NODE_TYPE_STRUCTURE)
        {
            DEBUG_indent(indent + 2);
            printf("struct: %.*s\n", fprint_string(current_member->identifier));
            DEBUG_print_structure_members(current_member, indent + 3);

            continue;
        }

        DEBUG_indent(indent + 2);
        DEBUG_print_type_signature(&current_member->type);
        printf("%.*s", fprint_string(current_member->identifier));

        if(current_member->type.flags & AST_TYPE_MODIFIER_FLAG_ARRAY)
        {                        
            Assert(current_member->array_data.array_expression);
            for(AST_node_t *current_array = current_member->array_data.array_expression;
                current_array;
                current_array = current_array->next_sibling)
            {
                AST_expression_value_t value = evaluate_expression_AST(current_array);
                current_array->array_size    = value.int_value;

                printf("[%u]", current_array->array_size);
            }
        }
        else if(current_member->expression.info && 
                current_member->expression.evaluated &&
                current_member->node_type != AST_NODE_TYPE_LAMBDA)
        {
            AST_expression_value_t value = current_member->expression.value;
            printf(" = ");

            switch(value.type)
            {
                case AST_EXPRESSION_VALUE_INT:
                {
                    printf("%ld", value.int_value);
                }break;
                case AST_EXPRESSION_VALUE_UNSIGNED:
                {
                    printf("%lu", value.unsigned_value);
                }break;
                case AST_EXPRESSION_VALUE_FLOAT:
                {
                    printf("%f", value.float32_value);
                }break;
                case AST_EXPRESSION_VALUE_DOUBLE:
                {
                    printf("%lf", value.float64_value);
                }break;
                case AST_EXPRESSION_VALUE_LITERAL:
                case AST_EXPRESSION_VALUE_IDENT:
                {
                    printf("%.*s", fprint_string(value.identifier_value));
                }break;
            }
        }

        printf("\n");
    }
}

internal_api void
DEBUG_print_enum_data(AST_node_t *enum_AST, u32 indent)
{
    DEBUG_indent(indent);
    printf("values (%u)\n", enum_AST->struct_decl.member_count);
    for(AST_node_t *current_member = enum_AST->struct_decl.first_member;
        current_member;
        current_member = current_member->next_sibling)
    {
        DEBUG_indent(indent + 1);

        printf("%.*s", fprint_string(current_member->identifier));
        if(current_member->expression.info)
        {
            printf(" = ");

            AST_expression_value_t value = current_member->expression.value;
            switch(value.type)
            {
                case AST_EXPRESSION_VALUE_INT:
                {
                    printf("%ld", value.int_value);
                } break;
                case AST_EXPRESSION_VALUE_UNSIGNED:
                {
                    printf("%lu", value.unsigned_value);
                } break;
                case AST_EXPRESSION_VALUE_FLOAT:
                {
                    printf("%f", value.float32_value);
                } break;
                case AST_EXPRESSION_VALUE_DOUBLE:
                {
                    printf("%lf", value.float64_value);
                } break;
                case AST_EXPRESSION_VALUE_LITERAL:
                case AST_EXPRESSION_VALUE_IDENT:
                {
                    printf("%.*s", fprint_string(value.identifier_value));
                } break;
            }
        }
        printf("\n");
    }
}

internal_api void
parse_macro_info(parser_t *parser, macro_info_t *macro_info, lexer_token_t name_token)
{
    lexer_t *lexer = &parser->lexer;
    lexer_token_t token = lexer_peek_token(lexer);

    macro_info->name = c_string_make_copy(&permanent_arena, name_token.data);
    if(lexer->current_stream->line_number == lexer->secondary_stream->line_number)
    {
        string_builder_t temp_builder;
        c_string_builder_init(&temp_builder, MB(10));
        defer(c_string_builder_deinit(&temp_builder));

        // NOTE(Sleepster): If the macro takes arguments 
        token = lexer_get_next_token(lexer);

        language_keyword_t *keyword = get_keyword_from_identifier(token.data);
        if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
        {
            language_keyword_t new_keyword = {};
            new_keyword.identifier = macro_info->name;
            new_keyword.keyword_id = keyword->keyword_id;

            dynarray_add(&g_language_info.keywords, &new_keyword);
        }

        if(token.token_type == TOKEN_TYPE_OPEN_PAREN)
        {
            char separator = name_token.data.data[macro_info->name.count];
            if(separator != ' ')
            {
                lexer_push_bookmark(lexer, token);

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
                }
            }
            else
            {
                c_string_builder_append_data(&temp_builder, token.data);
            }
        }
        else
        {
            c_string_builder_append_data(&temp_builder, token.data);
        }

        // NOTE(Sleepster): We can fill out the rest of the macro info here... 
        lexer_push_bookmark(lexer, token);
        for(;;)
        {
            string_t line = lexer_eat_lines(&transient_arena, lexer, 1);
            if(line.count > 0)
            {
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
            else
            {
                break;
            }
        }
        lexer_pop_bookmark(lexer);

        // NOTE(Sleepster): Create the token stream's token buffer
        macro_info->expansion_string = c_string_make_copy(&permanent_arena, c_string_builder_get_current_string(&temp_builder));
        lexer_init_token_stream_from_string(&macro_info->expansion_token_stream, macro_info->expansion_string);

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
        if(token_count <= 0)
        {
            report_error(parser, "Somehow when parsing the token stream for the macro: '%.*s', token_count was 0...\n", fprint_token(name_token));
        }

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

        if(token_index != token_count)
        {
            report_error(parser, "Macro '%.*s': counted %u tokens but wrote %u...\n",
                         fprint_token(name_token), token_count, token_index);
        }

        lexer_pop_token_stream(lexer);
        macro_info->is_set = true;
    }
}

internal_api lexer_token_t
handle_macro_expansion(parser_t *parser, bool8 record_macro)
{
    lexer_t *lexer = &parser->lexer;

    lexer_token_t token = lexer_get_next_token(lexer);
    if(c_string_compare(token.data, STR("define")))
    {
        if(record_macro)
        {
            lexer_token_t name_token  = lexer_get_next_token(lexer);
            macro_info_t *macro_entry = hash_table_get_element_ptr(&parser->macro_table, name_token.data);

            if(!macro_entry->is_set)
            {
                macro_info_t macro_info = {};
                macro_info.is_set = true;

                parse_macro_info(parser, &macro_info, name_token);
                Assert(macro_info.name.count > 0);

                printf("========== MACRO DEFINITION ========\n");
                printf("Macro: '%.*s'...\n", fprint_string(macro_info.name));
                printf("Expansion: '%.*s'...\n", fprint_string(macro_info.expansion_string));
                printf("Argument Count: '%d'...\n", macro_info.argument_count);
                for(u32 index = 0;
                    index < macro_info.argument_count;
                    ++index)
                {
                    printf("\tArgument: '%.*s'...\n", fprint_string(macro_info.arguments[index]));
                }
                printf("====================================\n");

                language_keyword_t *keyword = get_keyword_from_identifier(macro_info.expansion_string);
                if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
                {
                    language_keyword_t new_keyword;
                    new_keyword.keyword_id = keyword->keyword_id;
                    new_keyword.identifier = c_string_make_copy(&parser->arena, token.data);

                    dynarray_add(&g_language_info.keywords, &new_keyword);
                    printf("Found token: '%.*s' which is a #define'd alias for the language keyword: '%.*s'...\n",
                           fprint_string(macro_info.name), fprint_string(keyword->identifier));
                }

                hash_table_add_element(&parser->macro_table, &macro_info, name_token.data);
            }
            else
            {
                printf("SKIPPED (already set) for macro name: '%.*s' in file '%.*s'\n",
                       fprint_token(name_token), fprint_string(parser->filename));
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
        if(if_token.token_type == TOKEN_TYPE_NUMBER) 
        {
            u32 number = c_string_read_uint(if_token.data);
            if(number == 0)
            {
                while(!c_string_compare(token.data, STR("endif")))
                {
                    token = lexer_get_next_token(lexer);
                }
            }
        }
        else if(if_token.token_type == TOKEN_TYPE_BANG)
        {
            lexer_eat_lines(&parser->arena, lexer, 1);
        }
        else
        {
            //report_error(lexer, "Currently the only item supported after a '#if ' is a number... instead got: '%.*s'...\n", fprint_token(if_token));
            while(!c_string_compare(token.data, STR("endif")))
            {
                token = lexer_get_next_token(lexer);
            }
        }
    }
    else if(c_string_compare(token.data, STR("ifdef")) ||
            c_string_compare(token.data, STR("elif"))  ||
            c_string_compare(token.data, STR("else")))
    {
        while(!c_string_compare(token.data, STR("endif")))
        {
            token = lexer_get_next_token(lexer);
        }
    }
    else if(c_string_compare(token.data, STR("pragma")))
    {
        lexer_eat_lines(&parser->arena, lexer, 1);
    }

    return(token);
}


/* TODO: Here's the steps of the parser:
 *
 * PHASE 1:
 *  - Find all macros and record them to their associated namespace and file declaration_context_t
 *  - Once finished across all files, join these items into a single global registery
 *  - Record macros that are in the '#ifdef' style like branching Assert definitions as branching. Record macro mutability.
 *    If we find a macro being defined several times, record that this macro either has mutltiple definitions or has changing definitions.
 *  
 *
 *  FOR #IFDEF
 *  - There is no single good way to know what is #ifdef at the time of analyzing as the definition might be in another file
 *    and thus makes it impossible to know what branch of the ifdef is the one that will be parsed. One thing we will do is allow
 *    the user to pass their own list of defined macros to the command line interface and allow the user to SET their own macros
 *    in the API version.
 *
 *    We will have to parse both branches of the conditional, storing the result inside the macro_info_t as a branching path.
 *    Once we have parsed all the macros in all files, we'll be able to determine which path is valid.
 *
 * PHASE 2:
 *  - Find all constants (constexpr) and record them the same way
 *  - Build ASTs for every structure, expression, enum, and lambda
 *  - DO NOT link the code_type_t to that of the AST. Types cannot be determined in this phase, 
 *    this also means expressions remain unresolved due to the fact that constants are unknown.
 *  - Collapse all the found AST_node_t (including constants) into a global table for processing. 
 *    The same "read only" style as the first phase.
 *
 * PHASE 3:
 * - Infer types by linking the AST to that of their code_type_t data
 * - Evaluate all expressions now that all constants, macros, and types are known.
 * - Output 
 */

/* TODO: NEW PLAN (?)
 * Since we want this tool to be very simple, plug and play like the previous metaprogram that could simply be invoked by:
 * ./metaprogram --dir=../code/
 *
 * We will likely need to redesign how we parse files. Right now, the files are as parallelized as can be. In theory, this is fine but sort of
 * puts the cart before the horse. We should move to a single linear approach, using threading when proven best.
 *
 * PHASE 1:
 *  - The user calls our program like so:
 *      ./athena_reflector --directory=../code/ --recursive=false
 *    From there, we will gather all files from the target directory, recursively if needed.
 *  - Once we have a list of the valid files to parse (just the .h files, if you care about the type information for something, put it in a header)
 *  - Due to the usage of macros, we need to build a dependency tree for the each of the files using their #includes (in the case of Unity builds, #include "*.cpp" is skipped). This is annoying, and may want a better approach.
 *  - Once there is some list of "what file requires what" for each of the files, we should arrange them in a linear file stream inside the lexer. This will go from the first file (acting as the top of the single virtual file) to the bottom.
 *  - Parse from this virtual top to virtual bottom linearly, recording macros and such as needed and interrupting the primary token stream as needed for items like macro definitions. This will allow us to parse macros in a stateful manner.
 *  - While parsing, store the location (the beginning index and ending index of the token stream) of items like structures, classes, functions, and enums in an array.
 *  - Once the file has been parsed top to bottom and all items that wish to be parsed are recorded, parse them and generate their type information.
 *  - Output the information
 */

// NOTE(Sleepster): PHASE 1: MACROS
internal_api void
record_file_macros(parser_t *parser)
{
    Assert(parser->lexer.current_stream->string.count > 0);

    u32 token_count = 0;
    while(parser->lexer.current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(&parser->lexer);
        ++token_count;
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(parser, true);
            }break;
        }
    }

    // NOTE(Sleepster): Once we gather the macros from this file, reset the token stream for the file 
    lexer_reset_token_stream(parser->lexer.current_stream);

    printf("Parsed file '%.*s'... Found: '%d' tokens...\n", fprint_string(parser->filename), token_count);
}

internal_api void
consolidate_macro_tables(void)
{
    for(u32 parser_index = 0;
        parser_index < g_symbol_table.file_count;
        ++parser_index)
    {
        parser_t *parser = g_symbol_table.file_parsers + parser_index;

        printf("File handled: '%.*s'...\n", fprint_string(parser->filename));
        for(const auto &element: parser->macro_table.used_entries)
        {
            macro_info_t *macro = &element->item;
            hash_table_add_element(&g_symbol_table.defined_global_macro_table, macro, macro->name);

            printf("Macro added: '%.*s'...\n", fprint_string(macro->name));
        }
    }
}

// NOTE(Sleepster): PHASE 2: ASTs AND CONSTANTS 
internal_api void
record_file_constants(parser_t *parser)
{
    lexer_t *lexer = &parser->lexer;
    while(lexer->current_stream->string.count > 0)
    {
        lexer_token_t token = lexer_get_next_token(lexer);
        switch(token.token_type)
        {
            case TOKEN_TYPE_CONSTEXPR:
            {
                lexer_token_t type_token = lexer_get_next_token(lexer);
                lexer_token_t name_token = lexer_peek_token(lexer);

                u32 pointer_depth = 0;
                while(name_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    name_token = lexer_peek_token(lexer, pointer_depth + 2);
                    ++pointer_depth;
                }

                lexer_token_t peek_token = lexer_peek_token(lexer, pointer_depth + 2);

                AST_node_t *node = null;
                if(peek_token.token_type == TOKEN_TYPE_EQUALS)
                {
                    lexer_token_t initializer_token = lexer_peek_token(lexer, pointer_depth + 3);
                    // NOTE(Sleepster): If this is not an initializer list 
                    if(initializer_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                    {
                        // NOTE(Sleepster): It's an easily parsable expression 
                        node = AST_create_new_node(&parser->arena, parser->active_decl_context);

                        node->node_type  = AST_NODE_TYPE_CONSTEXPR;
                        node->identifier = c_string_make_copy(&permanent_arena, name_token.data);
                        if(pointer_depth > 0)
                        {
                            node->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                        }

                        lexer_get_next_token(lexer);
                        lexer_get_next_token(lexer);

                        // TODO(Sleepster): Check to make sure there's actually an ending semicolon 
                        node->expression.info = generate_expression_AST(parser, 0, null);
                    }
                }
                else if(peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                {
                    // NOTE(Sleepster): Lambda, not a constant 
                    node = generate_lambda_AST(parser, type_token, pointer_depth, false);
                }

                if(node)
                {
                    hash_table_add_element(&parser->active_decl_context->code_decls, &node, node->identifier);
                }
            }break;
        }
    }

    lexer_reset_token_stream(lexer->current_stream);
}

internal_api void
build_file_AST(parser_t *parser)
{
    lexer_t *lexer = &parser->lexer;
    while(lexer->current_stream)
    {
        lexer_token_t token = parser_get_next_lexer_token(parser);
        switch(token.token_type)
        {
            case TOKEN_TYPE_POUND:
            {
                token = handle_macro_expansion(parser, false);
            }break;
            case TOKEN_TYPE_TYPEDEF:
            {
                generate_typedef_AST(parser);
            }break;
            case TOKEN_TYPE_STRUCT:
            case TOKEN_TYPE_UNION:
            case TOKEN_TYPE_CLASS:
            {
                generate_structure_AST(parser);
            }break;
            case TOKEN_TYPE_ENUM:
            {
                generate_enum_AST(parser);
            }break;
            case TOKEN_TYPE_NAMESPACE:
            {
                lexer_token_t namespace_token = parser_get_next_lexer_token(parser);
                token = namespace_token;

                string_t lexical_namespace = c_string_make_copy(&parser->arena, namespace_token.data);
                declaration_context_t *context = parser_create_declaration_context(parser, 
                                                                                   lexical_namespace,
                                                                                   parser->active_decl_context);
                parser_push_decl_context(parser, context);
            }break;
            case TOKEN_TYPE_CLOSE_BRACE:
            {
                // NOTE(Sleepster): If this is a semicolon than it's some global initializer... 
                lexer_token_t semicolon = parser_peek_next_lexer_token(parser);
                if(semicolon.token_type != TOKEN_TYPE_SEMICOLON)
                {
                    //parser_pop_decl_context(parser);
                }
            }break;
            case TOKEN_TYPE_CONSTEXPR:
            {
            }break;
            case TOKEN_TYPE_OPEN_BRACKET:
            {
                lexer_token_t next_token = parser_get_next_lexer_token(parser);
                if(next_token.token_type == TOKEN_TYPE_OPEN_BRACKET)
                {
                    // NOTE(Sleepster): This is a C++ attribute
                    lexer_token_t attribute_name = parser_get_next_lexer_token(parser);
                    lexer_token_t argument_token = parser_get_next_lexer_token(parser);
                    if(argument_token.token_type != TOKEN_TYPE_CLOSE_BRACKET)
                    {
                        report_error(parser, "Sorry, we don't support attributes with arguments...\n");
                    }

                    code_attribute_t attribute = {
                        .name = c_string_make_copy(&parser->arena, attribute_name.data),
                    };

                    dynarray_add(&parser->current_attribute_list, &attribute);
                }
            }break;
            case TOKEN_TYPE_TEMPLATE:
            {
                lexer_token_t open_angle_bracket = parser_get_next_lexer_token(parser);
                if(open_angle_bracket.token_type != TOKEN_TYPE_LESS_THAN)
                {
                    report_error(parser, 
                                 "Expected a '<' following a template declaration, instead found: '%.s'...\n", 
                                 fprint_token(open_angle_bracket));
                }

                code_attribute_t template_attribute = create_template_attribute(parser);
                dynarray_add(&parser->current_attribute_list, &template_attribute);
            }break;
            // NOTE(Sleepster): We don't really care about these two... 
            //case TOKEN_TYPE_INLINE:
            //case TOKEN_TYPE_STATIC:
            case TOKEN_TYPE_CONST:
            case TOKEN_TYPE_IDENT:
            {
                // NOTE(Sleepster): 
                // We want to create function defines like this:
                //
                // void *allocator(memory_arena_t *arena)
                //
                // where we parse this as:
                //
                // "allocator is a function that returns a void * and takes a memory_arena_t * as an argument" 

                bool8 is_const = false;
                if(token.token_type == TOKEN_TYPE_CONST)
                {
                    token = parser_get_next_lexer_token(parser);
                    is_const = true;
                }

                lexer_token_t return_type = token;
                lexer_token_t name_token  = parser_peek_next_lexer_token(parser);
                if(name_token.token_type == TOKEN_TYPE_LESS_THAN)
                {
                    while(name_token.token_type != TOKEN_TYPE_GREATER_THAN)
                    {
                        name_token = parser_get_next_lexer_token(parser);
                    }

                    name_token = parser_peek_next_lexer_token(parser);
                }

                u32 peek_amount = 2;
                u32 return_type_pointer_depth = 0;
                while(name_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    name_token = parser_peek_next_lexer_token(parser, peek_amount++);
                    ++return_type_pointer_depth;
                }

                if(name_token.token_type == TOKEN_TYPE_IDENT)
                {
                    lexer_token_t namespace_peek_token = parser_peek_next_lexer_token(parser, peek_amount);
                    if(namespace_peek_token.token_type == TOKEN_TYPE_DOUBLE_COLON)
                    {
                        // NOTE(Sleepster): Eat the namespace
                        parser_create_declaration_context(parser, name_token.data, parser->active_decl_context);
                        for(u32 peek_index = 0;
                            peek_index < peek_amount;
                            ++peek_index)
                        {
                            token = parser_get_next_lexer_token(parser);
                        }

                        // NOTE(Sleepster): Skip the next peek token because that's a name! 
                        lexer_token_t parenthesis_token = parser_peek_next_lexer_token(parser, 2);
                        if(parenthesis_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                        {
                            while(parenthesis_token.token_type != TOKEN_TYPE_CLOSE_PAREN)
                            {
                                parenthesis_token = parser_get_next_lexer_token(parser);
                            }

                            parenthesis_token = parser_get_next_lexer_token(parser);
                            if(parenthesis_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                            {
                                while(parenthesis_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                                {
                                    parenthesis_token = parser_get_next_lexer_token(parser);
                                }
                                //generate_lambda_AST(parser, return_type, return_type_pointer_depth, is_const);
                            }
                        }
                    }
                    else if(namespace_peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                    {
                        generate_lambda_AST(parser, return_type, return_type_pointer_depth, is_const);
                    }
                }
                else
                {
                    lexer_eat_lines(&transient_arena, lexer, 1);
                }
            }break;
            case TOKEN_TYPE_EOF:
            {
                return;
            }break;
            default:
            {
            }break;
        }
    }
}

internal_api void
consolidate_AST_nodes(void)
{
    for(u32 parser_index = 0;
        parser_index < g_symbol_table.file_count;
        ++parser_index)
    {
        parser_t *parser = g_symbol_table.file_parsers + parser_index;

        // NOTE(Sleepster): Consolidate all recorded declaration_context_t
        for(auto &decl_context: parser->recorded_decl_contexts)
        {
            // NOTE(Sleepster): Check if it's unique to the global table, adding it to the global table if it is.
            s32 index = 0;
            bool8 unique = true;
            for(auto &context: g_symbol_table.declaration_contexts)
            {
                if(context.context_ID == decl_context.context_ID)
                {
                    unique = false;
                    break;
                }
            }

            if(unique)
            {
                dynarray_add(&g_symbol_table.declaration_contexts, &decl_context);
            }
            else
            {
                // NOTE(Sleepster): Combine the knowledge of the two contexts to get a better picture of what symbols
                // are actually within this scope.
                declaration_context_t *recorded_context = &g_symbol_table.declaration_contexts[index];
                for(auto &element: decl_context.code_decls.used_entries) 
                {
                    AST_node_t *code_decl = element->item;
                    // TODO(Sleepster): Check to make sure that two items with the same name but of different types don't 
                    // have issues here. Such as:
                    // struct item_data
                    // {
                    //      char *data;
                    // }
                    //
                    // and
                    //
                    // void item_data(item_data *item, char *data);
                    //
                    // These two are completely different despite sharing the same name, so should have no issue.
                    hash_table_add_element(&recorded_context->code_decls, &code_decl, code_decl->identifier);
                }

                // NOTE(Sleepster): Do the same for each of the types, this is safe regardless of if they are 
                // unique types or not. 
                for(auto &local_type: decl_context.local_types.used_entries)
                {
                    code_type_t *type = local_type->item;
                    code_type_t *table_type = hash_table_get_element(&recorded_context->local_types, type->identifier);
                    if(!table_type || !table_type->is_registered)
                    {
                        hash_table_add_element(&recorded_context->local_types, &type, type->identifier);
                    }
                }
            }
        }
    }
}

// NOTE(Sleepster): PHASE 3: TYPE INFERENCE
internal_api void
deduce_AST_node_type_data(void)
{
    for(const auto &decl_context: g_symbol_table.declaration_contexts)
    {
        for(auto &element: decl_context.code_decls.used_entries)
        {
            AST_node_t *code_decl = element->item;
            if(code_decl->node_type != AST_NODE_TYPE_CONSTEXPR)
            {
                // NOTE(Sleepster): Set the code metatype 
                code_type_t *type = code_decl->type.code_type;
                if(type && type->code_metatype == CODE_TYPE_UNDEFINED)
                {
                    type->type_data = code_decl;
                    switch(code_decl->node_type)
                    {
                        case AST_NODE_TYPE_ENUM:
                        {
                            type->code_metatype = CODE_TYPE_ENUM;
                        }break;
                        case AST_NODE_TYPE_STRUCTURE:
                        {
                            type->code_metatype = CODE_TYPE_STRUCTURE;
                        }break;
                        case AST_NODE_TYPE_LAMBDA:
                        {
                            type->code_metatype = CODE_TYPE_LAMBDA;
                        }break;
                        default:
                        {
                            InvalidCodePath;
                        }break;
                    }
                }

                switch(code_decl->node_type)
                {
                    case AST_NODE_TYPE_STRUCTURE:
                    case AST_NODE_TYPE_ENUM:
                    {
                        // NOTE(Sleepster): Evaluate expressions on members
                        for(AST_node_t *current_member = code_decl->struct_decl.first_member;
                            current_member;
                            current_member = current_member->next_sibling)
                        {
                            if(current_member->expression.info && !current_member->expression.evaluated)
                            {
                                current_member->expression.value = evaluate_expression_AST(current_member->expression.info);
                                current_member->expression.evaluated = true;
                            }
                            else if(current_member->type.flags & AST_TYPE_MODIFIER_FLAG_ARRAY)
                            {
                                Assert(current_member->array_data.array_expression);
                                for(AST_node_t *current_array = current_member->array_data.array_expression;
                                    current_array;
                                    current_array = current_array->next_sibling)
                                {
                                    AST_expression_value_t value = evaluate_expression_AST(current_array);
                                    current_array->array_size    = value.int_value;
                                }
                            }
                        }
                    }break;
                    case AST_NODE_TYPE_LAMBDA:
                    {
                        // TODO(Sleepster): Evaluate expressions on arguments 
                    }break;
                }
            }
            else
            {
                code_decl->expression.value     = evaluate_expression_AST(code_decl->expression.info);
                code_decl->expression.evaluated = true;
            }
        }
    }
}

internal_api void
consolidate_AST_types(void)
{
    // TODO(Sleepster): We are saving member identifiers as types. Meaning:
    // u32 type_data;
    //
    // the identifier type_data gets stored as a type along with u32 somehow.
    for(auto &decl_context: g_symbol_table.declaration_contexts)
    {
        for(auto &element: decl_context.local_types.used_entries)
        {
            code_type_t *type = element->item;
            Assert(type->identifier.count > 0 && type->identifier.data != 0);

            code_type_t *found = hash_table_get_element(&g_symbol_table.type_table, type->identifier);
            if(!found || !found->is_registered)
            {
                hash_table_add_element(&g_symbol_table.type_table, &type, type->identifier);
            }
        }
    }
}

internal_api void
parse_single_file(string_t filename)
{
    string_t file_data = c_file_read_entirety(filename);
    parser_t *parser   = parser_create(filename, file_data); 

    record_file_macros(parser);
    record_file_constants(parser);
    consolidate_macro_tables();

    build_file_AST(parser);
    consolidate_AST_nodes();
    deduce_AST_node_type_data();
}

VISIT_FILES(gather_files_in_directory)
{
    string_t filename = visit_file_data->fullname;
    string_t file_ext = c_string_get_file_ext_from_path(filename);
    if(!c_string_compare(file_ext, STR(".h")))
    {
        return;
    }

    if(c_string_compare(visit_file_data->directory_name, STR("meta"))      || 
       c_string_compare(visit_file_data->directory_name, STR("generated")) ||
       c_string_compare(visit_file_data->directory_name, STR("GENERATED"))) 
    {
        return;
    }

    string_t new_filename = c_string_make_copy(&permanent_arena, filename);
    printf("File added: '%.*s'...\n", fprint_string(new_filename));
    dynarray_add(&state.filenames, &new_filename);
}

internal_api void
parse_directory_type_data(void)
{
    // NOTE(Sleepster): Read the data for each of the files and create their parsers 
    for(u32 iterator = 0;
        iterator < state.filenames.used;
        ++iterator)
    {
        string_t filename = state.filenames[iterator];
        string_t filedata = c_file_read_entirety(filename);

        parser_t *parser = parser_create(filename, filedata);
        dynarray_add(&state.parser_table, &parser);
    }

#if 1
    for(u32 file_index = 0;
        file_index < state.filenames.used;
        ++file_index)
    {
        parser_t *file_parser = state.parser_table[file_index];
        Assert(file_parser->lexer.current_stream->string.data != null && file_parser->lexer.current_stream->string.count > 0);
        Assert(file_parser);

        printf("Parsing macros for file: '%.*s'...\n", fprint_string(file_parser->filename));
        record_file_macros(file_parser);
    }

    consolidate_macro_tables();

    for(u32 iterator = 0;
        iterator < state.filenames.used;
        ++iterator)
    {
        parser_t *file_parser = state.parser_table[iterator];
        Assert(file_parser);

        record_file_constants(file_parser);
        build_file_AST(file_parser);
    }

    consolidate_AST_nodes();
    deduce_AST_node_type_data();
    consolidate_AST_types();
#else
    // NOTE(Sleepster): Collect the macros for each of the files. 
    work_completion_fence_t macro_fence = {};
    for(u32 file_index = 0;
        file_index < state.filenames.used;
        ++file_index)
    {
        c_threadpool_push_work_order(&global_context->main_threadpool, [=]() {
            u32 thread_arena_is_valid = AtomicCompareExchange32(&permanent_arena.is_initialized,
                                                                true,
                                                                false);
            if(thread_arena_is_valid == false)
            {
                permanent_arena = c_arena_create(MB(10));
                transient_arena = c_arena_create(MB(10));
            }

            parser_t *file_parser = state.parser_table[file_index];
            Assert(file_parser->lexer.current_stream->string.data != null && file_parser->lexer.current_stream->string.count > 0);
            Assert(file_parser);

            printf("Parsing macros for file: '%.*s'...\n", fprint_string(file_parser->filename));
            record_file_macros(file_parser);
        }, &macro_fence);
    }

    c_threadpool_wait_on_fence(&global_context->main_threadpool, &macro_fence);

    // NOTE(Sleepster): Consolidate macros 
    consolidate_macro_tables();

    // NOTE(Sleepster): Record the AST_node_t for this file, both the constants and other items.
    work_completion_fence_t AST_fence = {};
    for(u32 iterator = 0;
        iterator < state.filenames.used;
        ++iterator)
    {
        c_threadpool_push_work_order(&global_context->main_threadpool, [iterator]() {
            parser_t *file_parser = state.parser_table[iterator];
            Assert(file_parser);

            record_file_constants(file_parser);
            build_file_AST(file_parser);
        }, &AST_fence);
    }

    c_threadpool_wait_on_fence(&global_context->main_threadpool, &AST_fence);

    consolidate_AST_nodes();
    deduce_AST_node_type_data();
#endif
}

internal_api void
print_type_info_data(string_builder_t *builder, code_type_t *type, AST_node_t *type_data, u32 indent_level)
{
    auto ident = [&]() {
        for(u32 index = 0;
            index < indent_level;
            ++index)
        {
            c_string_builder_append_data(builder, STR("\t"));
        }
    };

    ident();
    c_string_builder_sprintf(builder, ".type_name     = \"%.*s\",\n", fprint_string(type->identifier));

    //c_string_builder_sprintf(builder, ".alias_of      = \"%.*s\",\n", fprint_string(type_data->type.code_type->identifier));
    //c_string_builder_sprintf(builder, ".next_overload = \"%.*s\",\n", fprint_string(type_data->type.code_type->identifier));
    //c_string_builder_sprintf(builder, ".type          = \"%.*s\",\n", fprint_string(type->identifier));
    ident();
    c_string_builder_sprintf(builder, ".size          = sizeof(%.*s),\n", fprint_string(type_data->identifier));

    ident();
    c_string_builder_sprintf(builder, ".member_name   = \"%.*s\",\n", fprint_string(type_data->identifier));

    ident();
    c_string_builder_sprintf(builder, ".parent        = static_cast<type_info_struct_t*>(&DEFAULT_type_%.*s),\n", fprint_string(type->identifier));
    //c_string_builder_sprintf(&builder, ".array_size  = %lu,\n", type_data->type.flags);
    ident();
    c_string_builder_sprintf(builder, ".flags         = %lu,\n", type_data->type.flags);

    ident();
    c_string_builder_sprintf(builder, ".pointer_depth = %lu,\n", type_data->type.pointer_depth);
}

internal_api void
print_type_info_struct_member_info(string_builder_t *builder, code_type_t *type, AST_node_t *current_member)
{
    if(current_member->node_type == AST_NODE_TYPE_STRUCTURE)
    {
        return;
    }

    c_string_builder_sprintf(builder, "\t.%.*s = {\n", fprint_string(current_member->identifier));
    print_type_info_data(builder, type, current_member, 2);
    if(current_member->expression.evaluated)
    {
        c_string_builder_sprintf(builder, "\t\t.value = {\n");
        c_string_builder_sprintf(builder, "\t\t\t.type = %d\n", current_member->expression.value.type);
        switch(current_member->expression.value.type)
        {
            case AST_EXPRESSION_VALUE_INT:
            {
                c_string_builder_sprintf(builder, "\t\t\t.int64 = %ld\n", current_member->expression.value.int_value);
            }break;
            case AST_EXPRESSION_VALUE_UNSIGNED:
            {
                c_string_builder_sprintf(builder, "\t\t\t.u64 = %lu\n", current_member->expression.value.unsigned_value);
            }break;
            case AST_EXPRESSION_VALUE_FLOAT:
            {
                c_string_builder_sprintf(builder, "\t\t\t.float32 = %f\n", current_member->expression.value.float32_value);
            }break;
            case AST_EXPRESSION_VALUE_DOUBLE:
            {
                c_string_builder_sprintf(builder, "\t\t\t.float64 = %lf\n", current_member->expression.value.float32_value);
            }break;
            case AST_EXPRESSION_VALUE_IDENT:
            case AST_EXPRESSION_VALUE_LITERAL:
            {
                c_string_builder_sprintf(builder, "\t\t\t.string = \"%.*s\"\n", fprint_string(current_member->expression.value.identifier_value));
            }break;
        }
        c_string_builder_sprintf(builder, "\t\t};\n");
    }
    c_string_builder_sprintf(builder, "\t};\n");
}

void
athena_handle_type_info(const char *char_filepath, bool32 directory, bool32 recursive)
{
    string_t filepath = STR(char_filepath);
    if(!directory)
    {
        symbol_table_init(filepath, recursive);
        parse_single_file(filepath);
    }
    else
    {
        visit_file_data_t visit_info = c_directory_create_visit_data(gather_files_in_directory, recursive, null);
        symbol_table_init(filepath, recursive);

        c_directory_visit(filepath, &visit_info);
        parse_directory_type_data();
    }

    printf("Global symbol table\n");
    printf("  TYPES:\n");
    for(const auto &element : g_symbol_table.type_table.used_entries)
    {
        code_type_t *type = element->item;
        printf("    %.*s\n", fprint_string(type->identifier));
    }

    printf("  Declaration contexts: %u\n\n", g_symbol_table.declaration_contexts.used);
    for(const auto &scope : g_symbol_table.declaration_contexts)
    {
        printf("Context: %.*s\n", fprint_string(scope.lexical_scope));
        printf("  Types (%u)\n", scope.local_types.used_entries.used);

        printf("\n  Declarations (%u)\n", scope.code_decls.used_entries.used);
        for(const auto &element : scope.code_decls.used_entries)
        {
            AST_node_t *AST = element->item;
            switch(AST->node_type)
            {
                case AST_NODE_TYPE_CONSTEXPR:
                {
                    printf("    constexpr %.*s = ", fprint_string(AST->identifier));
                    switch(AST->expression.value.type)
                    {
                        case AST_EXPRESSION_VALUE_INT:
                        {
                            printf("%ld", AST->expression.value.int_value);
                        }break;
                        case AST_EXPRESSION_VALUE_UNSIGNED:
                        {
                            printf("%lu", AST->expression.value.unsigned_value);
                        }break;
                        case AST_EXPRESSION_VALUE_FLOAT:
                        {
                            printf("%.2f", AST->expression.value.float32_value);
                        }break;
                        case AST_EXPRESSION_VALUE_DOUBLE:
                        {
                            printf("%.2lf", AST->expression.value.float64_value);
                        }break;
                        case AST_EXPRESSION_VALUE_IDENT:
                        case AST_EXPRESSION_VALUE_LITERAL:
                        {
                            printf("%.*s", fprint_string(AST->expression.value.identifier_value));
                        }break;
                    }

                    printf("\n\n");
                }break;
                case AST_NODE_TYPE_STRUCTURE:
                {
                    printf("    struct %.*s\n", fprint_string(AST->identifier));
                    DEBUG_print_structure_members(AST, 3);
                    printf("\n");
                }break;
                case AST_NODE_TYPE_ENUM:
                {
                    printf("    enum %.*s\n", fprint_string(AST->identifier));
                    DEBUG_print_enum_data(AST, 3);
                    printf("\n");
                }break;
                case AST_NODE_TYPE_LAMBDA:
                {
                    DEBUG_print_lambda_data(AST, 2);
                    printf("\n");
                } break;
            }
        }

        printf("\n");
    }

#if 0
    string_t item_to_read = ...
    type_info_t *string_data       = type_info(string_t);
    type_info_t *item_to_read_data = type_info(item_to_read);
    type_info_t *item_data         = type_info_from_string(item_to_read);
    
    type_info_struct_t *string_type_info = static_cast<type_info_struct_t*>(string_data);
    for(auto &member: string_type_info.members_array)
    {
        printf("member_name: '%s'...\n", member->name);
        printf("type: '%s'...\n", metatype_to_name(member->metatype));
        printf("offset: '%d'...\n", member->offset);
        printf("size: '%d'...\n", member->size);
    }

    type_info_member_t *count = string_data.members.count;
    type_info_member_t *data  = string_data.members.data;

    s64 *member_variable  = meta_get_member_pointer(string_data, STR("count"));
    s64 *member_variable2 = meta_get_member_pointer(string_t, count); // second param is a type_info_member_t*
#endif

    string_builder_t file_builder;
    c_string_builder_init(&file_builder, MB(40));

    // NOTE(Sleepster): Create the structure types. 
    for(u32 type_index = 0;
        type_index < g_symbol_table.type_table.used_entries.used;
        ++type_index)
    {
        code_type_t *type = (code_type_t*)((g_symbol_table.type_table.used_entries[type_index])->item);

#if 0
        bool8 guarded = false;
        if(type->owner_file.count > 0 && 
           type->owner_file.data != null && 
          (type->code_metatype == CODE_TYPE_STRUCTURE || type->code_metatype == CODE_TYPE_ENUM || type->code_metatype == CODE_TYPE_LAMBDA))
        {
            string_t header_name = c_string_get_filename_from_path(type->owner_file);
            s32 index = c_string_find_first_char_from_right(header_name, '.');
            if(index != -1)
            {
                header_name.data[index] = '_';
            }
            string_t header_guard_string = c_string_to_upper(&permanent_arena, header_name);
            c_string_builder_sprintf(&file_builder, "#ifdef %.*s\n", fprint_string(header_guard_string));

            guarded = true;
        }
#endif

        switch(type->code_metatype)
        {
            case CODE_TYPE_STRUCTURE:
            case CODE_TYPE_ENUM:
            {
                c_string_builder_sprintf(&file_builder, "struct type_info_struct_%.*s : public type_info_struct_t {\n", fprint_string(type->identifier));
                c_string_builder_sprintf(&file_builder, "\tunion {\n");
                c_string_builder_sprintf(&file_builder, "\t\ttype_info_member_t member_array[%d];\n", type->type_data->struct_decl.member_count);
                for(AST_node_t *current_member = type->type_data->struct_decl.first_member;
                    current_member;
                    current_member = current_member->next_sibling)
                {
                    c_string_builder_sprintf(&file_builder, "\t\ttype_info_member_t %.*s;\n", fprint_string(current_member->identifier));
                }
                c_string_builder_sprintf(&file_builder, "\t}members;\n");
                c_string_builder_append_data(&file_builder, STR("};\n\n"));
            }break;
            case CODE_TYPE_LAMBDA:
            {
                c_string_builder_sprintf(&file_builder, "struct type_info_procedure_%.*s : public type_info_procedure_t {\n", fprint_string(type->identifier));
                c_string_builder_sprintf(&file_builder, "\tunion {\n");
                c_string_builder_sprintf(&file_builder, "\t\ttype_info_member_t argument_array[%d];\n", type->type_data->lambda.argument_count);
                if(type->type_data->lambda.first_argument)
                {
                    for(AST_node_t *current_argument = type->type_data->lambda.first_argument;
                        current_argument;
                        current_argument = current_argument->next_sibling)
                    {
                        c_string_builder_sprintf(&file_builder, "\t\ttype_info_member_t %.*s\n", fprint_string(current_argument->identifier));
                    }
                }
                c_string_builder_sprintf(&file_builder, "\t}arguments;\n");
                c_string_builder_append_data(&file_builder, STR("};\n\n"));
            }break;
        }

#if 0
        if(guarded)
        {
            c_string_builder_append_data(&file_builder, STR("#endif\n"));
        }
#endif
    }

    for(u32 type_index = 0;
        type_index < g_symbol_table.type_table.used_entries.used;
        ++type_index)
    {
        code_type_t *type = (code_type_t*)((g_symbol_table.type_table.used_entries[type_index])->item);
        switch(type->code_metatype)
        {
            case CODE_TYPE_STRUCTURE:
            case CODE_TYPE_ENUM:
            {
                c_string_builder_sprintf(&file_builder, "constexpr type_info_struct_%.*s DEFAULT_type_%.*s = {\n", fprint_string(type->identifier), fprint_string(type->identifier));
                print_type_info_data(&file_builder, type, type->type_data, 1);

                c_string_builder_sprintf(&file_builder, "\t.member_count = %d\n", type->type_data->struct_decl.member_count);
                for(AST_node_t *current_member = type->type_data->struct_decl.first_member;
                    current_member;
                    current_member = current_member->next_sibling)
                {
                    print_type_info_struct_member_info(&file_builder, type, current_member);
                }

                c_string_builder_append_data(&file_builder, STR("};\n\n"));
            }break;
            case CODE_TYPE_LAMBDA:
            {
                c_string_builder_sprintf(&file_builder, "constexpr type_info_procedure_%.*s type_%.*s = {\n", fprint_string(type->identifier), fprint_string(type->identifier));
                print_type_info_data(&file_builder, type, type->type_data, 1);

                c_string_builder_sprintf(&file_builder, "\t.argument_count = %d,\n", type->type_data->lambda.argument_count);
                for(AST_node_t *current_argument = type->type_data->lambda.first_argument;
                    current_argument;
                    current_argument = current_argument->next_sibling)
                {
                    print_type_info_struct_member_info(&file_builder, type, current_argument);
                }
                c_string_builder_sprintf(&file_builder, "};\n");
            }break;
        }
    }

    string_t string = c_string_builder_get_current_string(&file_builder);
    fprintf(stdout, "%.*s\n", fprint_string(string));
}

#if 0
            case CODE_TYPE_PRIMITIVE:
            {
                c_string_builder_append_data(&file_builder, STR("const static type_info_t type_info_%.*s = {\n"));
                c_string_builder_sprintf(&file_builder, "\t.type_name = \"%.*s\",\n", fprint_string(type->identifier));
                c_string_builder_sprintf(&file_builder, "\t.size = sizeof(%.*s),\n",  fprint_string(type->identifier));
                c_string_builder_append_data(&file_builder, STR("};\n"));
            }break;
            case CODE_TYPE_STRUCTURE:
            {
                c_string_builder_append_data(&file_builder, STR("const static type_info_struct_t type_info_struct_%.*s = {\n"));
                c_string_builder_sprintf(&file_builder, "\t.type_name = \"%.*s\",\n", fprint_string(type->identifier));
                c_string_builder_sprintf(&file_builder, "\t.size = sizeof(%.*s),\n",  fprint_string(type->identifier));
                c_string_builder_sprintf(&file_builder, "\t.member_count = %d,\n", type->struct_decl.member_count));
                for(AST_node_t *current_member = type->struct_decl.first_member;
                    current_member;
                    current_member->next_sibling)
                {
                    print_type_info_struct_member_info(&file_builder, type, current_member);
                }

                c_string_builder_append_data(&file_builder, STR("};\n"));
            }break;
            case CODE_TYPE_ENUM:
            {
            }break;
            case CODE_TYPE_LAMBDA:
            {
            }break;
#endif

int
main(int argc, char **argv)
{
    // NOTE(Sleepster): Just for the threadpool 
    c_global_context_init();

    // NOTE(Sleepster): Thread init 
    permanent_arena = c_arena_create(MB(10));
    transient_arena = c_arena_create(MB(10));

    // NOTE(Sleepster): This is a global READ ONLY dataset
    initialize_default_language_info();

    // NOTE(Sleepster): Thread init 
    Expect(argc > 1, "You must pass a file to parse or a target directory that contains these files...\n");

    char **requested_filename    = c_program_flag_add_string("-filename", null, "This is the file we wish to parse...\n");
    char   **requested_directory = c_program_flag_add_string("-directory", null, "Points to the directory you wish to parse...\n");
    bool32 *recursive            = c_program_flag_add_bool32("-recursive", false, "Denotes recursive parsing over the passed directory...\n");

    c_program_flag_parse_args(argc, argv);

#if 0
    //u32 thread_count = 1;
    u32 thread_count = sys_get_thread_count() - 1;
    c_threadpool_init(&global_context->main_threadpool, thread_count, MB(200), true, false);
#endif

    const char *filepath;
    bool8 directory = false;
    if(*requested_directory)
    {
        filepath  = *requested_directory;
        directory = true;
    }
    else
    {
        filepath = *requested_filename;
    }
   
    athena_handle_type_info(filepath, directory, *recursive);
    return(0);
}
