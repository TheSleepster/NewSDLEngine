/* ========================================================================
   $File: athena_symbol_table.cpp $
   $Date: May 26 2026 04:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_symbol_table.h"

internal_api language_keyword_t*
get_keyword_from_identifier(string_t identifier)
{
    language_keyword_t *result = null;
#if 0
    for(u32 index = 0;
        index < g_language_info.keywords.used;
        ++index)
    {
        language_keyword_t *found = g_language_info.keywords + index;
        if(c_string_compare(found->identifier, identifier))
        {
            result = found;
            break;
        }
    }
#else
    for(auto &keyword: g_language_info.keywords)
    {
        if(c_string_compare(keyword.identifier, identifier))
        {
            result = &keyword;
            break;
        }
    }
#endif

    if(result == null)
    {
        // NOTE(Sleepster): First element should be invalid 
        result = g_language_info.keywords.items;
        Expect(result->keyword_id == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}


// NOTE(Sleepster): SYMBOL TABLE

internal_api void
symbol_table_init(string_t filepath, bool8 recursive)
{
    s32 file_count = 1;

    bool8 is_directory = sys_directory_exists(filepath);
    if(is_directory)
    {
        file_count = sys_directory_get_file_count(filepath, recursive);
    }

    g_symbol_table.file_count   = file_count;
    g_symbol_table.file_parsers = c_arena_push_array(&permanent_arena, parser_t, g_symbol_table.file_count); 

    g_symbol_table.defined_global_macro_table = hash_table_create<macro_info_t>(1024);
    g_symbol_table.is_initialized = true;
}

internal_api AST_node_t* 
symbol_table_find_code_declaration(string_t identifier)
{
    AST_node_t *result = null;
    for(auto &decl_context: g_symbol_table.declaration_contexts)
    {
        result = hash_table_get_element(&decl_context.code_decls, identifier);
        if(result)
        {
            break;
        }
    }

    return(result);
}

// PARSER
internal_api declaration_context_t*
parser_create_declaration_context(parser_t *parser, string_t scope_name, declaration_context_t *parent)
{
    declaration_context_t *result = null;

    u64 context_ID = hash_table_hash_key(scope_name);
    for(auto &context: parser->recorded_decl_contexts)
    {
        if(context.context_ID == context_ID && parent == context.parent_scope)
        {
            result = &context;
            printf("====================================\n");
            printf("Found declaration_context of: '%.*s'...\n", fprint_string(scope_name));
            printf("====================================\n");
            break;
        }
    }

    if(!result)
    {
        // NOTE(Sleepster): lexical_scope's string shares the lifetime of the string you passed in.
        declaration_context_t  new_context = {};
        new_context.local_types   = hash_table_create<code_type_t*>(512);
        new_context.code_decls    = hash_table_create<AST_node_t*>(512);
        new_context.lexical_scope = scope_name;
        new_context.context_ID    = context_ID;
        new_context.parent_scope  = parent;

        result = dynarray_add(&parser->recorded_decl_contexts, &new_context);
        printf("====================================\n");
        printf("Recorded declaration_context of: '%.*s'...\n", fprint_string(scope_name));
        
        if(parent && parent->lexical_scope.data)
        {
            printf("Parent context is: '%.*s'...\n", fprint_string(parent->lexical_scope));
        }
        printf("====================================\n");
    }

    return(result);
}

internal_api parser_t* 
parser_create(string_t filename, string_t file_data)
{
    Expect(g_symbol_table.is_initialized == true,
           "You must call symbol_table_init() before creating any file parsers...\n");

    u32 parser_index = AtomicIncrement32(&g_symbol_table.next_parser_index);
    parser_t *parser = g_symbol_table.file_parsers + parser_index;

    parser->arena    = c_arena_create(MB(1));
    parser->filename = c_string_make_copy(&parser->arena, filename);
    
    // NOTE(Sleepster): This has to pass the lexer by pointer or we get a weird use-after-return stack bug. 
    lexer_create(&parser->lexer, file_data);
    parser->macro_table = hash_table_create<macro_info_t>(1024);

    declaration_context_t *global_scope = parser_create_declaration_context(parser, STR("global"), null);
    Expect(g_language_info.language_primitive_types.items != null,
           "Cannot initialize the parser without primtive type information... Make sure you call initialize_default_language_info() before you call this function!\n");

    for(u32 primitive_index = 0;
        primitive_index < g_language_info.language_primitive_types.used;
        ++primitive_index)
    {
        code_type_t *primitive = g_language_info.language_primitive_types + primitive_index;
        hash_table_add_element(&global_scope->local_types, 
                               &primitive, 
                                primitive->identifier);
    }
        
    dynarray_add(&parser->decl_context_stack, &global_scope);
    parser->active_decl_context = parser->decl_context_stack[0];

    return(parser);
}

internal_api void
parser_push_decl_context(parser_t *parser, declaration_context_t *context)
{
    dynarray_add(&parser->decl_context_stack, &context);
    dynarray_add_if_unique(&parser->recorded_decl_contexts, context);

    parser->active_decl_context = parser->decl_context_stack[parser->decl_context_stack.used - 1];
}

internal_api void
parser_pop_decl_context(parser_t *parser)
{
    if(parser->decl_context_stack.used - 1 <= 0)
    {
        report_error(parser,
                     "We attempted to pop the decl_context stack, however this would pop the top level file context as well... This should not happen as the file context must constantly be at index 0...\n",
                     fprint_string(parser->filename));
    }

    dynarray_pop(&parser->decl_context_stack);
    parser->active_decl_context = parser->decl_context_stack[parser->decl_context_stack.used - 1];
}

internal_api void
initialize_default_language_info(void)
{
    string_t default_keyword_strings[] = {
#define X(string, enum, token_type) STR(string),
        DEFAULT_KEYWORD_LIST(X)
#undef X
    };

    keywords_t default_keyword_enums[] = {
#define X(string, enum, token_type) enum,
        DEFAULT_KEYWORD_LIST(X)
#undef X
    };

    // NOTE(Sleepster): Keywords 
    for(u32 index = 0;
        index < ArrayCount(default_keyword_strings);
        ++index)
    {
        language_keyword_t keyword = {};
        keyword.identifier = c_string_make_copy(&permanent_arena, default_keyword_strings[index]);
        keyword.keyword_id = default_keyword_enums[index];

        dynarray_add(&g_language_info.keywords, &keyword);
    }

    // NOTE(Sleepster): Primitives 
    string_t default_primitive_types[] = {
#define X(string) STR(string),
        DEFAULT_PRIMITIVE_TYPES_LIST(X)
#undef X
    };

    for(u32 index = 0;
        index < ArrayCount(default_primitive_types);
        ++index)
    {
        string_t type_name = default_primitive_types[index];
        u64 type_id = hash_table_hash_key(type_name);

        //code_type_t primitive    = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, type_id);
        code_type_t primitive   = {};
        primitive.is_registered = true;
        primitive.type_inferred = true;
        primitive.identifier    = c_string_make_copy(&permanent_arena, type_name);
        primitive.ID            = type_id;
        primitive.code_metatype = CODE_TYPE_PRIMITIVE;
        
        dynarray_add(&g_language_info.language_primitive_types, &primitive);
    }
}

internal_api lexer_token_stream_t 
parser_substitute_macro_arguments(parser_t *parser, lexer_token_t last_token, macro_info_t *macro_info)
{
    lexer_token_stream_t result = {};

    lexer_t *lexer = &parser->lexer;
    // NOTE(Sleepster): This right now is completely wrong.
    // Firstly, this function should just not be getting called when it is.
    // Secondly, this function should operate on TWO streams at the same time, eating from both and replacing the saved macro's arguments
    // with the corresponding argument from the invoked macro. like so:
    // #define item(name) item_##name
    //
    // In the event of this invocation:
    // item(gloves);
    //
    // We should see:
    // item_gloves
    
    lexer_token_t next_expansion_token = lexer_peek_token(lexer);
    // NOTE(Sleepster): If the macro takes arguments... 
    if(next_expansion_token.token_type == TOKEN_TYPE_OPEN_PAREN && last_token.data.data[last_token.data.count] != ' ')
    {
        Expect(next_expansion_token.token_type == TOKEN_TYPE_OPEN_PAREN,
               "When attempting to substitute arguments of a macro, we failed to find the invocation (which is an '(') and instead were left with: '%.*s'...\n",
               fprint_token(next_expansion_token));
        next_expansion_token = lexer_get_next_token(lexer);

        result = lexer_copy_token_stream(&parser->arena, &macro_info->expansion_token_stream);

        // NOTE(Sleepster): Go through each of the arguments and find every token that matches that argument, replace this
        // with the actual argument from the macro.
    
        u32 macro_argument_index = 0;
        while(next_expansion_token.token_type != TOKEN_TYPE_CLOSE_PAREN)
        {
            // NOTE(Sleepster): Get our argument string
            next_expansion_token = lexer_get_next_token(lexer);
            Expect(next_expansion_token.token_type != TOKEN_TYPE_COMMA,
                   "Expected to find another argument when expanding this macro, instead found: '%.*s'...\n",
                   fprint_token(next_expansion_token));

            if(macro_info->argument_count)
            {
                string_t macro_argument = macro_info->arguments[macro_argument_index];
                for(u32 token_index = 0;
                    token_index < macro_info->expansion_token_stream.buffered_token_count;
                    ++token_index)
                {
                    lexer_token_t *macro_token = result.token_buffer + token_index;
                    if(c_string_compare(macro_token->data, macro_argument))
                    {
                        *macro_token = next_expansion_token;
                        ++macro_argument_index;

                        break;
                    }
                }
                lexer_token_t check_token = lexer_peek_token(lexer, 1);
                if(check_token.token_type != TOKEN_TYPE_COMMA && check_token.token_type != TOKEN_TYPE_CLOSE_PAREN)
                {
                    report_error(parser,
                                 "Invalid token when parsing the end of this macro invocation's argument string... token found was: '%.*s'...\n",
                                 fprint_token(check_token));
                }
                lexer_get_next_token(lexer);
                if(check_token.token_type == TOKEN_TYPE_CLOSE_PAREN)
                {
                    break;
                }
            }
        }
    }
    // NOTE(Sleepster): If it does not... 
    else
    {
        result = macro_info->expansion_token_stream;
    }

    return(result);
}

internal_api lexer_token_t
parser_fetch_next_token(parser_t *parser)
{
    lexer_t *lexer = &parser->lexer;

    lexer_token_t result;
    result = lexer_get_next_token(lexer);
    if(result.token_type == TOKEN_TYPE_EOF)
    {
        lexer_pop_token_stream(lexer);
        result = lexer_get_next_token(lexer);
    }

    macro_info_t *macro = hash_table_get_element_ptr(&parser->macro_table, result.data);
    if(macro->is_set)
    {
        lexer_token_stream_t macro_stream = parser_substitute_macro_arguments(parser, result, macro);
        lexer_push_token_stream(lexer, &macro_stream);

        result = lexer_get_next_token(lexer);
        if(lexer->current_stream->string.count == 0 || result.token_type == TOKEN_TYPE_EOF)
        {
            lexer_pop_token_stream(lexer);
        }
    }
    
    return(result);
}


internal_api lexer_token_t
parser_get_next_lexer_token(parser_t *parser)
{
    lexer_token_t result;
    if(parser->buffered_token_count > 0)
    {
        result = parser->peek_ahead_buffer[parser->token_buffer_head];

        parser->token_buffer_head     = (parser->token_buffer_head + 1) % MAX_PEEK_AHEAD_TOKENS;
        parser->buffered_token_count -= 1;
    }
    else
    {
        result = parser_fetch_next_token(parser);
    }

    return(result);
}

internal_api lexer_token_t
parser_peek_next_lexer_token(parser_t *parser, u32 peek_amount = 1)
{
    lexer_token_t result = {};
    while(parser->buffered_token_count < peek_amount)
    {
        lexer_token_t token = parser_fetch_next_token(parser);

        u32 next_token_index = (parser->token_buffer_head + parser->buffered_token_count) % MAX_PEEK_AHEAD_TOKENS;
        parser->peek_ahead_buffer[next_token_index] = token;

        parser->buffered_token_count += 1;
    }

    u32 buffer_index = (parser->token_buffer_head + peek_amount - 1) % MAX_PEEK_AHEAD_TOKENS;
    result = parser->peek_ahead_buffer[buffer_index];

    return(result);
}

internal_api char *
get_metatype_string(u32 metatype)
{
    switch(metatype)
    {
#define X(enum, string) case enum: return string; break;
        CODE_TYPE_METATYPE_LIST(X)
        default: return "null";
#undef X
    }
}

// search for code_type
// * We must also be able to search within the current scope.
internal_api code_type_t*
parser_search_for_code_type(parser_t *parser, string_t identifier)
{
    code_type_t *result = null;
    for(s32 index = parser->decl_context_stack.used - 1;
        index >= 0;
        --index)
    {
        declaration_context_t *decl_context = parser->decl_context_stack[index];
        code_type_t *found = hash_table_get_element(&decl_context->local_types, identifier);
        if(found && found->is_registered)
        {
            result = found;
            break;
        }
    }

    return(result);
}

#if 0
internal_api code_type_t*
is_type_within_declaration_context(declaration_context_t *context, string_t identifier)
{
    code_type_t *result = null;
    return(result);
}
#endif

// register code_type
internal_api code_type_t* 
parser_register_code_type_identifier(parser_t *parser, string_t identifier, code_type_t *type_alias = null)
{
    code_type_t *result = parser_search_for_code_type(parser, identifier);
    if(!result)
    {
        result = c_arena_push_struct(&parser->arena, code_type_t); 

        result->identifier    = identifier;
        result->alias_of      = type_alias;
        result->is_registered = true;

        hash_table_add_element(&parser->active_decl_context->local_types, &result, identifier);
    }

    return(result);
}
