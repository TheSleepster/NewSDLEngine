/* ========================================================================
   $File: athena.cpp $
   $Date: May 26 2026 10:56 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_synchronization.h>

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION 

#include "hash_table.h"
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
 * - [X] No default definition of types like NULL or nullptr
 * - [X] Function overloading. (Right now, function overloading is automatically handled.)
 * - [X] Constexpr values are not parsed
 * - [X] Constructors and deconstructors handled
 * - [ ] When we find an identifier in the place of an expected number, we should try to find the enum value as well..
 * - [ ] #if statments that use macros or constexpr values are invalid. Thus crash the program.
 * - [ ] No way of printing the namespace string such as "Namespace is: 'Structure::'...\n"
 * - [ ] C++ style [[attributes]] are not handled...
 * - [ ] Nested macros are unaccounted for, macro expansion is not recursive and MUST be recursive
 * - [ ] Templated members such as "hash_table_t<Type> types" would blow up the parser
 * - [ ] In the same way as above, templated members like "hash_table_t<Type> *table" would blow up the parser
 * - [ ] Macros such as:
 *
 *      #ifdef MATH_API_IMPL
 *      # define MATH_API 
 *      #else 
 *      # define MATH_API extern
 *      #endif
 *
 *      Break that macro parser.
 */

internal_api void
report_error(lexer_t *lexer, char *message, ...)
{
    char buffer[8096] = {};

    va_list arg_ptr;
    va_start(arg_ptr, message);
    int length = vsnprintf(buffer, sizeof(buffer), message, arg_ptr);
    va_end(arg_ptr);

    // TODO(Sleepster): Filename? 
    fprintf(stderr, "\033[31m[Athena Error] Line: '%d': %.*s\033[0m\n", lexer->current_stream->line_number + 1, length, buffer);
    fprintf(stderr, "error reported... exiting...\n");
#if 0
    exit(-1);
#else
    AssertBreak;
#endif
}

#define SYMBOL_TABLE_SIZE (4096)

struct scope_stack_t
{
    DynArray_t(u64) current_stack;
    s32             current_stack_depth;

    u64             active_scope_ID;
};

thread_static memory_arena_t permanent_arena;
thread_static memory_arena_t transient_arena;
thread_static scope_stack_t  thread_scope_stack;

internal_api void
push_scope_stack(string_t scope)
{
    u64 scope_ID = hash_table_hash_key(scope);

    c_dynarray_push(thread_scope_stack.current_stack, scope_ID);
    ++thread_scope_stack.current_stack_depth;
}

internal_api void
pop_scope_stack(void)
{
    Assert(thread_scope_stack.current_stack_depth >= 0);

    c_dynarray_pop(thread_scope_stack.current_stack);
    --thread_scope_stack.current_stack_depth;
}

// ATHENA FILES
#include "athena_lexer.h"
#include "athena_ast.h"
#include "athena_symbol_table.h"

struct AST_node_t;

internal_api inline u64   type_id_from_identifier(string_t string, u64 modular = SYMBOL_TABLE_SIZE);
internal_api code_type_t* symbol_table_register_typename(string_t type_name, u32 expected_metatype, u64 alias_id = INVALID_ID);

internal_api inline u64
type_id_from_identifier(string_t string, u64 modular)
{
    u64 result = 0;
    Expect(string.count > 0, "String passed to 'type_id_from_identifier()' was of size 0...\n");

    result = hash_table_hash_key(string);
    if(modular > 0) result %= modular;

    return(result);
}

#include "athena_lexer.cpp"
#include "athena_symbol_table.cpp"
#include "athena_ast.cpp"

