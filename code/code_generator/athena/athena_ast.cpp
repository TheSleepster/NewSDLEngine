/* ========================================================================
   $File: athena_ast.cpp $
   $Date: May 30 2026 12:10 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_ast.h"

internal_api char*
print_AST_node_type(u32 node_type)
{
    switch(node_type)
    {
#define X(enum, string) case enum: {return(string);}break;
        AST_NODE_TYPE_LIST(X)
#undef X

        default: {return("null");}break;
    }
}

internal_api true_inline AST_node_t*
AST_create_new_node(memory_arena_t *arena)
{
    AST_node_t *result = c_arena_push_struct(arena, AST_node_t);
    ZeroStruct(*result);

    return(result);
}

internal_api true_inline void
AST_add_member(AST_node_t *parent, AST_node_t *next_member)
{
    if(parent->struct_decl.first_member == null)
    {
        parent->struct_decl.first_member = next_member;
    }
    else
    {
        for(AST_node_t *current_member = parent->struct_decl.first_member;
            current_member;
            current_member = current_member->next_sibling)
        {
            if(!current_member->next_sibling)
            {
                current_member->next_sibling = next_member;
                break;
            }
        }
    }
}

internal_api true_inline void
AST_add_sibling(AST_node_t *sibling, AST_node_t *new_sibling)
{
    for(AST_node_t *current_sibling = sibling;
        current_sibling;
        current_sibling = current_sibling->next_sibling)
    {
        if(!current_sibling->next_sibling)
        {
            current_sibling->next_sibling = new_sibling;
            break;
        }
    }
}

internal_api void
build_number_AST_node(lexer_t *lexer, AST_node_t *value_expression, lexer_token_t token)
{
    bool8 signed_value = false;
    bool8 is_float     = false;

    lexer_token_t value_token = token;
    if(token.token_type == TOKEN_TYPE_DASH)
    {
        signed_value = true;
        symbol_table_get_next_lexer_token(lexer);
    }

    s32 decimal_index = c_string_find_first_char_from_left(token.data, '.');
    if(decimal_index == -1)
    {
        lexer_token_t float_token = lexer_peek_token(lexer, 1);
        if(float_token.data.count == 1 && float_token.data.data[0] == 'f')
        {
            is_float = true;
        }
    }
    else
    {
        is_float = true;
    }

    value_expression->node_type    = AST_NODE_TYPE_NUMBER;
    value_expression->type.literal = c_string_make_copy(&permanent_arena, value_token.data); 
    if(signed_value && !is_float)
    {
        value_expression->type.code_type = symbol_table_search_for_code_type(STR("int64_t"));
        value_expression->type.int_value = c_string_read_int(value_token.data); 

        value_expression->type.flags |= AST_TYPE_MODIFIER_FLAG_SIGNED;
    }
    else if(!signed_value && !is_float)
    {
        value_expression->type.code_type      = symbol_table_search_for_code_type(STR("uint64_t"));
        value_expression->type.unsigned_value = c_string_read_uint(value_token.data); 
    }
    else if(is_float)
    {
        bool8 half_float = false;
        lexer_token_t float_token = lexer_peek_token(lexer, 1);
        if(float_token.data.count == 1 && float_token.data.data[0] == 'f')
        {
            symbol_table_get_next_lexer_token(lexer);
            half_float = true;
        }

        if(half_float)
        {
            value_expression->type.code_type     = symbol_table_search_for_code_type(STR("float"));
            value_expression->type.float32_value = c_string_read_float32(value_token.data); 
        }
        else
        {
            value_expression->type.code_type     = symbol_table_search_for_code_type(STR("double"));
            value_expression->type.float64_value = c_string_read_float64(value_token.data); 
        }
        value_expression->type.flags |= (AST_TYPE_MODIFIER_FLAG_FLOAT | AST_TYPE_MODIFIER_FLAG_SIGNED);
    }
    else
    {
        report_error(lexer, 
                     "Expected to parse a number when building the number AST node... Failed to find that... Instead found: '%.*s'...\n", 
                     fprint_token(token));
    }
}

internal_api u32
get_infix_binding_power(lexer_token_t *token)
{
    u32 result = 0;
    switch(token->token_type)
    {
        // NOTE(Sleepster): Bitshift left is the only special infix operator... (not '&', '|', etc.)
        case TOKEN_TYPE_BITSHIFT_LEFT:  {result = 5;}break;
        case TOKEN_TYPE_BITSHIFT_RIGHT: {result = 5;}break;

        case TOKEN_TYPE_PLUS:
        case TOKEN_TYPE_DASH:          {result = 10;}break;

        case TOKEN_TYPE_FORWARD_SLASH:
        case TOKEN_TYPE_ASTERISK:      {result = 20;}break;
        default: {result = 0;}break;
    };

    return(result);
}

internal_api s32
get_prefix_binding_power(lexer_token_t *token)
{
    s32 result = -1;
    switch(token->token_type)
    {
        case TOKEN_TYPE_DASH: {result = 25;}break;
    }

    return(result);
}

internal_api AST_node_t*
generate_unary_expression_AST(lexer_t *lexer, lexer_token_t *token, AST_node_t *unary_operation)
{
    AST_node_t *result = AST_create_new_node(&permanent_arena);
    result->node_type  = AST_NODE_TYPE_UNARY_EXPRESSION;
    result->unary_expression.operator_type = token->token_type;
    result->unary_expression.operand       = unary_operation;

    return(result);
}

internal_api AST_node_t*
generate_binary_expression_AST(lexer_token_t *operator_token, AST_node_t *left_node, AST_node_t *right_node)
{
    AST_node_t *result = AST_create_new_node(&permanent_arena);
    result->node_type  = AST_NODE_TYPE_BINARY_EXPRESSION;
    result->binary_expression.operator_type = operator_token->token_type;
    result->binary_expression.left          = left_node;
    result->binary_expression.right         = right_node;

    return(result);
}

internal_api AST_node_t*
generate_nud_prefix_AST(lexer_t *lexer, lexer_token_t *token)
{
    AST_node_t *result = null;

    s32 prefix_value = get_prefix_binding_power(token);
    if(prefix_value == -1)
    {
        // NOTE(Sleepster): If this is not a prefix value (like '-') 
        switch(token->token_type)
        {
            case TOKEN_TYPE_NUMBER:
            {
                result = AST_create_new_node(&permanent_arena);
                build_number_AST_node(lexer, result, *token);
            }break;
            // NOTE(Sleepster): For macros, this will never happen since the symbol table simply substitues the macro expansion in place. 
            case TOKEN_TYPE_IDENT:
            {
                AST_expression_value_t *constant = hash_table_get_element_ptr(&g_symbol_table.constants_table, token->data);
                if(constant->type != AST_EXPRESSION_VALUE_INVALID)
                {
                    result = AST_create_new_node(&permanent_arena);
                    result->node_type  = AST_NODE_TYPE_NUMBER;
                    result->identifier = c_string_make_copy(&permanent_arena, token->data); 
                    switch(constant->type)
                    {
                        case AST_EXPRESSION_VALUE_INT:
                        case AST_EXPRESSION_VALUE_UNSIGNED:
                        case AST_EXPRESSION_VALUE_FLOAT:
                        case AST_EXPRESSION_VALUE_DOUBLE:
                        {
                            result->type.literal = constant->identifier_value;
                            memcpy(&result->type.float64_value, &constant->float64_value, sizeof(double));

                            printf("Found constexpr: '%.*s'...\n", token->data.count, token->data.data);
                        }break;
                        case AST_EXPRESSION_VALUE_LITERAL:
                        {
                            result = AST_create_new_node(&permanent_arena);
                            result->node_type         = AST_NODE_TYPE_LITERAL;
                            result->type.string_value = c_string_make_copy(&permanent_arena, token->data);

                            printf("String literal: '%.*s' found when generating an expression...\n", token->data.count, token->data.data);
                        }break;
                        default: { InvalidCodePath; } break;
                    }
                }
                else
                {
                    printf("Wanted to input parsing data for constant: '%.*s' however, this constant is not valid yet...\n", fprint_string(token->data));
                }
            }break;
            case TOKEN_TYPE_LITERAL:
            {
                result = AST_create_new_node(&permanent_arena);
                result->node_type         = AST_NODE_TYPE_LITERAL;
                result->type.string_value = c_string_make_copy(&permanent_arena, token->data);

                printf("String literal: '%.*s' found when generating an expression...\n", token->data.count, token->data.data);
            }break;
            case TOKEN_TYPE_OPEN_PAREN:
            {
                // NOTE(Sleepster): Subexpression 
                result = generate_expression_AST(lexer, 0, token);
            }break;
            default: 
            {
                report_error(lexer, 
                             "Expected either an identifier, a number, or another expression when generating the prefix AST... Instead found: '%.*s' token type: '%.*s'...\n",
                             token->data.count, token->data.data, lexer_token_type_to_string(token));
            }break;
        }
    }
    else
    {
        AST_node_t *unary_operation = generate_expression_AST(lexer, prefix_value, token);
        result = generate_unary_expression_AST(lexer, token, unary_operation);
    }

    return(result);
}

internal_api AST_node_t*
generate_led_AST(lexer_t *lexer, lexer_token_t *token, AST_node_t *left_hand_expression)
{
    u32 current_infix_binding_power = get_infix_binding_power(token);

    lexer_token_t operator_token = *token;

    AST_node_t *right_expression  = generate_expression_AST(lexer, current_infix_binding_power, token);
    AST_node_t *binary_expression = generate_binary_expression_AST(&operator_token, left_hand_expression, right_expression);
    return(binary_expression);
}


internal_api AST_node_t* 
generate_expression_AST(lexer_t *lexer, s32 expression_min_binding_power, lexer_token_t *token_out)
{
    AST_node_t *value_expression = null;

    // NOTE(Sleepster): Is the next token a number? 
    lexer_token_t token       = symbol_table_get_next_lexer_token(lexer);
    AST_node_t *left_hand_AST = generate_nud_prefix_AST(lexer, &token);

    token = symbol_table_get_next_lexer_token(lexer);
    for(;;)
    {
        // NOTE(Sleepster): Measure the current left expression's binding power if it beats our fence, fold it in. Otherwise, ignore it.
        s32 current_left_expression_binding_power = get_infix_binding_power(&token);

        if(current_left_expression_binding_power >= expression_min_binding_power && 
           current_left_expression_binding_power != 0)
        {
            left_hand_AST = generate_led_AST(lexer, &token, left_hand_AST);
        }
        else
        {
            break;
        }
    }

    if(token_out)
    {
        *token_out = token;
    }

    value_expression = left_hand_AST;
    return(value_expression);
}

internal_api AST_expression_value_t
case_expression_value(AST_expression_value_t value, u32 target_type)
{
    AST_expression_value_t result = {};
    if(value.type != target_type)
    {
        float64 intermediate = 0.0;
        switch(target_type)
        {
            case AST_EXPRESSION_VALUE_INT:      { intermediate = (float64)value.int_value;      }break;
            case AST_EXPRESSION_VALUE_UNSIGNED: { intermediate = (float64)value.unsigned_value; }break;
            case AST_EXPRESSION_VALUE_FLOAT:    { intermediate = (float64)value.float32_value;  }break;
            case AST_EXPRESSION_VALUE_DOUBLE:   { intermediate = value.float64_value;           }break;
        }

        result = {target_type};
        switch(target_type)
        {
            case AST_EXPRESSION_VALUE_INT:      { result.int_value = (s32)intermediate;     }break; 
            case AST_EXPRESSION_VALUE_UNSIGNED: { result.int_value = (u32)intermediate;     }break; 
            case AST_EXPRESSION_VALUE_FLOAT:    { result.int_value = (float32)intermediate; }break; 
            case AST_EXPRESSION_VALUE_DOUBLE:   { result.int_value = (float64)intermediate; }break; 
        }
    }
    else
    {
        result = value;
    }

    return(result);
}

internal_api AST_expression_value_t
evaluate_binary_expression(u32 operator_type, AST_expression_value_t left, AST_expression_value_t right)
{
    AST_expression_value_t value = {};
    u32 common = Max(left.type, right.type);

    value.type = common;

    bool is_shift_operation = (operator_type == TOKEN_TYPE_BITSHIFT_LEFT || operator_type == TOKEN_TYPE_BITSHIFT_RIGHT);
    Expect(!is_shift_operation || common <= AST_EXPRESSION_VALUE_FLOAT,
           "Bit-shift applied to floating-point operands\n");

    left  = case_expression_value(left,  common);
    right = case_expression_value(right, common);
    switch(common)
    {
        case AST_EXPRESSION_VALUE_INT:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           value.int_value = left.int_value + right.int_value; break;
                case TOKEN_TYPE_DASH:           value.int_value = left.int_value - right.int_value; break;
                case TOKEN_TYPE_ASTERISK:       value.int_value = left.int_value * right.int_value; break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.int_value != 0, "Division by zero\n");
                    value.int_value = left.int_value / right.int_value;
                } break;
                case TOKEN_TYPE_BITSHIFT_LEFT:  value.int_value = (s32)((u32)left.int_value << right.int_value); break;
                case TOKEN_TYPE_BITSHIFT_RIGHT: value.int_value = left.int_value >> right.int_value;             break;
                default: Expect(false, "Unknown operator for s32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_UNSIGNED:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           value.unsigned_value = left.unsigned_value + right.unsigned_value; break;
                case TOKEN_TYPE_DASH:           value.unsigned_value = left.unsigned_value - right.unsigned_value; break;
                case TOKEN_TYPE_ASTERISK:       value.unsigned_value = left.unsigned_value * right.unsigned_value; break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.unsigned_value != 0, "Division by zero\n");
                    value.unsigned_value = left.unsigned_value / right.unsigned_value;
                } break;
                case TOKEN_TYPE_BITSHIFT_LEFT:  value.unsigned_value = left.unsigned_value << right.unsigned_value; break;
                case TOKEN_TYPE_BITSHIFT_RIGHT: value.unsigned_value = left.unsigned_value >> right.unsigned_value; break;


                // TODO(Sleepster): 
                // The issue here is very simple, for some reason the final character of an expression 
                // (whether it be a comma, or a close paren, or literally anything) gets added to the expression's AST. This is bad.
                case TOKEN_TYPE_CLOSE_PAREN: {Expect(false, "Bro what the hell bro...\n"); }break;
                default: Expect(false, "Unknown operator for u32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_FLOAT:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           value.float32_value = left.float32_value + right.float32_value; break;
                case TOKEN_TYPE_DASH:           value.float32_value = left.float32_value - right.float32_value; break;
                case TOKEN_TYPE_ASTERISK:       value.float32_value = left.float32_value * right.float32_value; break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.float32_value != 0.0f, "Division by zero\n");
                    value.float32_value = left.float32_value / right.float32_value;
                } break;
                default: Expect(false, "Unknown operator for f32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_DOUBLE:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           value.float64_value = left.float64_value + right.float64_value; break;
                case TOKEN_TYPE_DASH:           value.float64_value = left.float64_value - right.float64_value; break;
                case TOKEN_TYPE_ASTERISK:       value.float64_value = left.float64_value * right.float64_value; break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.float64_value != 0.0, "Division by zero\n");
                    value.float64_value = left.float64_value / right.float64_value;
                } break;
                default: Expect(false, "Unknown operator for f64\n"); break;
            }
        }break;
    }

    return(value);
}

internal_api AST_expression_value_t 
evaluate_expression_AST(AST_node_t *expression)
{
    AST_expression_value_t result = {};

    AST_type_t *type = &expression->type;
    switch(expression->node_type)
    {
        case AST_NODE_TYPE_LITERAL:
        {
            result.type             = AST_EXPRESSION_VALUE_LITERAL;
            result.identifier_value = type->string_value;
        }break;
        case AST_NODE_TYPE_NUMBER:
        {
            if(type->flags & AST_TYPE_MODIFIER_FLAG_SIGNED)
            {
                if(type->flags & AST_TYPE_MODIFIER_FLAG_FLOAT)
                {
                    if(c_string_compare(type->code_type->identifier, STR("float")))
                    {
                        result.type          = AST_EXPRESSION_VALUE_FLOAT;
                        result.float32_value = type->float32_value;
                    }
                    else
                    {
                        result.type          = AST_EXPRESSION_VALUE_DOUBLE;
                        result.float64_value = type->float64_value;
                    }
                }
                else
                {
                    result.type      = AST_EXPRESSION_VALUE_INT;
                    result.int_value = type->int_value;
                }
            }
            else
            {
                result.type           = AST_EXPRESSION_VALUE_UNSIGNED;
                result.unsigned_value = type->unsigned_value;
            }
        }break;
        case AST_NODE_TYPE_UNARY_EXPRESSION:
        {
            AST_expression_value_t value = evaluate_expression_AST(expression->unary_expression.operand);
            switch(expression->unary_expression.operator_type)
            {
                case TOKEN_TYPE_DASH:
                {
                    switch(value.type)
                    {
                        case AST_EXPRESSION_VALUE_INT:
                        {
                            value.int_value = -value.int_value;
                        }break;
                        case AST_EXPRESSION_VALUE_UNSIGNED:
                        {
                            value.unsigned_value = -value.unsigned_value;
                        }break;
                        case AST_EXPRESSION_VALUE_FLOAT:
                        {
                            value.float32_value = -value.float32_value;
                        }break;
                        case AST_EXPRESSION_VALUE_DOUBLE:
                        {
                            value.float64_value = -value.float64_value;
                        }break;
                    }
                }break;
                default: { InvalidCodePath; }break;
            }
        }break;
        case AST_NODE_TYPE_BINARY_EXPRESSION:
        {
            AST_expression_value_t left  = evaluate_expression_AST(expression->binary_expression.left);
            AST_expression_value_t right = evaluate_expression_AST(expression->binary_expression.right);

            result = evaluate_binary_expression(expression->binary_expression.operator_type, left, right);
        }break;
        default: { Expect(false, "Node_type is invalid: '%s'...\n", print_AST_node_type(expression->node_type)); }break;
    }

    return(result);
}

internal_api void
parse_lambda_argument_list(lexer_t *lexer, AST_node_t *lambda)
{
    lexer_token_t token = symbol_table_get_next_lexer_token(lexer);
    do {
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            u32 argument_flags = 0;

            language_keyword_t *keyword = symbol_table_get_keyword(token.data);
            if(keyword->keyword_id == TOKEN_KEYWORD_CONST)
            {
                token = symbol_table_get_next_lexer_token(lexer);
                argument_flags |= AST_TYPE_MODIFIER_FLAG_CONST;
            }

            if(keyword->keyword_id == TOKEN_KEYWORD_VOLATILE)
            {
                token = symbol_table_get_next_lexer_token(lexer);
                argument_flags |= AST_TYPE_MODIFIER_FLAG_VOLATILE;
            }

            // NOTE(Sleepster): Generate an AST_node_t for each of the function's arugments;
            lexer_token_t argument_typename = token;

            // TODO(Sleepster): handle other cases like namespace::type
            //
            // Also allow for the usage of combining hashes for overloads. Such as:
            //
            // line 13      void get_item(string_t item_name);
            //
            // line 219     void get_item(u64 ID);
            //
            // The above should be 2 different types. This can be achieved through many methods. Just do it.

            // TODO(Sleepster):   
            // We also need to handle namespaced types
            //
            // void item_manager::get_items(string_t item_name);
            //
            // void get_items(string_t item_name);
            //
            // should be different as well..
            if(argument_typename.token_type == TOKEN_TYPE_IDENT || 
               argument_typename.token_type == TOKEN_TYPE_NAMESPACE || 
               argument_typename.token_type == TOKEN_TYPE_ASTERISK)
            {
                lexer_token_t argument_name = symbol_table_get_next_lexer_token(lexer);

                u32 pointer_depth = 0;
                while(argument_name.token_type == TOKEN_TYPE_ASTERISK)
                {
                    argument_name = symbol_table_get_next_lexer_token(lexer);
                    ++pointer_depth;
                }

                if(argument_name.token_type != TOKEN_TYPE_IDENT && 
                   c_string_compare(argument_typename.data, STR("void")))
                {
                    // NOTE(Sleepster): 
                    // This is a declaration like:
                    //
                    // int random_function(void);
                    //
                    // therefore there are no arguments
                    break;
                }

                if(argument_name.token_type != TOKEN_TYPE_IDENT)
                {
                    report_error(lexer,
                                 "Expected the arugment_name token to be a valid identifier... Instead it was: '%.*s'...\n",
                                 fprint_token(argument_name));
                }

                AST_node_t *argument = AST_create_new_node(&permanent_arena);
                argument->node_type  = AST_NODE_TYPE_LAMBDA_ARGUMENT;
                argument->identifier = c_string_make_copy(&permanent_arena, argument_name.data);

                AST_type_t *type_data = &argument->type;
                type_data->code_type  = symbol_table_search_for_code_type(argument_typename.data);
                if(!type_data->code_type)
                {
                    type_data->code_type = symbol_table_register_typename(argument_typename.data, CODE_TYPE_LAMBDA);
                }

                type_data->literal       = c_string_make_copy(&permanent_arena, argument_typename.data);
                type_data->flags         = pointer_depth > 0 ? (argument_flags | AST_TYPE_MODIFIER_FLAG_POINTER) : argument_flags;
                type_data->pointer_depth = pointer_depth;
                if(lambda->lambda.first_argument != null)
                {
                    for(AST_node_t *current_argument = lambda->lambda.first_argument;
                        current_argument;
                        current_argument = current_argument->next_sibling)
                    {
                        if(current_argument->next_sibling == null)
                        {
                            current_argument->next_sibling = argument;
                            break;
                        }
                    }
                }
                else
                {
                    lambda->lambda.first_argument = argument;
                }

                ++lambda->lambda.argument_count;

                token = symbol_table_get_next_lexer_token(lexer);
                if(token.token_type == TOKEN_TYPE_EQUALS)
                {
                    argument->expression.info = generate_expression_AST(lexer, 0, &token);
                }

                if(token.token_type == TOKEN_TYPE_CLOSE_PAREN)
                {
                    break;
                }
            }
        }
        else if(token.token_type == TOKEN_TYPE_NUMBER)
        {
            // NOTE(Sleepster): Exit if we find a number, if we find one this is likely a constructor. 
            break;
        }

        token = symbol_table_get_next_lexer_token(lexer);
    }while(token.token_type != TOKEN_TYPE_CLOSE_PAREN);
}

internal_api AST_node_t* 
generate_structure_AST(lexer_t *lexer)
{
    AST_node_t *result  = null;
    u32 structure_flags = 0;

    lexer_token_t name_token = symbol_table_get_next_lexer_token(lexer);

    u64 struct_ID = INVALID_ID;

    lexer_token_t token;
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        if(name_token.token_type != TOKEN_TYPE_IDENT) 
        {
            report_error(lexer,
                         "Expected to find the name of the structure after the 'struct' keyword, failed to find that... instead found: '%.*s'...\n",
                         fprint_token(name_token));
        }

        code_type_t *struct_data = symbol_table_register_typename(name_token.data, CODE_TYPE_STRUCTURE);
        printf("Registered structure type: '%.*s'...\n", fprint_string(name_token.data));

        struct_ID = struct_data->ID;
        token     = symbol_table_get_next_lexer_token(lexer);
    }
    else if(name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        token = name_token;
        structure_flags = AST_TYPE_MODIFIER_FLAG_ANONYMOUS;
    }

    // NOTE(Sleepster): Inheritance 
    AST_node_t *inheritance_node = null;
    if(token.token_type == TOKEN_TYPE_COLON)
    {
        lexer_token_t inheritance_publicity_token = symbol_table_get_next_lexer_token(lexer);
        if(inheritance_publicity_token.token_type != TOKEN_TYPE_PRIVATE && inheritance_publicity_token.token_type != TOKEN_TYPE_PUBLIC)
        {
            report_error(lexer,
                         "Expected there to be a public/private denotion on the inherited structure, instead found: '%.*s'...\n",
                         fprint_token(inheritance_publicity_token));
        }

        lexer_token_t inherited_typename = symbol_table_get_next_lexer_token(lexer);
        if(inherited_typename.token_type != TOKEN_TYPE_IDENT)
        {
            report_error(lexer,
                         "Expected there to be a typename for the inherited structure, instead found: '%.*s'...\n",
                         fprint_token(inherited_typename));
        }

        inheritance_node            = AST_create_new_node(&permanent_arena);
        inheritance_node->node_type = AST_NODE_TYPE_INHERITANCE_INFO;

        code_type_t *inherited_type = symbol_table_search_for_code_type(inherited_typename.data);
        if(!inherited_type)
        {
            inherited_type = symbol_table_register_typename(inherited_typename.data, CODE_TYPE_STRUCTURE);
            printf("Registered structure type: '%.*s'...\n", fprint_string(inherited_typename.data));
        }

        inheritance_node->type.code_type = inherited_type;
        inheritance_node->inheritance_info.inheritance_type = inheritance_publicity_token.token_type;
        inheritance_node->inheritance_info.inherited_data   = inherited_type->type_info_AST;

        // NOTE(Sleepster): Eat the token right before the open brace 
        token = symbol_table_get_next_lexer_token(lexer);
    }
    else if(token.token_type == TOKEN_TYPE_IDENT)
    {
        // NOTE(Sleepster): This was a forward declaration! 
        return(result);
    }

    if(token.token_type != TOKEN_TYPE_OPEN_BRACE && token.token_type != TOKEN_TYPE_SEMICOLON)
    {
        report_error(lexer, 
                     "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
                     fprint_token(name_token), fprint_token(token));
    }

    // NOTE(Sleepster): If it's a brace, it's a definition... otherwise it's just a declaration and we don't care... 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        AST_node_t *structure_root = AST_create_new_node(&permanent_arena);
        structure_root->node_type  = AST_NODE_TYPE_STRUCTURE;
        structure_root->type.flags = structure_flags;
        if(inheritance_node)
        {
            structure_root->struct_decl.inherited_type_info = inheritance_node;
        }

        push_scope_stack(name_token.data);
        defer(pop_scope_stack());

        if(struct_ID != INVALID_ID)
        {
            code_type_t *type = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, struct_ID);
            if(!type->type_info_AST)
            {
                type->type_info_AST = structure_root;
            }

            structure_root->type.code_type = type;
        }

        if(name_token.token_type == TOKEN_TYPE_IDENT) structure_root->identifier = c_string_make_copy(&permanent_arena, name_token.data);
        else                                          structure_root->identifier = STR("anonymous");

        for(;;)
        {
            lexer_token_t typename_token = symbol_table_get_next_lexer_token(lexer);
            if(typename_token.token_type == TOKEN_TYPE_CLOSE_BRACE) break;

            // NOTE(Sleepster): Handle type modifier (const or volatile)
            u32 type_modifier_flags = 0;
            language_keyword_t *keyword = symbol_table_get_keyword(typename_token.data);

            bool8 is_valid_member = true;
            while(keyword->keyword_id != TOKEN_KEYWORD_INVALID &&
                  keyword->keyword_id != TOKEN_KEYWORD_STRUCT  &&
                  keyword->keyword_id != TOKEN_KEYWORD_UNION) 
            {
                if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
                {
                    if(keyword->keyword_id == TOKEN_KEYWORD_VOLATILE)
                    {
                        type_modifier_flags |= AST_TYPE_MODIFIER_FLAG_VOLATILE;
                    }
                    if(keyword->keyword_id == TOKEN_KEYWORD_CONST)
                    {
                        type_modifier_flags |= AST_TYPE_MODIFIER_FLAG_CONST;
                    }

                    if(keyword->keyword_id == TOKEN_KEYWORD_PUBLIC  ||
                       keyword->keyword_id == TOKEN_KEYWORD_PRIVATE ||
                       keyword->keyword_id == TOKEN_KEYWORD_PROTECTED)
                    {
                        is_valid_member = false;
                    }

                    typename_token = symbol_table_get_next_lexer_token(lexer);
                }

                keyword = symbol_table_get_keyword(typename_token.data);
            }

            if(is_valid_member)
            {
                lexer_token_t member_name_token = lexer_peek_token(lexer);
                if(member_name_token.token_type == TOKEN_TYPE_OPEN_PAREN || 
                   typename_token.token_type == TOKEN_TYPE_TILDE)
                {
                    // NOTE(Sleepster): Constructor / Deconstructor 
#if 1
                    lexer_token_t end_token = symbol_table_get_next_lexer_token(lexer);
                    if(end_token.token_type != TOKEN_TYPE_SEMICOLON && end_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                    {
                        while(end_token.token_type != TOKEN_TYPE_SEMICOLON && end_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                        {
                            end_token = symbol_table_get_next_lexer_token(lexer);
                        }
                    }

                    if(end_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                    {
                        while(end_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                        {
                            end_token = symbol_table_get_next_lexer_token(lexer);
                        }
                    }
#else
                    AST_node_t *constructor_node = AST_create_new_node(&permanent_arena);
                    if(typename_token.token_type == TOKEN_TYPE_TILDE)
                    {
                        constructor_node->node_type = AST_NODE_TYPE_DECONSTRUCTOR;
                        typename_token = symbol_table_get_next_lexer_token(lexer);
                    }
                    else
                    {
                        constructor_node->node_type = AST_NODE_TYPE_CONSTRUCTOR;
                    }

                    constructor_node->identifier = c_string_make_copy(&permanent_arena, typename_token.data);
                    parse_lambda_argument_list(lexer, constructor_node);

                    // NOTE(Sleepster): Register the constructor / deconstructor 
                    AST_type_t *type_data = &constructor_node->type;
                    type_data->code_type  = symbol_table_search_for_code_type(typename_token.data);

                    u64 current_scope_ID = c_dynarray_get_value(thread_scope_stack.current_stack, (u32)thread_scope_stack.current_stack_depth - 1);
                    if(type_data->code_type->scope_ID != current_scope_ID)
                    {
                        type_data->code_type = symbol_table_register_typename(typename_token.data, CODE_TYPE_LAMBDA);
                    }

                    lexer_token_t end_token = symbol_table_get_next_lexer_token(lexer);
                    if(end_token.token_type != TOKEN_TYPE_SEMICOLON && end_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                    {
                        report_error(lexer, 
                                     "Expected to find either a semicolon to denote the end of this declaration or an open brace to begin the definition, instead found: '%.*s'...\n",
                                     fprint_token(end_token));
                    }

                    if(end_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                    {
                        while(end_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                        {
                            end_token = symbol_table_get_next_lexer_token(lexer);
                        }
                    }

                    // TODO(Sleepster): We do not infer the type of constructors or desconstructors as they are special 
                    constructor_node->type.code_type->type_info_AST = constructor_node;
                    constructor_node->type.code_type->code_metatype = CODE_TYPE_LAMBDA;
#endif
                }
                else
                {
                    // NOTE(Sleepster): Handle member pointer 
                    u32 pointer_depth = 0;
                    if(member_name_token.token_type == TOKEN_TYPE_ASTERISK)
                    {
                        do {
                            member_name_token = lexer_peek_token(lexer, pointer_depth + 2);
                            ++pointer_depth;
                        }while(member_name_token.token_type == TOKEN_TYPE_ASTERISK);
                    }

                    // NOTE(Sleepster): This assert is disabled for anonymous structures...
                    // Expect(member_name_token.token_type == TOKEN_TYPE_IDENT, 
                    //        "Expected to find a member name in this location... Instead found: '%.*s'...\n",
                    //        fprint_token(member_name_token));

                    lexer_token_t peek_token = lexer_peek_token(lexer, pointer_depth + 2);
                    if(peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                    {
                        // usual lambda
                        AST_node_t *member_node = generate_lambda_AST(lexer, 
                                                                      typename_token, 
                                                                      pointer_depth, 
                                                                      (type_modifier_flags & AST_TYPE_MODIFIER_FLAG_CONST) ? true : false);
                        member_node->identifier = member_node->type.code_type->identifier;
                        AST_add_member(structure_root, member_node);

                        peek_token = lexer_peek_token(lexer);
                        if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                        {
                            while(peek_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                            {
                                peek_token = symbol_table_get_next_lexer_token(lexer);
                            }
                        }
                        else if(peek_token.token_type == TOKEN_TYPE_SEMICOLON)
                        {
                            symbol_table_get_next_lexer_token(lexer);
                        }
                        else
                        {
                            report_error(lexer,
                                         "Failure to parse expression after struct member lambda... Found: '%.*s' which is an invalid token...\n",
                                         fprint_token(peek_token));
                        }
                    }
                    else if(typename_token.token_type == TOKEN_TYPE_IDENT)
                    {
                        symbol_table_get_next_lexer_token(lexer);

                        for(u32 pointer_index = 0;
                            pointer_index < pointer_depth;
                            ++pointer_index)
                        {
                            symbol_table_get_next_lexer_token(lexer);
                        }

                        AST_node_t *member_node = AST_create_new_node(&permanent_arena);
                        member_node->node_type  = AST_NODE_TYPE_STRUCTURE_MEMBER;
                        member_node->identifier = c_string_make_copy(&permanent_arena, member_name_token.data); 

                        code_type_t *code_type = symbol_table_search_for_code_type(typename_token.data);
                        if(!code_type || !code_type->is_registered)
                        {
                            code_type = symbol_table_register_typename(typename_token.data, CODE_TYPE_UNDEFINED);
                            printf("Registered structure type: '%.*s'...\n", fprint_string(typename_token.data));
                        }

                        member_node->type.code_type     = code_type;
                        member_node->type.pointer_depth = pointer_depth;
                        member_node->type.flags         = type_modifier_flags;
                        if(pointer_depth > 0)
                        {
                            member_node->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                        }

                        AST_add_member(structure_root, member_node);

                        // NOTE(Sleepster): Eat the semicolon, if it is not a semicolon, then check for an array or a default expression value
                        token = symbol_table_get_next_lexer_token(lexer);
                        if(token.token_type == TOKEN_TYPE_OPEN_BRACKET)
                        {
                            member_node->expression.info = generate_expression_AST(lexer, 0, null);
                            member_node->type.flags     |= AST_TYPE_MODIFIER_FLAG_ARRAY;

                            lexer_token_t token = lexer_peek_token(lexer);
                            if(token.token_type != TOKEN_TYPE_SEMICOLON)
                            {
                                report_error(lexer, "Expected token following an array declaration to be that of a ';', instead found: '%.s'...\n", fprint_token(token));
                            }
                        }
                        else if(token.token_type == TOKEN_TYPE_EQUALS)
                        {
                            // NOTE(Sleepster): Generate the assignment AST 
                            member_node->expression.info = generate_expression_AST(lexer, 0, &token);
                        }
                    }
                    else if(typename_token.token_type == TOKEN_TYPE_STRUCT || 
                            typename_token.token_type == TOKEN_TYPE_UNION)
                    {
                        AST_node_t *nested_structure = generate_structure_AST(lexer);
                        AST_add_member(structure_root, nested_structure);
                    }
                }
            }
            else
            {
                lexer_eat_lines(&transient_arena, lexer, 1);
            }
        }
        
        token = symbol_table_get_next_lexer_token(lexer);
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            symbol_table_register_typename(token.data, struct_ID);
            token = symbol_table_get_next_lexer_token(lexer);
        }
        if(token.token_type != TOKEN_TYPE_SEMICOLON)
        {
            report_error(lexer,
                         "Finished parsing a structured type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
                         fprint_token(token));
        }

        result = structure_root;
    }

    return(result);
}

// TODO(Sleepster): Should this create a code type?
internal_api AST_node_t*
generate_enum_AST(lexer_t *lexer)
{
    AST_node_t *result = null;

    u64 enum_ID  = 0;

    lexer_token_t token;
    lexer_token_t name_token = symbol_table_get_next_lexer_token(lexer);
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        code_type_t *enum_data = symbol_table_register_typename(name_token.data, CODE_TYPE_ENUM);
        printf("Registered enum type: '%.*s'...\n", fprint_string(name_token.data));

        enum_ID = enum_data->ID;
        token   = symbol_table_get_next_lexer_token(lexer);
    }
    else if(name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        token = name_token;
    }

    if(token.token_type != TOKEN_TYPE_OPEN_BRACE && token.token_type != TOKEN_TYPE_SEMICOLON)
    {
        report_error(lexer,
                     "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
                     fprint_token(name_token), fprint_token(token));
    }

    // NOTE(Sleepster): Same as a structured type, if it's an open brace it's a definition 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        AST_node_t *enum_root     = AST_create_new_node(&permanent_arena);
        enum_root->node_type      = AST_NODE_TYPE_ENUM;
        enum_root->type.code_type = symbol_table_search_for_code_type(name_token.data);
        if(enum_ID != INVALID_ID)
        {
            code_type_t *type = hash_table_get_element_ptr_at_index(&g_symbol_table.type_table, enum_ID);
            if(!type->type_info_AST)
            {
                type->type_info_AST = enum_root;
            }
        }

        push_scope_stack(name_token.data);
        defer(pop_scope_stack());

        if(name_token.token_type == TOKEN_TYPE_IDENT) enum_root->identifier = c_string_make_copy(&permanent_arena, name_token.data);
        else                                          enum_root->identifier = STR("anonymous");
        for(;;)
        {
            if(token.token_type == TOKEN_TYPE_CLOSE_BRACE) break;

            lexer_token_t enum_member_token = symbol_table_get_next_lexer_token(lexer);
            if(enum_member_token.token_type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if(enum_member_token.token_type != TOKEN_TYPE_IDENT)
            {
                report_error(lexer, "Expected to find an identifier when parsing type information for an enum... Instead found: '%.*s'...\n", fprint_token(enum_member_token));
            }

            AST_node_t *member = AST_create_new_node(&permanent_arena);
            member->node_type  = AST_NODE_TYPE_ENUM_MEMBER;
            member->identifier = c_string_make_copy(&permanent_arena, enum_member_token.data);

            AST_add_member(enum_root, member);

            // NOTE(Sleepster): Eat whatever comes after the member name...
            token = symbol_table_get_next_lexer_token(lexer);

            // NOTE(Sleepster): If it's an '=' then this is an expression 
            if(token.token_type == TOKEN_TYPE_EQUALS)
            {
                member->expression.info = generate_expression_AST(lexer, 0, &token);
            }
        }

        token = symbol_table_get_next_lexer_token(lexer);
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            symbol_table_register_typename(token.data, enum_ID);
            token = symbol_table_get_next_lexer_token(lexer);
        }
        if(token.token_type != TOKEN_TYPE_SEMICOLON)
        {
            report_error(lexer,
                         "Finished parsing a enum type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
                         fprint_token(token));
        }

        result = enum_root;
    }

    return(result);
}

internal_api AST_type_t 
symbol_table_create_lambda_type(AST_node_t *expression)
{
    // NOTE(Sleepster): Return an AST_type_t that says:
    //
    // "We are a lambda type named "allocator" that returns a void * and takes in a memory_arena_t * and a size..."
    //
    // and use that as the type. Sort of like a C-style typedef:
    //
    // typedef void *allocator_t(memory_arena_t *arena, u32 size);
    // allocator_t item_allocator;
    //
    // "allocator_t" is a lambda type that returns a void* and takes in a memory_arena_t * and a u32 size.
    code_type_t *lambda_type = symbol_table_register_typename(expression->identifier, CODE_TYPE_LAMBDA);
    printf("Registered lambda type: '%.*s'...\n", fprint_string(expression->identifier));

    AST_type_t new_type = {};
    new_type.flags      = AST_TYPE_MODIFIER_FLAG_PROCEDURE;
    new_type.code_type  = lambda_type;

    return(new_type);
}

internal_api AST_node_t* 
generate_lambda_AST(lexer_t      *lexer, 
                    lexer_token_t return_type_token, 
                    u32           return_type_pointer_depth, 
                    bool8         return_type_is_const)
{
    AST_node_t *return_type = AST_create_new_node(&permanent_arena);
    return_type->node_type  = AST_NODE_TYPE_LAMBDA_RETURN_TYPE;
    return_type->identifier = c_string_make_copy(&permanent_arena, return_type_token.data);

    AST_type_t *return_type_data = &return_type->type;

    return_type_data->code_type = symbol_table_search_for_code_type(return_type->identifier);
    if(!return_type_data->code_type)
    {
        return_type_data->code_type = symbol_table_register_typename(return_type->identifier, CODE_TYPE_UNDEFINED);
        printf("Registered type: '%.*s'...\n", fprint_string(return_type->identifier));
    }

    return_type_data->literal       = c_string_make_copy(&permanent_arena, return_type_token.data);
    return_type_data->flags         = return_type_pointer_depth > 0 ? AST_TYPE_MODIFIER_FLAG_POINTER : 0;
    return_type_data->pointer_depth = return_type_pointer_depth;
    if(return_type_is_const)
    {
        return_type_data->flags |= AST_TYPE_MODIFIER_FLAG_CONST;
    }

    lexer_token_t procedure_name_token = symbol_table_get_next_lexer_token(lexer);
    if(procedure_name_token.token_type != TOKEN_TYPE_IDENT)
    {
        for(u32 pointer_index = 0;
            pointer_index < return_type_pointer_depth;
            ++pointer_index)
        {
            procedure_name_token = symbol_table_get_next_lexer_token(lexer);
        }
    }

    // NOTE(Sleepster): Generate the lambda_node 
    if(procedure_name_token.token_type != TOKEN_TYPE_IDENT)
    {
        report_error(lexer,
                     "Expected to find an identifier for the name of this lambda, instead found: '%.*s'\n",
                     fprint_token(procedure_name_token));
    }

    AST_node_t *lambda = AST_create_new_node(&permanent_arena);
    lambda->lambda.return_type = return_type;

    parse_lambda_argument_list(lexer, lambda);

    // NOTE(Sleepster): Fill in the data related to the lambda. 
    lambda->node_type  = AST_NODE_TYPE_LAMBDA;
    lambda->identifier = c_string_make_copy(&permanent_arena, procedure_name_token.data);
    lambda->type       = symbol_table_create_lambda_type(lambda);

    return(lambda);
}

internal_api AST_node_t* 
generate_typedef_AST(lexer_t *lexer)
{
    AST_node_t *result = null;

    lexer_token_t type_token = symbol_table_get_next_lexer_token(lexer);
    switch(type_token.token_type)
    {
        case TOKEN_TYPE_STRUCT:
        case TOKEN_TYPE_UNION:
        {
            result = generate_structure_AST(lexer);
        }break;
        case TOKEN_TYPE_ENUM:
        {
            result = generate_enum_AST(lexer);
        }break;
        case TOKEN_TYPE_IDENT:
        {
            lexer_token_t alias_token = lexer_peek_token(lexer);

            u32 pointer_depth = 0;
            while(alias_token.token_type == TOKEN_TYPE_ASTERISK)
            {
                alias_token = lexer_peek_token(lexer, pointer_depth + 2);
                ++pointer_depth;
            }

            if(alias_token.token_type == TOKEN_TYPE_IDENT)
            {
                lexer_token_t final_token = lexer_peek_token(lexer, pointer_depth + 2);
                // NOTE(Sleepster): 
                // For something as simple as this typedef expression, 
                // we do not need an AST since that is insanely redundant...
                if(final_token.token_type == TOKEN_TYPE_SEMICOLON)
                {
                    code_type_t *main_type = symbol_table_search_for_code_type(type_token.data);
                    if(!main_type)
                    {
                        main_type = symbol_table_register_typename(type_token.data, CODE_TYPE_UNDEFINED);
                    }

                    symbol_table_register_typename(alias_token.data, main_type->ID);
                    printf("FOUND TYPE ALIAS: '%.*s' OF TYPE: '%.*s'...\n",
                           fprint_token(alias_token), fprint_token(type_token));

                    lexer_eat_lines(&transient_arena, lexer, 1);
                }
                else if(final_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                {
                    // NOTE(Sleepster): Lambda. 
                    result = generate_lambda_AST(lexer, 
                                                 type_token, 
                                                 pointer_depth, 
                                                 false);
                }
            }
        }break;
    }

    return(result);
}
