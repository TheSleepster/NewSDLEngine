/* ========================================================================
   $File: athena_symbol_table.cpp $
   $Date: May 26 2026 04:52 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_symbol_table.h"

internal_api void
store_structure_AST(AST_node_t *node)
{
    Expect(node->node_type == AST_NODE_TYPE_STRUCTURE, 
           "Attempted to store an AST_node_t in the AST_Node structures table... However, the AST_node_t was not of type AST_NODE_TYPE_STRUCTURE and was instead: '%.*s'...\n",
           print_AST_node_type(node->node_type));

    TicketMutexScope(&g_symbol_table.AST_structures_mutex)
    {
        c_dynarray_push(g_symbol_table.structures, node);
    }
}

internal_api void
store_enum_AST(AST_node_t *node)
{
    Expect(node->node_type == AST_NODE_TYPE_ENUM, 
           "Attempted to store an AST_node_t in the AST_Node enums table... However, the AST_node_t was not of type AST_NODE_TYPE_ENUM and was instead: '%.*s'...\n",
           print_AST_node_type(node->node_type));

    TicketMutexScope(&g_symbol_table.AST_enums_mutex)
    {
        c_dynarray_push(g_symbol_table.enums, node);
    }
}

internal_api void
store_lambda_AST(AST_node_t *node)
{
    Expect(node->node_type == AST_NODE_TYPE_LAMBDA, 
           "Attempted to store an AST_node_t in the AST_Node lambdas table... However, the AST_node_t was not of type AST_NODE_TYPE_LAMBDA and was instead: '%.*s'...\n",
           print_AST_node_type(node->node_type));

    TicketMutexScope(&g_symbol_table.AST_lambdas_mutex)
    {
        c_dynarray_push(g_symbol_table.lambdas, node);
    }
}

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
            u64 lookup_hash = (c_combine_hashes(current_scope_id, type_ID) % SYMBOL_TABLE_SIZE);

            code_type_t *candidate = c_hash_table_get_value_ptr_at_index(&g_symbol_table.type_table, lookup_hash);
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
    u64 type_hash = (c_combine_hashes(scope_ID, type_ID) % SYMBOL_TABLE_SIZE);

    code_type_t *type = symbol_table_search_for_code_type(type_name);
    if(!type)
    {
        TicketMutexScope(&g_symbol_table.type_table_mutex)
        {
            type = c_hash_table_get_value_ptr_at_index(&g_symbol_table.type_table, type_hash);
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
    else if(type && type->code_metatype == CODE_TYPE_LAMBDA)
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

internal_api lexer_token_stream_t 
symbol_table_substitute_macro_arguments(lexer_t *lexer, lexer_token_t last_token, macro_info_t *macro_info)
{
    lexer_token_stream_t result = {};

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
                    report_error(lexer,
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
symbol_table_get_next_lexer_token(lexer_t *lexer)
{
    lexer_token_t result;
    result = lexer_get_next_token(lexer);
    if(result.token_type == TOKEN_TYPE_EOF)
    {
        lexer_pop_token_stream(lexer, false);
        result = lexer_get_next_token(lexer);
    }

    macro_info_t *macro = null;
    TicketMutexScope(&g_symbol_table.macro_table_mutex)
    {
        macro = c_hash_table_get_value_ptr(&g_symbol_table.macro_table, result.data);
        if(macro->is_set)
        {
            lexer_token_stream_t macro_stream = symbol_table_substitute_macro_arguments(lexer, result, macro);
            lexer_push_token_stream(lexer, &macro_stream);

            result = lexer_get_next_token(lexer);
            if(lexer->current_stream->string.count == 0)
            {
                lexer_pop_token_stream(lexer, false);
            }
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