internal_api void
parse_macro_info(lexer_t *lexer, macro_info_t *macro_info, lexer_token_t name_token)
{
    macro_info->name      = c_string_make_copy(&permanent_arena, name_token.data);
    macro_info->name_hash = ((hash_table_hash_key(name_token.data)) % SYMBOL_TABLE_SIZE);

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
    if(token_count <= 0)
    {
        report_error(lexer, "Somehow when parsing the token stream for the macro: '%.*s', token_count was 0...\n", fprint_token(name_token));
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
                macro_info_t *macro_info = hash_table_get_element_ptr(&g_symbol_table.macro_table, name_token.data);
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
        if(if_token.token_type != TOKEN_TYPE_NUMBER) 
        {
            //report_error(lexer, "Currently the only item supported after a '#if ' is a number... instead got: '%.*s'...\n", fprint_token(if_token));
        }
        else
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
    }

    return(token);
}

internal_api void
DEBUG_print_lambda_data(AST_node_t *lambda_AST)
{
    AST_type_t *return_type = &lambda_AST->lambda.return_type->type;
    printf("\tLambda return type: '%.*s ", fprint_string(return_type->code_type->identifier));
    for(u32 index = 0;
        index < return_type->pointer_depth;
        ++index)
    {
        printf("*");
    }
    printf("'\n");
    printf("Lambda takes '%d' arguments...\n", lambda_AST->lambda.argument_count);
    if(lambda_AST->lambda.argument_count > 0)
    {
        for(AST_node_t *current_argument = lambda_AST->lambda.first_argument;
            current_argument;
            current_argument = current_argument->next_sibling)
        {
            AST_type_t *argument_type = &current_argument->type;

            printf("\tArgument is: '%.*s' with a type: '%.*s",
                   fprint_string(current_argument->identifier), fprint_string(argument_type->code_type->identifier));
            for(u32 index = 0;
                index < argument_type->pointer_depth;
                ++index)
            {
                printf("*");
            }
            printf("'\n");
            if(current_argument->expression.info)
            {
                AST_expression_value_t value = evaluate_expression_AST(current_argument->expression.info);
                printf("\t\tArgument default value: ");
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
                    {
                        printf("%.*s", fprint_string(value.identifier_value));
                    }break;
                    case AST_EXPRESSION_VALUE_IDENT:
                    {
                        InvalidCodePath;
                    }break;
                }
                printf("\n");
            }
        }
    }
}

internal_api void
DEBUG_print_structure_members(AST_node_t *structure)
{
    for(AST_node_t *member = structure->struct_decl.first_member;
        member;
        member = member->next_sibling)
    {
        AST_type_t *type = &member->type;

        if(member->node_type != AST_NODE_TYPE_STRUCTURE)
        {
            printf("\t\t%.*s %.*s\n", fprint_string(type->code_type->identifier), fprint_string(member->identifier));
            if(type->flags != 0)
            {
                printf("\t\tType flags:\n");
#define X(enum, string, value) if(type->flags & enum) { printf("\t\t\t%s\n", string); }
                AST_TYPE_MODIFIER_FLAGS(X)
#undef X
            }

            if(type->flags & AST_TYPE_MODIFIER_FLAG_ARRAY)
            {
                AST_node_t *expression = member->expression.info;

                AST_expression_value_t value;
                if(expression)
                {
                    value = evaluate_expression_AST(expression);
                }

                type->array_size = value.int_value;
                printf("\t\tMember is an array of size: '%d'...\n", type->array_size);
            }

            if(member->type.flags & AST_TYPE_MODIFIER_FLAG_PROCEDURE)
            {
                DEBUG_print_lambda_data(member);
            }
        }
        else
        {
            printf("\tType: '%.*s' is a nested structure...\n", fprint_string(member->identifier));
            DEBUG_print_structure_members(member);
        }
    }
}

