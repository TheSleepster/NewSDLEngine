/* ========================================================================
   $File: athena_symbol_table.cpp $
   $Date: May 26 2026 04:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_symbol_table.h"

#if 0
internal_api void
symbol_table_init(void)
{
    g_symbol_table.macro_table     = hash_table_create<macro_info_t>(SYMBOL_TABLE_SIZE);
    g_symbol_table.type_table      = hash_table_create<code_type_t>(SYMBOL_TABLE_SIZE);    
    g_symbol_table.constants_table = hash_table_create<AST_expression_value_t>(SYMBOL_TABLE_SIZE); 

    g_symbol_table.keywords   = c_dynarray_create(language_keyword_t);
    g_symbol_table.primitives = c_dynarray_create(code_type_t*);

    g_symbol_table.structures = c_dynarray_create(AST_node_t*);
    g_symbol_table.enums      = c_dynarray_create(AST_node_t*);
    g_symbol_table.lambdas    = c_dynarray_create(AST_node_t*);

    g_symbol_table.valid_type_table_indices = c_dynarray_create(u32);
    c_dynarray_reserve(g_symbol_table.valid_type_table_indices, 400);

    // NOTE(Sleepster): Default keywords 
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
    
    for(u32 index = 0;
        index < ArrayCount(default_keyword_strings);
        ++index)
    {
        language_keyword_t keyword = {};
        keyword.identifier         = c_string_make_copy(&permanent_arena, default_keyword_strings[index]);
        keyword.keyword_id         = default_keyword_enums[index];

        c_dynarray_push(g_symbol_table.keywords, keyword);
    }
    // NOTE(Sleepster): Default primitives 
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
        u64 type_id = (hash_table_hash_key(type_name) % SYMBOL_TABLE_SIZE);

        code_type_t *primitive   = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, type_id);
        primitive->is_registered = true;
        primitive->type_inferred = true;
        primitive->identifier    = c_string_make_copy(&permanent_arena, type_name);
        primitive->ID            = type_id;
        primitive->code_metatype = CODE_TYPE_PRIMITIVE;
        
        c_dynarray_push(g_symbol_table.primitives, primitive);
    }

    // NOTE(Sleepster): NULL 
    string_t null_name = STR("NULL");
    u64 hash = ((hash_table_hash_key(null_name)) % SYMBOL_TABLE_SIZE);

    macro_info_t *null_macro = hash_table_get_element_ptr(&g_symbol_table.macro_table, null_name);

    null_macro->name      = c_string_make_copy(&permanent_arena, null_name);
    null_macro->name_hash = hash;
    null_macro->expansion_string = STR("0");
    null_macro->expansion_token_stream = init_token_stream_from_string(null_macro->expansion_string);
    null_macro->is_set = true;

    // NOTE(Sleepster): Nullptr 
    string_t nullptr_name = STR("nullptr");
    hash = ((hash_table_hash_key(null_name)) % SYMBOL_TABLE_SIZE);

    macro_info_t *nullptr_macro = hash_table_get_element_ptr(&g_symbol_table.macro_table, nullptr_name);

    nullptr_macro->name      = c_string_make_copy(&permanent_arena, nullptr_name);
    nullptr_macro->name_hash = hash;
    nullptr_macro->expansion_string = STR("0");
    nullptr_macro->expansion_token_stream = init_token_stream_from_string(null_macro->expansion_string);
    nullptr_macro->is_set = true;
}

internal_api language_keyword_t*
symbol_table_get_keyword(string_t string)
{
    language_keyword_t *result = null;
    c_dynarray_for(g_symbol_table.keywords, keyword_index)
    {
        language_keyword_t *keyword = g_symbol_table.keywords + keyword_index;
        if(c_string_compare(keyword->identifier, string))
        {
            result = keyword;
            break;
        }
    }

    if(result == null)
    {
        // NOTE(Sleepster): Should be invalid 
        result = g_symbol_table.keywords;
        Expect(result->keyword_id == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}

// NOTE(Sleepster): This will search for the code type in the current scope, null if the type is not known to the type table... 
internal_api code_type_t*
symbol_table_search_for_code_type(string_t type_identifier)
{
    code_type_t *result = null;

    TicketMutexScope(&g_symbol_table.type_table_mutex)
    {
        u64 type_ID = type_id_from_identifier(type_identifier);

        // NOTE(Sleepster): 
        // This will work down the scope stack checking whatever our current namespace 
        // is up to global to find the registered type. 
        for(s32 stack_index = Max(thread_scope_stack.current_stack_depth - 1, 0);
            stack_index >= 0;
            --stack_index)
        {
            u64 current_scope_id = c_dynarray_get_value(thread_scope_stack.current_stack, (u32)stack_index);
            u64 lookup_hash = (hash_table_combine_hashes(current_scope_id, type_ID) % SYMBOL_TABLE_SIZE);

            code_type_t *candidate = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, lookup_hash);
            if(candidate && candidate->is_registered)
            {
                result = candidate;
                break; 
            }
        }
    }

    return(result);
}

// NOTE(Sleepster): This will add the type to the type_table in the current_scope 
internal_api code_type_t* 
symbol_table_register_typename(string_t type_name, u32 expected_metatype, u64 alias_id)
{
    u64 type_ID   = type_id_from_identifier(type_name);
    u64 scope_ID  = c_dynarray_get_value(thread_scope_stack.current_stack, (u32)thread_scope_stack.current_stack_depth); 
    u64 type_hash = (hash_table_combine_hashes(scope_ID, type_ID) % SYMBOL_TABLE_SIZE);

    code_type_t *type = symbol_table_search_for_code_type(type_name);
    if(!type)
    {
        TicketMutexScope(&g_symbol_table.type_table_mutex)
        {
            type = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, type_hash);
            Expect(type->is_registered == false, 
                   "When registering a type by name of: '%.*s' we expected the type to not already been registered, however this was not the case and the type's type->is_registered is true... meaning this is an invalid action...\n",
                   fprint_string(type_name));
        }

        type->identifier    = type_name;
        type->ID            = type_hash;
        type->alias_of      = alias_id;
        type->code_metatype = expected_metatype;
        type->scope_ID      = scope_ID;
        type->is_registered = true;
        type->type_inferred = false;
    }
    else if(type->code_metatype == CODE_TYPE_LAMBDA)
    {
        code_type_t *new_overload = c_arena_push_struct(&permanent_arena, code_type_t);
        if(!type->next_overload)
        {
            type->next_overload = new_overload;
        }
        else
        {
            for(code_type_t *current_overload = type->next_overload;
                current_overload;
                current_overload = current_overload->next_overload)
            {
                if(!current_overload->next_overload)
                {
                    current_overload->next_overload = new_overload;
                    break;
                }
            }
        }

        new_overload->identifier    = type_name;
        new_overload->ID            = type_hash;
        new_overload->alias_of      = alias_id;
        new_overload->code_metatype = expected_metatype;
        new_overload->scope_ID      = scope_ID;
        new_overload->is_registered = true;
        new_overload->type_inferred = false;

        type = new_overload;
    }

    return(type);
} 

internal_api void
symbol_table_infer_type(AST_node_t *node_data)
{
    code_type_t *type = symbol_table_search_for_code_type(node_data->identifier);
    Expect(type, "Failure to find any type for identifier: '%.*s'...\n", fprint_string(node_data->identifier))

    if(!type->type_inferred)
    {
        type->type_info_AST = node_data;
        switch(node_data->node_type)
        {
            case AST_NODE_TYPE_LAMBDA:    { type->code_metatype = CODE_TYPE_LAMBDA;    }break;
            case AST_NODE_TYPE_STRUCTURE: { type->code_metatype = CODE_TYPE_STRUCTURE; }break;
            case AST_NODE_TYPE_ENUM:      { type->code_metatype = CODE_TYPE_ENUM;      }break;
        }
    }
    else
    {
        Expect(false, 
               "We attempted to infer a type with an identifier of: '%.*s' however, this identifier has already been inferred, this is invalid behavior...\n",
               fprint_string(node_data->identifier));
    }
}

#endif

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

        result = lexer_copy_token_stream(&macro_info->expansion_token_stream);

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

// NOTE(Sleepster): SYMBOL TABLE

internal_api void
symbol_table_init(string_t filepath, bool8 recursive)
{
    table->file_count   = sys_directory_get_file_count(filepath, recursive);
    table->file_parsers = c_arena_push_array(&permanent_arena, parser_t, table->file_count); 

    table->defined_global_macro_table = hash_table_create<macro_info_t>(1024);
    table->defined_global_constants   = hash_table_create<AST_node_t *>(1024);

    table->is_initialized = true;
}

// PARSER
internal_api declaration_context_t*
parser_create_declaration_context(parser_t *parser, declaration_context_t *parent)
{
    declaration_context_t *result = null;

    declaration_context_t  new_context = {};
    new_context.local_types  = hash_table_create<code_type_t>(512);
    new_context.parent_scope = parent;

    result = dynarray_add(&parser->recorded_decl_contexts, &new_context);

    return(result);
}

internal_api parser_t* 
parser_create(string_t filename)
{
    u32 parser_index = AtomicIncrement32(&g_symbol_table.next_parser_index);
    parser_t *parser = g_symbol_table.file_parsers + parser_index;

    parser->arena    = c_arena_create(MB(1));
    parser->filename = c_string_make_copy(&parser->arena, filename);

    string_t file_data  = c_file_read_entirety(parser->filename, &parser->arena);
    parser->lexer       = lexer_create(file_data);

    parser->macro_table     = hash_table_create<macro_info_t>(1024);
    parser->constants_table = hash_table_create<AST_expression_value_t>(1024);

    declaration_context_t *global_scope = parser_create_declaration_context(parser, null);
    Expect(g_language_info.language_primitive_types.items != null,
           "Cannot initialize the parser without primtive type information... Make sure you call initialize_default_language_info() before you call this function!\n");

    // NOTE(Sleepster): Maybe this doesn't do what we think it does... references are weird.
    for(auto &primitive: g_language_info.language_primitive_types)
    {
        hash_table_add_element(&global_scope->local_types, 
                               primitive, 
                               primitive.identifier);
    }
        
    dynarray_add(&parser->decl_context_stack,     global_scope);
    dynarray_add(&parser->recorded_decl_contexts, global_scope);

    parser->active_decl_context = parser->decl_context_stack.items;

    return(parser);
}

internal_api void
parser_push_decl_context(parser_t *parser, declaration_context_t *context)
{
    dynarray_add(&parser->decl_context_stack, context);
    dynarray_add_if_unique(&parser->recorded_decl_contexts, context);

    parser->active_decl_context = dynarray_get_ptr_at_index(&parser->decl_context_stack, parser->decl_context_stack.used - 1);
}

internal_api void
parser_pop_decl_context(parser_t *parser)
{
    if(parser->decl_context_stack.used - 1 <= 0)
    {
        report_error(parser,
                     "We attempted to pop the decl_context stack on the parser for file: '%.*s' however this would pop the top level file context as well... This should not happen as the file context must constantly be at index 1...\n",
                     fprint_string(parser->filename));
    }

    dynarray_pop(&parser->decl_context_stack);
    parser->active_decl_context = dynarray_get_ptr_at_index(&parser->decl_context_stack, parser->decl_context_stack.used);
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

internal_api language_keyword_t*
get_keyword_from_identifier(string_t identifier)
{
    language_keyword_t *result = null;
    for(auto &keyword: g_language_info.keywords)
    {
        if(c_string_compare(keyword.identifier, identifier))
        {
            result = &keyword;
            break;
        }
    }

    if(result == null)
    {
        // NOTE(Sleepster): First element should be invalid 
        result = g_language_info.keywords.items;
        Expect(result->keyword_id == TOKEN_KEYWORD_INVALID, "Default keyword is not invalid... this is fatal...\n");
    }

    return(result);
}

internal_api lexer_token_t
parser_get_next_lexer_token(parser_t *parser)
{
    lexer_t *lexer = &parser->lexer;

    lexer_token_t result;
    result = lexer_get_next_token(lexer);
    if(result.token_type == TOKEN_TYPE_EOF)
    {
        lexer_pop_token_stream(lexer, false);
        result = lexer_get_next_token(lexer);
    }

    // macro_info_t *macro = hash_table_get_element_ptr(&g_symbol_table.macro_table, result.data);
    macro_info_t *macro = hash_table_get_element_ptr(&parser->macro_table, result.data);
    if(macro->is_set)
    {
        lexer_token_stream_t macro_stream = parser_substitute_macro_arguments(parser, result, macro);
        lexer_push_token_stream(lexer, &macro_stream);

        result = lexer_get_next_token(lexer);
        if(lexer->current_stream->string.count == 0)
        {
            lexer_pop_token_stream(lexer, false);
        }
    }
    
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
    for(u32 index = parser->decl_context_stack.used - 1;
        index > 0;
        --index)
    {
        declaration_context_t *decl_context = dynarray_get_ptr_at_index(&parser->decl_context_stack, index);
        code_type_t *found = hash_table_get_element_ptr(&decl_context->local_types, identifier);
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
parser_register_code_type_identifier(parser_t *parser, string_t identifier, u64 alias_id = -1)
{
    code_type_t *result = parser_search_for_code_type(parser, identifier);
    if(!result)
    {
        result = hash_table_get_element_ptr(&parser->active_decl_context->local_types, identifier);

        result->identifier    = c_string_make_copy(&parser->arena, identifier);
        result->alias_of      = alias_id;
        result->is_registered = true;
    }

    return(result);
}

// infer code_type
internal_api void
parser_infer_type(parser_t *parser)
{
}