internal_api void
parse_file(string_t filename)
{
    string_t file_data = c_file_read_entirety(filename);

    // NOTE(Sleepster): Gather all the macros and all the constexpr in the file...
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
            case TOKEN_TYPE_CONSTEXPR:
            {
                lexer_token_t type_token = lexer_get_next_token(&lexer);
                lexer_token_t name_token = lexer_peek_token(&lexer);

                u32 pointer_depth = 0;
                while(name_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    name_token = lexer_peek_token(&lexer, pointer_depth + 2);
                    ++pointer_depth;
                }

                lexer_token_t peek_token = lexer_peek_token(&lexer, pointer_depth + 2);
                bool8 invalid_expression = false;

                AST_node_t *node = null;
                if(peek_token.token_type == TOKEN_TYPE_EQUALS)
                {
                    lexer_token_t initializer_token = lexer_peek_token(&lexer, pointer_depth + 3);
                    // NOTE(Sleepster): If this is not an initializer list 
                    if(initializer_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                    {
                        // NOTE(Sleepster): It's an easily parsable expression 
                        node = AST_create_new_node(&transient_arena);

                        node->node_type  = AST_NODE_TYPE_CONSTEXPR;
                        node->identifier = c_string_make_copy(&permanent_arena, name_token.data);
                        node->type.code_type = symbol_table_search_for_code_type(type_token.data);
                        if(!node->type.code_type)
                        {
                            node->type.code_type = symbol_table_register_typename(type_token.data, CODE_TYPE_UNDEFINED);
                        }

                        if(pointer_depth > 0)
                        {
                            node->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                        }

                        lexer_get_next_token(&lexer);
                        lexer_get_next_token(&lexer);

                        // TODO(Sleepster): Check to make sure there's actually an ending semicolon 
                        node->expression.info = generate_expression_AST(&lexer, 0, null);
                    }
                    else
                    {
                        invalid_expression = true;
                    }
                }
                else if(peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                {
                    // NOTE(Sleepster): Lambda, not a constant 
                    node = generate_lambda_AST(&lexer, type_token, pointer_depth, false);
                    invalid_expression = true;
                }

                if(!invalid_expression)
                {
                    AST_expression_value_t eval = evaluate_expression_AST(node->expression.info);
                    hash_table_add_element(&g_symbol_table.constants_table, eval, name_token.data);
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
                AST_node_t *typedef_AST = generate_typedef_AST(&lexer);
                if(typedef_AST)
                {
                    switch(typedef_AST->node_type)
                    {
                        case AST_NODE_TYPE_STRUCTURE:
                        {
                            store_structure_AST(typedef_AST);
                        }break;
                        case AST_NODE_TYPE_LAMBDA:
                        {
                            store_lambda_AST(typedef_AST);
                        }break;
                        case AST_NODE_TYPE_ENUM:
                        {
                            store_enum_AST(typedef_AST);
                        }break;
                    }
                }
            }break;
            case TOKEN_TYPE_STRUCT:
            case TOKEN_TYPE_UNION:
            case TOKEN_TYPE_CLASS:
            {
                AST_node_t *structure_AST = generate_structure_AST(&lexer);
                if(structure_AST)
                {
                    store_structure_AST(structure_AST);
                }
            }break;
            case TOKEN_TYPE_ENUM:
            {
                AST_node_t *enum_AST = generate_enum_AST(&lexer);
                if(enum_AST)
                {
                    store_enum_AST(enum_AST);
                }
            }break;
            case TOKEN_TYPE_NAMESPACE:
            {
                //token = symbol_table_get_next_lexer_token(&lexer);
                //push_scope_stack(token.data);
            }break;
            case TOKEN_TYPE_CLOSE_BRACE:
            {
                // if(thread_scope_stack.active_scope_ID)
                // {
                //     pop_scope_stack();
                // }
            }break;
            case TOKEN_TYPE_CONSTEXPR:
            {
                // TODO(Sleepster):
                // Right now there's a problem where this loop will process constexpr's that are lambdas twice, once as a constant
                // another as a valid type. Is this a problem? Idk... But right now I just don't care since it's not obvious that this
                // could be a problem.
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
                    token = symbol_table_get_next_lexer_token(&lexer);
                    is_const = true;
                }

                lexer_token_t return_type = token;
                lexer_token_t name_token  = lexer_peek_token(&lexer);

                u32 peek_amount = 2;
                u32 return_type_pointer_depth = 0;
                while(name_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    name_token = lexer_peek_token(&lexer, peek_amount++);
                    ++return_type_pointer_depth;
                }

                if(name_token.token_type == TOKEN_TYPE_IDENT)
                {
                    lexer_token_t namespace_peek_token = lexer_peek_token(&lexer, peek_amount);

                    bool8 namespaced = false;
                    if(namespace_peek_token.token_type == TOKEN_TYPE_DOUBLE_COLON)
                    {
                        // NOTE(Sleepster): Eat the namespace
                        push_scope_stack(name_token.data);
                        namespaced = true;

                        symbol_table_get_next_lexer_token(&lexer);
                        symbol_table_get_next_lexer_token(&lexer);
                    }

                    lexer_token_t parenthesis_token = lexer_peek_token(&lexer, peek_amount);
                    if(parenthesis_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                    {
                        AST_node_t *lambda = generate_lambda_AST(&lexer, return_type, return_type_pointer_depth, is_const);

                        store_lambda_AST(lambda);
                        if(namespaced)
                        {
                            pop_scope_stack();
                        } 
                    }
                }
                else
                {
                    lexer_eat_lines(&transient_arena, &lexer, 1);
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

VISIT_FILES(generate_project_RTTI)
{
    string_t filename = visit_file_data->fullname;
    string_t file_ext = c_string_get_file_ext_from_path(filename);

    if(!c_string_compare(file_ext, STR(".h")) && !c_string_compare(file_ext, STR(".cpp")))
    {
        return;
    }

    if(c_string_compare(visit_file_data->directory_name, STR("meta"))      || 
       c_string_compare(visit_file_data->directory_name, STR("generated")) ||
       c_string_compare(visit_file_data->directory_name, STR("GENERATED"))) 
    {
        return;
    }

    parse_file(filename);
}

int
main(int argc, char **argv)
{
    // NOTE(Sleepster): Just for the threadpool 
    c_global_context_init();

    //u32 thread_count = sys_get_thread_count();
    //c_threadpool_init(&global_context->main_threadpool, thread_count - 2, MB(10), false);

    // NOTE(Sleepster): Thread init 
    permanent_arena = c_arena_create(MB(10));
    transient_arena = c_arena_create(MB(10));
    thread_scope_stack.current_stack = c_dynarray_create(u64);

    // NOTE(Sleepster): Pushes the global scope 
    static const s32 global_scope_ID = 0;
    c_dynarray_push(thread_scope_stack.current_stack, global_scope_ID);
    ++thread_scope_stack.current_stack_depth;

    // NOTE(Sleepster): Thread init 

    symbol_table_init();
    Expect(argc > 1, "You must pass a file to parse...\n");

    char **requested_filename = c_program_flag_add_string("-filename", null, "This is the file we wish to parse...\n");
    char   **requested_directory = c_program_flag_add_string("-directory", null, "Points to the directory you wish to parse...\n");
    bool32 *recursive            = c_program_flag_add_bool32("-recursive", false, "Denotes recursive parsing over the passed directory...\n");

    c_program_flag_parse_args(argc, argv);
    if(!(*requested_directory))
    {
        string_t filename = STR(*requested_filename);
        parse_file(filename);
    }
    else
    {
        visit_file_data_t visit_info = c_directory_create_visit_data(generate_project_RTTI, *recursive, null);

        string_t directory = STR(*requested_directory);
        c_directory_visit(directory, &visit_info);
    }

    // NOTE(Sleepster): Print the parsed data 
    c_dynarray_for(g_symbol_table.structures, index)
    {
        AST_node_t *structure = g_symbol_table.structures[index]; 

        bool8 anon = true;
        if(!c_string_compare(structure->identifier, STR("anonymous")))
        {
            symbol_table_infer_type(structure);
            anon = false;
        }
        
        printf("====================================================================\n");
        printf("AST Node Type: '%s' is found:\n", print_AST_node_type(structure->node_type));
        printf("Type Name: %.*s\n", fprint_string(structure->identifier));

        AST_type_t *type = &structure->type;
        printf("Type ID is: %lu\n", type->code_type->ID);
        if(!anon)
        {
            printf("Type alias of: %lu\n", type->code_type->alias_of);
            printf("Metatype: '%s'\n", get_metatype_string(type->code_type->code_metatype));
            printf("code_type identifier is: '%.*s'...\n", fprint_string(type->code_type->identifier));
        }
        printf("type flags are:\n");

#define X(enum, string, value) if(type->flags & enum) { printf("%s\n", string); }
        AST_TYPE_MODIFIER_FLAGS(X)
#undef X
        printf("\tStructure Info!:\n");
        if(structure->struct_decl.inherited_type_info)
        {
            AST_node_t *inher = structure->struct_decl.inherited_type_info;
            printf("\t\tInherits from type: '%.*s'...\n", fprint_string(inher->identifier));
        }

        if(structure->struct_decl.first_member)
        {
            printf("\tStructure members are:\n");
            DEBUG_print_structure_members(structure);
        }

        printf("====================================================================\n");
    }

    c_dynarray_for(g_symbol_table.enums, index)
    {
        printf("====================================================================\n");
        AST_node_t *enum_AST = g_symbol_table.enums[index]; 

        bool8 anon = true;
        if(!c_string_compare(enum_AST->identifier, STR("anonymous")))
        {
            symbol_table_infer_type(enum_AST);
            anon = false;
        }

        printf("AST Node Type: '%s' is found:\n", print_AST_node_type(enum_AST->node_type));
        printf("Type Name: %.*s\n", fprint_string(enum_AST->identifier));

        AST_type_t *type = &enum_AST->type;
        if(!anon)
        {
            printf("Type alias of: %lu\n", type->code_type->alias_of);
            printf("Metatype: '%s'\n", get_metatype_string(type->code_type->code_metatype));
            printf("identifier is: '%.*s'...\n", fprint_string(type->code_type->identifier));
        }

        if(type->flags != 0)
        {
            printf("type flags are:\n");
#define X(enum, string, value) if(type->flags & enum) { printf("%s\n", string); }
            AST_TYPE_MODIFIER_FLAGS(X)
#undef X
        }

        printf("Enum members are:\n");
        for(AST_node_t *current_member = enum_AST->struct_decl.first_member;
            current_member;
            current_member = current_member->next_sibling)
        {
            AST_expression_value_t value = {};
            if(current_member->expression.info)
            {
                 value = evaluate_expression_AST(current_member->expression.info);
            }
            printf("Enum member: '%.*s'... Value is: '%lu'...\n", fprint_string(current_member->identifier), value.unsigned_value);
        }
        printf("====================================================================\n");
    }

    c_dynarray_for(g_symbol_table.lambdas, index)
    {
        printf("====================================================================\n");
        AST_node_t *lambda_AST = g_symbol_table.lambdas[index]; 
        symbol_table_infer_type(lambda_AST);

        printf("AST Node Type: '%s' is found:\n", print_AST_node_type(lambda_AST->node_type));
        printf("Type Name: %.*s\n", fprint_string(lambda_AST->identifier));

        AST_type_t *type = &lambda_AST->type;
        printf("Type ID is: %lu\n", type->code_type->ID);
        printf("Type alias of: %lu\n", type->code_type->alias_of);
        printf("Metatype: '%s'\n", get_metatype_string(type->code_type->code_metatype));

        printf("code_type identifier is: '%.*s'...\n", fprint_string(type->code_type->identifier));

        DEBUG_print_lambda_data(lambda_AST);

        printf("====================================================================\n");
    }

    return(0);
}
