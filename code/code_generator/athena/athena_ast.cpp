/* ========================================================================
   $File: athena_ast.cpp $
   $Date: May 30 2026 12:10 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "athena_ast.h"

internal_api AST_node_t* generate_structure_AST(parser_t *parser);

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
AST_create_new_node(memory_arena_t *arena, declaration_context_t *current_context)
{
    AST_node_t *result = c_arena_push_struct(arena, AST_node_t);
    ZeroStruct(*result);

    result->decl_context = current_context;

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

    ++parent->struct_decl.member_count;
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
build_number_AST_node(parser_t *parser, AST_node_t *value_expression, lexer_token_t token)
{
    lexer_token_t value_token = token;
    if(token.token_type == TOKEN_TYPE_DASH)
    {
        value_expression->type.flags |= AST_TYPE_MODIFIER_FLAG_SIGNED;
        parser_get_next_lexer_token(parser);
    }

    s32 decimal_index = c_string_find_first_char_from_left(token.data, '.');
    if(decimal_index == -1)
    {
        lexer_token_t float_token = parser_peek_next_lexer_token(parser, 1);
        if(float_token.data.count == 1 && float_token.data.data[0] == 'f')
        {
            value_expression->type.flags |= AST_TYPE_MODIFIER_FLAG_FLOAT;
        }
    }
    else
    {
        value_expression->type.flags |= AST_TYPE_MODIFIER_FLAG_DOUBLE_FLOAT;
    }

    value_expression->node_type          = AST_NODE_TYPE_NUMBER;
    value_expression->type.value_literal = c_string_make_copy(&parser->arena, value_token.data); 
}

internal_api u32
get_infix_binding_power(lexer_token_t *token)
{
    u32 result = 0;
    switch(token->token_type)
    {
        case TOKEN_TYPE_TERNARY_IF:
        {
            result = 2;
        }break;
        case TOKEN_TYPE_OR:
        {
            result = 5;
        }break;
        case TOKEN_TYPE_ANDAND:
        {
            result = 10;
        }break;
        case TOKEN_TYPE_XOR:
        {
            result = 20;
        }break;
        case TOKEN_TYPE_AND:
        {
            result = 25;
        }break;
        case TOKEN_TYPE_GREATER_THAN:
        case TOKEN_TYPE_LESS_THAN:
        case TOKEN_TYPE_LESS_EQUAL:
        case TOKEN_TYPE_GREATER_EQUAL:
        {
            result = 35;
        }break;
        case TOKEN_TYPE_BITSHIFT_LEFT:
        case TOKEN_TYPE_BITSHIFT_RIGHT:
        {
            result = 40;
        }break;
        case TOKEN_TYPE_PLUS:
        case TOKEN_TYPE_DASH:
        {
            result = 50;
        }break;
        case TOKEN_TYPE_FORWARD_SLASH:
        case TOKEN_TYPE_ASTERISK:
        {
            result = 60;
        }break;
        default:
        {
            result = 0;
        }break;
    }

    return(result);
}

internal_api s32
get_prefix_binding_power(lexer_token_t *token)
{
    s32 result = -1;
    switch(token->token_type)
    {
        case TOKEN_TYPE_BANG: 
        case TOKEN_TYPE_DASH: 
        { 
            result = 70; 
        }break;
    }

    return(result);
}

internal_api AST_node_t*
generate_unary_expression_AST(parser_t *parser, lexer_token_t *token, AST_node_t *unary_operation)
{
    AST_node_t *result = AST_create_new_node(&parser->arena, parser->active_decl_context);
    result->node_type  = AST_NODE_TYPE_UNARY_EXPRESSION;
    result->unary_expression.operator_type = token->token_type;
    result->unary_expression.operand       = unary_operation;

    return(result);
}

internal_api AST_node_t*
generate_binary_expression_AST(parser_t *parser, lexer_token_t *operator_token, AST_node_t *left_node, AST_node_t *right_node)
{
    AST_node_t *result = AST_create_new_node(&parser->arena, parser->active_decl_context);
    result->node_type  = AST_NODE_TYPE_BINARY_EXPRESSION;
    result->binary_expression.operator_type = operator_token->token_type;
    result->binary_expression.left          = left_node;
    result->binary_expression.right         = right_node;

    return(result);
}

internal_api AST_node_t*
generate_nud_prefix_AST(parser_t *parser, lexer_token_t *token)
{
    AST_node_t *result = null;

    s32 prefix_value = get_prefix_binding_power(token);
    if(prefix_value == -1)
    {
        // NOTE(Sleepster): If this is not a prefix value (like '-') 
        result = AST_create_new_node(&parser->arena, parser->active_decl_context);
        switch(token->token_type)
        {
            case TOKEN_TYPE_NUMBER:
            {
                build_number_AST_node(parser, result, *token);
            }break;
            // NOTE(Sleepster): For macros, this will never happen since the symbol table simply substitues the macro expansion in place. 
            case TOKEN_TYPE_IDENT:
            {
                if(c_string_compare(token->data, STR("true")))
                {
                    result->type.value_literal  = c_string_make_copy(&parser->arena, STR("1"));
                    result->node_type = AST_NODE_TYPE_NUMBER;
                }
                else if(c_string_compare(token->data, STR("false")))
                {
                    result->type.value_literal  = c_string_make_copy(&parser->arena, STR("0"));
                    result->node_type = AST_NODE_TYPE_NUMBER;
                }
                else
                {
                    result->type.value_literal = c_string_make_copy(&parser->arena, token->data);
                    result->identifier = c_string_make_copy(&parser->arena, token->data);
                    result->node_type  = AST_NODE_TYPE_IDENTIFIER;
                }
            }break;
            case TOKEN_TYPE_LITERAL:
            {
                result->node_type          = AST_NODE_TYPE_LITERAL;
                result->type.value_literal = c_string_make_copy(&parser->arena, token->data);

                printf("String literal: '%.*s' found when generating an expression...\n", (s32)token->data.count, token->data.data);
            }break;
            case TOKEN_TYPE_OPEN_PAREN:
            {
                // NOTE(Sleepster): Subexpression 
                result = generate_expression_AST(parser, 0, token);
            }break;
            default: 
            {
                if(token->token_type != TOKEN_TYPE_IDENT & token->token_type != TOKEN_TYPE_NULL && token->token_type != TOKEN_TYPE_NULLPTR)
                {
                    report_error(parser, 
                                 "Expected an identifier as the last option here... instead found: '%.*s'... It is of token_type: '%s'\n",
                                 token->data.count, token->data.data, lexer_token_type_to_string(token));
                }

                result->node_type  = AST_NODE_TYPE_IDENTIFIER;
                result->identifier = c_string_make_copy(&parser->arena, token->data);
            }break;
        }
    }
    else
    {
        AST_node_t *unary_operation = generate_expression_AST(parser, prefix_value, token);
        result = generate_unary_expression_AST(parser, token, unary_operation);
    }

    return(result);
}

internal_api AST_node_t*
generate_ternary_led_AST(parser_t *parser, AST_node_t *conditional_expression)
{
    lexer_token_t dummy;
    AST_node_t *then_expr = generate_expression_AST(parser, 0, &dummy);
    Expect(dummy.token_type == TOKEN_TYPE_COLON,
           "Expected ':' in ternary expression, got '%.*s'\n",
           fprint_token(dummy));

    // NOTE(Sleepster): Infix value for ternary is 2 
    // Parse the "else" branch with ternary_value - 1 for right-associativity.
    // This lets nested ternaries chain rightward:
    //      a ? b : c ? d : e  ->  a ? b : (c ? d : e)
    AST_node_t *else_expr = generate_expression_AST(parser, 2 - 1, &dummy);

    AST_node_t *result = AST_create_new_node(&parser->arena, parser->active_decl_context);
    result->node_type                    = AST_NODE_TYPE_TERNARY_EXPRESSION;
    result->ternary_expression.condition = conditional_expression;
    result->ternary_expression.then_expr = then_expr;
    result->ternary_expression.else_expr = else_expr;

    return(result);
}

internal_api AST_node_t*
generate_led_AST(parser_t *parser, lexer_token_t *token, AST_node_t *left_hand_expression)
{
    AST_node_t *full_expression = null;
    if(token->token_type != TOKEN_TYPE_TERNARY_IF)
    {
        u32 current_infix_binding_power = get_infix_binding_power(token);
        lexer_token_t operator_token = *token;

        AST_node_t *right_expression = generate_expression_AST(parser, current_infix_binding_power, token);
        full_expression = generate_binary_expression_AST(parser, &operator_token, left_hand_expression, right_expression);
    }
    else
    {
        full_expression = generate_ternary_led_AST(parser, left_hand_expression);
    }

    return(full_expression);
}

internal_api AST_node_t* 
generate_expression_AST(parser_t *parser, s32 expression_min_binding_power, lexer_token_t *token_out)
{
    AST_node_t *value_expression = null;

    // NOTE(Sleepster): Is the next token a number? 
    lexer_token_t token       = parser_get_next_lexer_token(parser);
    AST_node_t *left_hand_AST = generate_nud_prefix_AST(parser, &token);

    token = parser_get_next_lexer_token(parser);
    for(;;)
    {
        // NOTE(Sleepster): Measure the current left expression's binding power if it beats our fence, fold it in. Otherwise, ignore it.
        s32 current_left_expression_binding_power = get_infix_binding_power(&token);
        if(current_left_expression_binding_power >= expression_min_binding_power && 
           current_left_expression_binding_power != 0)
        {
            left_hand_AST = generate_led_AST(parser, &token, left_hand_AST);
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
            case AST_EXPRESSION_VALUE_DOUBLE:   { intermediate = (float64)value.float64_value;  }break;
        }

        result = {target_type};
        switch(target_type)
        {
            case AST_EXPRESSION_VALUE_INT:      { result.int_value      = (s32)intermediate;     }break; 
            case AST_EXPRESSION_VALUE_UNSIGNED: { result.unsigned_value = (u32)intermediate;     }break; 
            case AST_EXPRESSION_VALUE_FLOAT:    { result.float32_value  = (float32)intermediate; }break; 
            case AST_EXPRESSION_VALUE_DOUBLE:   { result.float64_value  = (float64)intermediate; }break; 
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
                case TOKEN_TYPE_PLUS:         { value.int_value = left.int_value + right.int_value; }break;
                case TOKEN_TYPE_DASH:         { value.int_value = left.int_value - right.int_value; }break;
                case TOKEN_TYPE_ASTERISK:     { value.int_value = left.int_value * right.int_value; }break;
                case TOKEN_TYPE_OR:           { value.int_value = left.int_value | right.int_value; }break; 
                case TOKEN_TYPE_AND:          { value.int_value = left.int_value & right.int_value; }break; 
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.int_value != 0, "Division by zero\n");
                    value.int_value = left.int_value / right.int_value;
                }break;
                case TOKEN_TYPE_BITSHIFT_LEFT:  { value.int_value = (s32)((u32)left.int_value << right.int_value); }break;
                case TOKEN_TYPE_BITSHIFT_RIGHT: { value.int_value = left.int_value >> right.int_value;             }break;
                default: Expect(false, "Unknown operator for s32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_UNSIGNED:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           { value.unsigned_value = left.unsigned_value + right.unsigned_value; }break;
                case TOKEN_TYPE_DASH:           { value.unsigned_value = left.unsigned_value - right.unsigned_value; }break;
                case TOKEN_TYPE_ASTERISK:       { value.unsigned_value = left.unsigned_value * right.unsigned_value; }break;
                case TOKEN_TYPE_OR:             { value.unsigned_value = left.unsigned_value | right.unsigned_value; }break; 
                case TOKEN_TYPE_AND:            { value.unsigned_value = left.unsigned_value & right.unsigned_value; }break; 
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.unsigned_value != 0, "Division by zero\n");
                    value.unsigned_value = left.unsigned_value / right.unsigned_value;
                }break;
                case TOKEN_TYPE_BITSHIFT_LEFT:  { value.unsigned_value = left.unsigned_value << right.unsigned_value; }break;
                case TOKEN_TYPE_BITSHIFT_RIGHT: { value.unsigned_value = left.unsigned_value >> right.unsigned_value; }break;

                // TODO(Sleepster): 
                // The issue here is very simple, for some reason the final character of an expression 
                // (whether it be a comma, or a close paren, or literally anything) gets added to the expression's AST. This is bad.
                case TOKEN_TYPE_CLOSE_PAREN: { Expect(false, "Bro what the hell bro...\n"); }break;
                default: Expect(false, "Unknown operator for u32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_FLOAT:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:          { value.float32_value = left.float32_value + right.float32_value; }break;
                case TOKEN_TYPE_DASH:          { value.float32_value = left.float32_value - right.float32_value; }break;
                case TOKEN_TYPE_ASTERISK:      { value.float32_value = left.float32_value * right.float32_value; }break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.float32_value != 0.0f, "Division by zero\n");
                    value.float32_value = left.float32_value / right.float32_value;
                }break;
                default: Expect(false, "Unknown operator for f32\n"); break;
            }
        }break;
        case AST_EXPRESSION_VALUE_DOUBLE:
        {
            switch(operator_type)
            {
                case TOKEN_TYPE_PLUS:           { value.float64_value = left.float64_value + right.float64_value; }break;
                case TOKEN_TYPE_DASH:           { value.float64_value = left.float64_value - right.float64_value; }break;
                case TOKEN_TYPE_ASTERISK:       { value.float64_value = left.float64_value * right.float64_value; }break;
                case TOKEN_TYPE_FORWARD_SLASH:
                {
                    Expect(right.float64_value != 0.0, "Division by zero\n");
                    value.float64_value = left.float64_value / right.float64_value;
                }break;
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
        case AST_NODE_TYPE_IDENTIFIER:
        {
            // NOTE(Sleepster): Inside generate, we record all constexpr as identifiers.
            // In this case, we must do what generate used to do, and determine the type here.
            AST_node_t *constant_expression = symbol_table_find_code_declaration(expression->identifier);
            if(constant_expression && constant_expression->node_type == AST_NODE_TYPE_CONSTEXPR)
            {
                if(constant_expression->expression.evaluated == false)
                {
                    constant_expression->expression.value     = evaluate_expression_AST(constant_expression->expression.info);
                    constant_expression->expression.evaluated = true;
                }

                result = constant_expression->expression.value;
            }
            else
            {
                language_keyword_t *keyword = get_keyword_from_identifier(expression->identifier);
                if(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
                {
                    if(keyword->keyword_id == TOKEN_KEYWORD_NULL || keyword->keyword_id == TOKEN_KEYWORD_NULLPTR)
                    {
                        result.type      = AST_EXPRESSION_VALUE_INT;
                        result.int_value = 0;
                    }
                    else
                    {
                        Expect(false, "Undeclared identifier within expression: '%.*s'...\n", fprint_string(expression->identifier));
                    }
                }                    
                else
                {
                    AST_node_t *node = hash_table_get_element(&expression->decl_context->enum_symbols, expression->identifier);
                    if(node && node->identifier.data != null)
                    {
                        if(node->expression.evaluated)
                        {
                            result = node->expression.value;
                        }
                        else
                        {
                            Expect(false, "Oopsies how are you here???\n");
                        }
                    }
                    else
                    {
                        result.type             = AST_EXPRESSION_VALUE_IDENT;
                        result.identifier_value = expression->identifier;
                    }
                }
            }
        }break;
        case AST_NODE_TYPE_LITERAL:
        {
            result.type             = AST_EXPRESSION_VALUE_LITERAL;
            result.identifier_value = type->value_literal;
        }break;
        case AST_NODE_TYPE_NUMBER:
        {
            if(type->flags & AST_TYPE_MODIFIER_FLAG_FLOAT)
            {
                result.type          = AST_EXPRESSION_VALUE_FLOAT;
                result.float32_value = c_string_read_float32(type->value_literal);
            }
            else if(type->flags & AST_TYPE_MODIFIER_FLAG_DOUBLE_FLOAT)
            {
                result.type          = AST_EXPRESSION_VALUE_DOUBLE;
                result.float64_value = c_string_read_float64(type->value_literal);
            }

            if((type->flags & AST_TYPE_MODIFIER_FLAG_SIGNED) && 
               ((type->flags & AST_TYPE_MODIFIER_FLAG_FLOAT) == 0) && 
               ((type->flags & AST_TYPE_MODIFIER_FLAG_DOUBLE_FLOAT) == 0))
            {
                result.type = AST_EXPRESSION_VALUE_INT;
                result.int_value = c_string_read_int(type->value_literal);
            }
            else if(((type->flags & AST_TYPE_MODIFIER_FLAG_FLOAT) == 0) && 
                    ((type->flags & AST_TYPE_MODIFIER_FLAG_DOUBLE_FLOAT) == 0))
            {
                result.type = AST_EXPRESSION_VALUE_UNSIGNED;
                result.int_value = c_string_read_uint(type->value_literal);
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
        case AST_NODE_TYPE_TERNARY_EXPRESSION:
        {
            Assert(false);
        }break;
        default: { Expect(false, "Node_type is invalid: '%s'...\n", print_AST_node_type(expression->node_type)); }break;
    }

    return(result);
}

internal_api argument_list_t 
parse_argument_list(parser_t *parser, AST_node_t *parent)
{
    argument_list_t result = {};

    lexer_token_t token = parser_get_next_lexer_token(parser);
    do {
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            lexer_token_t typename_token = token;

            // NOTE(Sleepster): Check for qualifiers like const or volatile 
            u32 argument_flags = 0;
            language_keyword_t *keyword = get_keyword_from_identifier(token.data);
            if(keyword->keyword_id == TOKEN_KEYWORD_CONST)
            {
                typename_token = parser_get_next_lexer_token(parser);
                argument_flags |= AST_TYPE_MODIFIER_FLAG_CONST;
            }
            if(keyword->keyword_id == TOKEN_KEYWORD_VOLATILE)
            {
                typename_token = parser_get_next_lexer_token(parser);
                argument_flags |= AST_TYPE_MODIFIER_FLAG_VOLATILE;
            }

            // NOTE(Sleepster): Check if this is a namespaced type 
            lexer_token_t check_namespace_token = parser_peek_next_lexer_token(parser);
            if(check_namespace_token.token_type == TOKEN_TYPE_DOUBLE_COLON)
            {
                parser_get_next_lexer_token(parser);
                typename_token = parser_get_next_lexer_token(parser);
            }
            else if(check_namespace_token.token_type == TOKEN_TYPE_LESS_THAN)
            {
                // TODO(Sleepster): For now we just assume that templates are waste of time to parse here. However if this
                // needs to change, here's where you'd do so.
                while(check_namespace_token.token_type != TOKEN_TYPE_GREATER_THAN)
                {
                    check_namespace_token = parser_get_next_lexer_token(parser);
                }
            }

            lexer_token_t declaration_name = parser_get_next_lexer_token(parser);

            u32 pointer_depth = 0;
            while(declaration_name.token_type == TOKEN_TYPE_ASTERISK)
            {
                declaration_name = parser_get_next_lexer_token(parser);
                ++pointer_depth;
            }

            if(declaration_name.token_type != TOKEN_TYPE_IDENT && 
               c_string_compare(typename_token.data, STR("void")))
            {
                // NOTE(Sleepster): 
                // This is a declaration like:
                //
                // int random_function(void);
                //
                // therefore there are no arguments
                break;
            }

            if(declaration_name.token_type != TOKEN_TYPE_IDENT)
            {
                report_error(parser, "Error, expected to find identifier following a typename in this argument list... Instead found: '%.*s' it is of token_type: '%.*s'...\n",
                             fprint_token(declaration_name), lexer_token_type_to_string(&declaration_name));
            }

            AST_node_t *argument = AST_create_new_node(&parser->arena, parser->active_decl_context);
            argument->node_type  = AST_NODE_TYPE_LAMBDA_ARGUMENT;
            argument->identifier = c_string_make_copy(&parser->arena, declaration_name.data);

            AST_type_t *type_data = &argument->type;
            if(!c_string_compare(typename_token.data, STR("typename")))
            {
                type_data->code_type = parser_register_code_type(parser, c_string_make_copy(&parser->arena, typename_token.data));
            }

            type_data->flags         = pointer_depth > 0 ? (argument_flags | AST_TYPE_MODIFIER_FLAG_POINTER) : argument_flags;
            type_data->pointer_depth = pointer_depth;
            if(result.first_argument == null)
            {
                result.first_argument = argument;
            }
            else
            {
                for(AST_node_t *current_argument = result.first_argument;
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

            token = parser_get_next_lexer_token(parser);
            if(token.token_type == TOKEN_TYPE_EQUALS)
            {
                argument->expression.info = generate_expression_AST(parser, 0, &token);
            }

            ++result.argument_count;

            if(token.token_type == TOKEN_TYPE_CLOSE_PAREN  ||
               token.token_type == TOKEN_TYPE_GREATER_THAN)
            {
                break;
            }


            // int main(int argc, char *argv);
            // int add(namespace::item item, namespace::thing thing)
            // template<typename T, typename U>
        }
        else if(token.token_type == TOKEN_TYPE_NUMBER)
        {
            // NOTE(Sleepster): Exit if we find a number, if we find one this is likely a constructor. 
            break;
        }

        token = parser_get_next_lexer_token(parser);
    }while(token.token_type != TOKEN_TYPE_CLOSE_PAREN && 
           token.token_type != TOKEN_TYPE_CLOSE_BRACE && 
           token.token_type != TOKEN_TYPE_GREATER_THAN);

    return(result);
}

internal_api void
add_AST_overload(AST_node_t *previous, AST_node_t *overload)
{
    AST_node_t *node      = previous;
    AST_node_t *last_node = node;

    bool8 found = false;
    while(node)
    {
        if(node->node_type == AST_NODE_TYPE_LAMBDA)
        {
            if(node->lambda.argument_count == overload->lambda.argument_count && 
               node->decl_context->context_ID == overload->decl_context->context_ID)
            {
                AST_node_t *lambda_arg = overload->lambda.first_argument;
                AST_node_t *node_arg   = node->lambda.first_argument;

                u32 matching_argument_count = 0;
                while(lambda_arg && node_arg)
                {
                    if(lambda_arg->type.code_type == node_arg->type.code_type)
                    {
                        matching_argument_count += 1;
                    }

                    lambda_arg = lambda_arg->next_sibling;
                    node_arg   = node_arg->next_sibling;
                }

                // NOTE(Sleepster): Add as overload if all arguments do not match 
                if(matching_argument_count == overload->lambda.argument_count)
                {
                    found = true;
                    break;
                }
            }
            else
            {
                // NOTE(Sleepster): Add as overload if the argument count is different or the decl contexts are different
                found = true;
                break;
            }
        }

        last_node = node;
        node = node->next_overload;
    }

    Assert(last_node);
    if(!found)
    {
        last_node->next_overload = overload;
    }
}

#if 0
internal_api AST_node_t* 
generate_structure_AST(parser_t *parser)
{
    AST_node_t *result  = null;
    u32 structure_flags = 0;

    lexer_t *lexer = &parser->lexer;

    // NOTE(Sleepster): Skip this structure if it is a template! 
    bool8 parse_structure = true;
    for(const auto &attribute: parser->current_attribute_list)
    {
        if(attribute.is_template)
        {
            parse_structure = false;
        }
    }

    lexer_token_t name_token = parser_get_next_lexer_token(parser);
    lexer_token_t token;
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        if(c_string_compare(name_token.data, STR("dynamic_render_font_page_t")))
        {
            int x = 0;
        }
        token = parser_get_next_lexer_token(parser);
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
        lexer_token_t inheritance_publicity_token = parser_get_next_lexer_token(parser);
        
        // NOTE(Sleepster): If the denotion is neither private nor public, it's invalid. 
        if(inheritance_publicity_token.token_type != TOKEN_TYPE_PRIVATE && 
           inheritance_publicity_token.token_type != TOKEN_TYPE_PUBLIC)
        {
            report_error(parser,
                         "Expected there to be a public/private denotion on the inherited structure, instead found: '%.*s'...\n",
                         fprint_token(inheritance_publicity_token));
        }

        lexer_token_t inherited_typename = parser_get_next_lexer_token(parser);
        if(inherited_typename.token_type != TOKEN_TYPE_IDENT)
        {
            report_error(parser,
                         "Expected there to be a typename for the inherited structure, instead found: '%.*s'...\n",
                         fprint_token(inherited_typename));
        }

        inheritance_node            = AST_create_new_node(&parser->arena, parser->active_decl_context);
        inheritance_node->node_type = AST_NODE_TYPE_INHERITANCE_INFO;

        code_type_t *inherited_type = parser_register_code_type(parser, c_string_make_copy(&parser->arena, inherited_typename.data));

        inheritance_node->inheritance_info.inheritance_type = inheritance_publicity_token.token_type;
        inheritance_node->inheritance_info.inherited_data   = inheritance_node;

        inheritance_node->type.code_type = inherited_type;

        // NOTE(Sleepster): Eat the token right before the open brace 
        token = parser_get_next_lexer_token(parser);
    }
    else if(token.token_type == TOKEN_TYPE_IDENT)
    {
        // NOTE(Sleepster): This was a forward declaration! 
        return(result);
    }

    if(token.token_type != TOKEN_TYPE_OPEN_BRACE && token.token_type != TOKEN_TYPE_SEMICOLON)
    {
        report_error(parser, 
                     "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
                     fprint_token(name_token), fprint_token(token));
    }

    // NOTE(Sleepster): If it's a brace, it's a definition... otherwise it's just a declaration and we don't care... 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        if(parse_structure)
        {
            AST_node_t *structure_root = AST_create_new_node(&parser->arena, parser->active_decl_context);
            structure_root->node_type  = AST_NODE_TYPE_STRUCTURE;
            structure_root->type.flags = structure_flags;

            structure_root->line_number = parser->lexer.current_stream->line_number;

            // NOTE(Sleepster): Copy the attributes 
            if(parser->current_attribute_list.used > 0)
            {
                dynarray_copy(&structure_root->attributes, &parser->current_attribute_list);
                dynarray_reset(&parser->current_attribute_list);
            }

            // NOTE(Sleepster): Handle inheritence 
            if(inheritance_node)
            {
                structure_root->struct_decl.inherited_type_info = inheritance_node;
            }

            bool8 pushed_context = false;

            // NOTE(Sleepster): Set the code type of the structure 
            if(name_token.token_type == TOKEN_TYPE_IDENT) 
            {
                structure_root->identifier = c_string_make_copy(&parser->arena, name_token.data);
                // NOTE(Sleepster): Create a new declaration_context_t for this scope, then push items onto it. 
                declaration_context_t *context = parser_create_declaration_context(parser, 
                                                                                   structure_root->identifier, 
                                                                                   parser->active_decl_context);
                parser_push_decl_context(parser, context);
                pushed_context = true;

                structure_root->decl_context = context;
            }

            u32 scope_depth = 1;
            lexer_token_t typename_token = {};
            while(scope_depth > 0 && typename_token.token_type != TOKEN_TYPE_EOF)
            {
                typename_token = parser_get_next_lexer_token(parser);
                if(typename_token.token_type == TOKEN_TYPE_CLOSE_BRACE) 
                {
                    lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 1);
                    if(peek_token.token_type == TOKEN_TYPE_SEMICOLON || peek_token.token_type == TOKEN_TYPE_IDENT)
                    {
                        --scope_depth;
                    }
                }

                if(typename_token.token_type == TOKEN_TYPE_STRUCT)
                {
                    lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 1);
                    if(peek_token.token_type == TOKEN_TYPE_IDENT)
                    {
                        lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 2);
                        if(peek_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                        {
                            typename_token = parser_get_next_lexer_token(parser);
                        }
                    }
                }

                // NOTE(Sleepster): Handle type modifier (const or volatile)
                u32 type_modifier_flags = 0;
                if(typename_token.token_type == TOKEN_TYPE_UNION)
                {
                    type_modifier_flags |= AST_TYPE_MODIFIER_FLAG_UNION;
                }

                language_keyword_t *keyword = get_keyword_from_identifier(typename_token.data);
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

                        typename_token = parser_get_next_lexer_token(parser);
                    }

                    keyword = get_keyword_from_identifier(typename_token.data);
                }

                if(is_valid_member)
                {
                    lexer_token_t member_name_token = parser_peek_next_lexer_token(parser);
                    if(member_name_token.token_type == TOKEN_TYPE_LESS_THAN)
                    {
                        // NOTE(Sleepster): Eat the template params, we don't care. 
                        u32 template_depth = 0;
                        do {
                            member_name_token = parser_get_next_lexer_token(parser);
                            if(member_name_token.token_type == TOKEN_TYPE_LESS_THAN)
                            {
                                ++template_depth;
                            }
                            else if(member_name_token.token_type == TOKEN_TYPE_GREATER_THAN)
                            {
                                --template_depth;
                            }
                        }while(template_depth > 0);

                        // NOTE(Sleepster): This gets us the name 
                        member_name_token = parser_peek_next_lexer_token(parser);
                    }

                    // NOTE(Sleepster): A normal member that should be recorded, otherwise a constructor 
                    if(member_name_token.token_type != TOKEN_TYPE_OPEN_PAREN &&
                       typename_token.token_type    != TOKEN_TYPE_TILDE)
                    {
                        // NOTE(Sleepster): Handle member pointer 
                        u32 pointer_depth = 0;
                        if(member_name_token.token_type == TOKEN_TYPE_ASTERISK)
                        {
                            do {
                                member_name_token = parser_peek_next_lexer_token(parser, pointer_depth + 2);
                                ++pointer_depth;
                            }while(member_name_token.token_type == TOKEN_TYPE_ASTERISK);
                        }

                        // NOTE(Sleepster): This assert is disabled for anonymous structures...
                        // Expect(member_name_token.token_type == TOKEN_TYPE_IDENT, 
                        //        "Expected to find a member name in this location... Instead found: '%.*s'...\n",
                        //        fprint_token(member_name_token));

                        lexer_token_t peek_token = parser_peek_next_lexer_token(parser, pointer_depth + 2);
                        if(peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                        {
                            // usual lambda
                            AST_node_t *member_node = generate_lambda_AST(parser, 
                                                                          typename_token, 
                                                                          pointer_depth, 
                                                                          (type_modifier_flags & AST_TYPE_MODIFIER_FLAG_CONST) ? true : false);
                            member_node->identifier = c_string_make_copy(&parser->arena, member_name_token.data);
                            AST_add_member(structure_root, member_node);

                            peek_token = parser_peek_next_lexer_token(parser);
                            if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                            {
                                while(peek_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                                {
                                    peek_token = parser_get_next_lexer_token(parser);
                                }
                            }
                            else if(peek_token.token_type == TOKEN_TYPE_SEMICOLON)
                            {
                                parser_get_next_lexer_token(parser);
                            }
                        }
                        else if(typename_token.token_type == TOKEN_TYPE_IDENT)
                        {
                            parser_get_next_lexer_token(parser);
                            for(u32 pointer_index = 0;
                                pointer_index < pointer_depth;
                                ++pointer_index)
                            {
                                parser_get_next_lexer_token(parser);
                            }

                            AST_node_t *member_node = AST_create_new_node(&parser->arena, parser->active_decl_context);
                            member_node->node_type  = AST_NODE_TYPE_STRUCTURE_MEMBER;
                            member_node->identifier = c_string_make_copy(&parser->arena, member_name_token.data); 

                            code_type_t *member_type    = parser_register_code_type(parser, typename_token.data);
                            member_node->type.code_type = member_type;

                            member_node->type.pointer_depth = pointer_depth;
                            member_node->type.flags         = type_modifier_flags;
                            if(pointer_depth > 0)
                            {
                                member_node->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                            }

                            AST_add_member(structure_root, member_node);

                            // NOTE(Sleepster): Eat the semicolon, if it is not a semicolon, then check for an array or a default expression value
                            token = parser_get_next_lexer_token(parser);
                            if(token.token_type == TOKEN_TYPE_OPEN_BRACKET)
                            {
                                member_node->type.flags |= AST_TYPE_MODIFIER_FLAG_ARRAY;

                                AST_node_t **array = &member_node->array_data.array_expression;
                                while(token.token_type != TOKEN_TYPE_SEMICOLON)
                                {
                                    if(!(*array)) *array = c_arena_push_struct(&parser->arena, AST_node_t);

                                    *array = generate_expression_AST(parser, 0, &token);
                                    array = &(*array)->next_sibling;

                                    token = parser_get_next_lexer_token(parser);
                                }
                            }
                            else if(token.token_type == TOKEN_TYPE_EQUALS)
                            {
                                // NOTE(Sleepster): Generate the assignment AST 
                                member_node->expression.info = generate_expression_AST(parser, 0, &token);
                            }
                        }
                        else if(typename_token.token_type == TOKEN_TYPE_STRUCT || 
                                typename_token.token_type == TOKEN_TYPE_UNION)
                        {
                            lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 1);
                            if(peek_token.token_type == TOKEN_TYPE_IDENT)
                            {
                                AST_node_t *nested_structure = generate_structure_AST(parser);
                                AST_add_member(structure_root, nested_structure);
                            }
                            else if(peek_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                            {
                                ++scope_depth;
                            }
                        }
                    }
                    if(member_name_token.token_type == TOKEN_TYPE_OPEN_PAREN || 
                       typename_token.token_type == TOKEN_TYPE_TILDE)
                    {
                        // NOTE(Sleepster): Constructor / Deconstructor 
                        lexer_token_t end_token = parser_get_next_lexer_token(parser);
                        if(end_token.token_type != TOKEN_TYPE_SEMICOLON && end_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                        {
                            while(end_token.token_type != TOKEN_TYPE_SEMICOLON && end_token.token_type != TOKEN_TYPE_OPEN_BRACE)
                            {
                                end_token = parser_get_next_lexer_token(parser);
                            }
                        }

                        if(end_token.token_type == TOKEN_TYPE_OPEN_BRACE)
                        {
                            while(end_token.token_type != TOKEN_TYPE_CLOSE_BRACE)
                            {
                                end_token = parser_get_next_lexer_token(parser);
                            }
                        }
                    }
                }
                else
                {
                    lexer_eat_lines(&transient_arena, lexer, 1);
                }
            }

            if(pushed_context)
            {
                parser_pop_decl_context(parser);
            }

            // NOTE(Sleepster): Check post closing brace for an identifier (C style)
            token = parser_get_next_lexer_token(parser);
            if(token.token_type == TOKEN_TYPE_IDENT)
            {
                string_t alias_name = c_string_make_copy(&parser->arena, token.data);
                structure_root->identifier = alias_name;

                token = parser_get_next_lexer_token(parser);
            }
            if(token.token_type != TOKEN_TYPE_SEMICOLON)
            {
                report_error(parser,
                             "Finished parsing a structured type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
                             fprint_token(token));
            }

            result = structure_root;
            if(result->identifier.data != null && result->identifier.count > 0)
            {
                code_type_t *type = parser_register_code_type(parser, result->identifier);
                type->type_data = result;

                result->type.code_type = type;
            }

            AST_node_t *node = hash_table_get_element(&parser->active_decl_context->code_decls, result->identifier);
            if(node)
            {
                node->next_overload = result;
            }
            else
            {
                hash_table_add_element(&parser->active_decl_context->code_decls, &result, result->identifier);
            }
        }
        else
        {
            u32 current_depth = 1;
            while(current_depth > 0)
            {
                lexer_token_t token = parser_get_next_lexer_token(parser);
                if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
                {
                    ++current_depth;
                }
                else if(token.token_type == TOKEN_TYPE_CLOSE_BRACE)
                {
                    --current_depth;
                }
            }
        }
    }

    return(result);
}
#else
internal_api void
consume_code_block(parser_t *parser, lexer_token_t *token_out)
{
    lexer_token_t peek_token = parser_get_next_lexer_token(parser);
    // NOTE(Sleepster): Eat all the braces here and return 
    u32 current_depth = 1;
    while(current_depth > 0)
    {
        peek_token = parser_get_next_lexer_token(parser);
        if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
        {
            ++current_depth;
        }
        else if(peek_token.token_type == TOKEN_TYPE_CLOSE_BRACE)
        {
            --current_depth;
        }
    }

    if(token_out)
    {
        *token_out = peek_token;
    }
}

internal_api void
consume_member_lambda_code_block(parser_t *parser, lexer_token_t member_type, lexer_token_t *token_out)
{
    while(member_type.token_type != TOKEN_TYPE_CLOSE_PAREN)
    {
        if(member_type.token_type == TOKEN_TYPE_SEMICOLON)
        {
            report_error(parser,
                         "Expected to find a ')' to match this deconstructors '('... However, we instead found: '%.*s'...\n",
                         fprint_token(member_type));
        }

        // NOTE(Sleepster): This is fine here because you can't pass anything to a deconstructor 
        member_type = parser_get_next_lexer_token(parser);
    }

    lexer_token_t peek_token = parser_peek_next_lexer_token(parser);
    if(peek_token.token_type == TOKEN_TYPE_SEMICOLON)
    {
        // NOTE(Sleepster): Eat the semicolon and return 
        *token_out = parser_get_next_lexer_token(parser);
    }
    else if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        // NOTE(Sleepster): Eat all the braces here and return 
        u32 current_depth = 1;
        while(current_depth > 0)
        {
            peek_token = parser_get_next_lexer_token(parser);
            if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
            {
                ++current_depth;
            }
            else if(peek_token.token_type == TOKEN_TYPE_CLOSE_BRACE)
            {
                --current_depth;
            }
        }

        *token_out = peek_token;
    }
    else
    {
        report_error(parser,
                     "Expected to either find a semicolon or an '{' at the end of this deconstructor's declaration.. instead found: '%.*s'...\n",
                     fprint_token(peek_token));
    }
}

internal_api AST_node_t*
parse_structure_member(parser_t *parser, AST_node_t *structure, lexer_token_t *token_out)
{
    AST_node_t *result = null;
    lexer_token_t member_type = parser_get_next_lexer_token(parser);
    if(member_type.token_type == TOKEN_TYPE_CLOSE_BRACE)
    {
        *token_out = member_type;
        return(result);
    }

    // NOTE(Sleepster): If this token is a '~' it is a deconstructor, return. 
    if(member_type.token_type != TOKEN_TYPE_TILDE)
    {
        // NOTE(Sleepster): Check if this is some sort of C-style embed like:
        // typedef struct blah {
        //     struct blah *next_blah; <- this
        // }blah_t;
        if(member_type.token_type == TOKEN_TYPE_STRUCT || member_type.token_type == TOKEN_TYPE_UNION)
        {
            lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 1);
            if(peek_token.token_type == TOKEN_TYPE_IDENT)
            {
                peek_token = parser_peek_next_lexer_token(parser, 2);
                if(peek_token.token_type == TOKEN_TYPE_ASTERISK)
                {
                    u32 peek_depth = 3;
                    while(peek_token.token_type != TOKEN_TYPE_IDENT && peek_token.token_type != TOKEN_TYPE_SEMICOLON)
                    {
                        if(peek_token.token_type != TOKEN_TYPE_ASTERISK &&
                           peek_token.token_type != TOKEN_TYPE_CONST    &&
                           peek_token.token_type != TOKEN_TYPE_VOLATILE)
                        {
                            report_error(parser,
                                         "Unexpected token when parsing a structure member. Found: '%.*s'...\n",
                                         fprint_token(peek_token));
                        }
                        peek_token = parser_peek_next_lexer_token(parser, peek_depth++);
                    }
                }

                if(peek_token.token_type == TOKEN_TYPE_IDENT)
                {
                    // NOTE(Sleepster): If there is an identifier here for the C-style embed, just eat the struct token. 
                    member_type = parser_get_next_lexer_token(parser);
                }
            }
            else if(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
            {
                // NOTE(Sleepster): Nested structure ... 
                AST_node_t *nested_structure = generate_structure_AST(parser);
                nested_structure->type.flags |= AST_TYPE_MODIFIER_FLAG_NESTED;
                if(nested_structure->type.flags & AST_TYPE_MODIFIER_FLAG_ANONYMOUS)
                {
                    AST_node_t *current_member = nested_structure->struct_decl.first_member;
                    while(current_member)
                    {
                        AST_add_member(structure, current_member);
                        AST_node_t *last_member = current_member;
                        current_member = current_member->next_sibling;

                        // NOTE(Sleepster): We have to wipe out it's next_sibling pointer since it's part of a new structure... 
                        last_member->next_sibling = null;
                    }
                }
                else
                {
                    AST_add_member(structure, nested_structure);
                }

                return(result);
            }
        }
        else
        {
            if(member_type.token_type == TOKEN_TYPE_PUBLIC    ||
               member_type.token_type == TOKEN_TYPE_PRIVATE   ||
               member_type.token_type == TOKEN_TYPE_PROTECTED)
            {
                // NOTE(Sleepster): Eat the semicolon 
                parser_get_next_lexer_token(parser);
                return(result);
            }
        }

        // NOTE(Sleepster): Deal with qualifiers
        u32 type_qualifier_flags = 0;
        language_keyword_t *keyword = get_keyword_from_identifier(member_type.data);
        if(keyword->keyword_id != TOKEN_KEYWORD_STRUCT &&
           keyword->keyword_id != TOKEN_KEYWORD_UNION)
        {
            while(keyword->keyword_id != TOKEN_KEYWORD_INVALID)
            {
                if(keyword->keyword_id == TOKEN_KEYWORD_VOLATILE)
                {
                    type_qualifier_flags |= AST_TYPE_MODIFIER_FLAG_VOLATILE;
                }
                if(keyword->keyword_id == TOKEN_KEYWORD_CONST)
                {
                    type_qualifier_flags |= AST_TYPE_MODIFIER_FLAG_CONST;
                }
                member_type = parser_get_next_lexer_token(parser);
                keyword = get_keyword_from_identifier(member_type.data);
            }
        }
        else
        {
            // NOTE(Sleepster): Nested structure ... 
            AST_node_t *nested_structure  = generate_structure_AST(parser);
            nested_structure->type.flags |= AST_TYPE_MODIFIER_FLAG_NESTED;
            if(nested_structure->type.flags & AST_TYPE_MODIFIER_FLAG_ANONYMOUS)
            {
                for(AST_node_t *current_member = nested_structure->struct_decl.first_member;
                    current_member;
                    current_member = current_member->next_sibling)
                {
                    AST_add_member(structure, current_member);
                }
            }
            else
            {
                AST_add_member(structure, nested_structure);
            }
            return(result);
        }

        u32 pointer_depth = 0;
        lexer_token_t asterisk_token = parser_peek_next_lexer_token(parser, 1);
        if(asterisk_token.token_type == TOKEN_TYPE_ASTERISK)
        {
            while(asterisk_token.token_type == TOKEN_TYPE_ASTERISK)
            {
                asterisk_token = parser_get_next_lexer_token(parser);
                ++pointer_depth;

                lexer_token_t peek = parser_peek_next_lexer_token(parser);
                if(peek.token_type != TOKEN_TYPE_ASTERISK)
                {
                    break;
                }
            }
        }

        // NOTE(Sleepster): If the asterisk token is an open paren then we need to verify 
        // this isn't some intrinsic like: 'alignas()' with an argument of '(some value)' like:
        // 'alignas((64))' as that would evaluate to a lambda, which is incorrect.
        lexer_token_t lambda_peek_token = parser_peek_next_lexer_token(parser, 2);
        if(lambda_peek_token.token_type == TOKEN_TYPE_OPEN_PAREN && 
           asterisk_token.token_type != TOKEN_TYPE_OPEN_PAREN)
        {
            // NOTE(Sleepster): Lambda 
            result = AST_create_new_node(&parser->arena, parser->active_decl_context);
            result = generate_lambda_AST(parser,
                                         member_type,
                                         pointer_depth,
                                        (type_qualifier_flags & AST_TYPE_MODIFIER_FLAG_CONST) ? true : false);

            lexer_token_t end_peek_token = parser_peek_next_lexer_token(parser);
            if(end_peek_token.token_type == TOKEN_TYPE_OPEN_BRACE)
            {
                consume_code_block(parser, null);
            }
            else if(end_peek_token.token_type == TOKEN_TYPE_SEMICOLON)
            {
                parser_get_next_lexer_token(parser);
            }
        }
        else
        {
            // NOTE(Sleepster): Normal member 
            lexer_token_t member_name = parser_get_next_lexer_token(parser);

            // NOTE(Sleepster): This would be a template 
            if(member_name.token_type == TOKEN_TYPE_LESS_THAN)
            {
                member_name = parser_get_next_lexer_token(parser);
                // NOTE(Sleepster): Eat the template params, we don't care. 
                u32 template_depth = 0;
                do {
                    member_name = parser_get_next_lexer_token(parser);
                    if(member_name.token_type == TOKEN_TYPE_LESS_THAN)
                    {
                        ++template_depth;
                    }
                    else if(member_name.token_type == TOKEN_TYPE_GREATER_THAN)
                    {
                        --template_depth;
                    }
                }while(template_depth > 0);

                // NOTE(Sleepster): This gets us the name of the template param 
                member_name = parser_peek_next_lexer_token(parser);
            }
            else if(member_name.token_type == TOKEN_TYPE_OPEN_PAREN)
            {
                // NOTE(Sleepster): Constructor, eat and return
                if(c_string_compare(structure->identifier, member_type.data))
                {
                    consume_member_lambda_code_block(parser, member_type, token_out);
                    return(result);
                }
                else
                {
                    if(c_string_compare(member_type.data, STR("alignas")))
                    {
                        u32 current_depth = 1;
                        // NOTE(Sleepster): Some compiler directive like: 'alignas(64)' or something of the sort.
                        while(current_depth > 0)
                        {
                            member_type = parser_get_next_lexer_token(parser);
                            if(member_type.token_type == TOKEN_TYPE_OPEN_PAREN)
                            {
                                ++current_depth;
                            }
                            else if(member_type.token_type == TOKEN_TYPE_CLOSE_PAREN)
                            {
                                --current_depth;
                            }
                        }

                        result = parse_structure_member(parser, structure, token_out);
                    }
                    else
                    {
                        while(member_type.token_type != TOKEN_TYPE_SEMICOLON)
                        {
                            member_type = parser_get_next_lexer_token(parser);
                        }
                    }

                    return(result);
                }
            }

            result = AST_create_new_node(&parser->arena, parser->active_decl_context);
            if(member_type.token_type == TOKEN_TYPE_IDENT)
            {
                // NOTE(Sleepster): Standard member 
                result->node_type  = AST_NODE_TYPE_STRUCTURE_MEMBER;
                result->identifier = c_string_make_copy(&parser->arena, member_name.data);
                code_type_t *member_code_type = parser_register_code_type(parser, member_type.data);
                result->type.code_type        = member_code_type;

                result->type.pointer_depth = pointer_depth;
                result->type.flags         = type_qualifier_flags;
                if(pointer_depth > 0)
                {
                    result->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                }

                // NOTE(Sleepster): Eat the semicolon, if it is not a semicolon, then check for an array or a default expression value
                lexer_token_t token = parser_get_next_lexer_token(parser);
                if(token.token_type == TOKEN_TYPE_OPEN_BRACKET)
                {
                    result->type.flags |= AST_TYPE_MODIFIER_FLAG_ARRAY;

                    AST_node_t **array = &result->array_data.array_expression;
                    while(token.token_type != TOKEN_TYPE_SEMICOLON)
                    {
                        if(!(*array)) *array = c_arena_push_struct(&parser->arena, AST_node_t);

                        *array = generate_expression_AST(parser, 0, &token);
                        array  = &(*array)->next_sibling;

                        token = parser_get_next_lexer_token(parser);
                    }
                }
                else if(token.token_type == TOKEN_TYPE_EQUALS)
                {
                    // NOTE(Sleepster): Generate the assignment AST 
                    result->expression.info = generate_expression_AST(parser, 0, &token);
                    lexer_token_t semicolon_token = parser_peek_next_lexer_token(parser);
                    if(semicolon_token.token_type == TOKEN_TYPE_SEMICOLON)
                    {
                        token = parser_get_next_lexer_token(parser);
                    }
                }
                else
                {
                    if(token.token_type != TOKEN_TYPE_SEMICOLON)
                    {
                        report_error(parser,
                                     "Expected to find either a ';' or an '[]' array size identifier at the end of this struct member declaration... Instead found: '%.*s'...\n",
                                     fprint_token(token));
                    }
                }

                *token_out = token;
            }
        }
    }
    else
    {
        // NOTE(Sleepster): Deconstructor path. 
        member_type = parser_get_next_lexer_token(parser);
        if(member_type.token_type != TOKEN_TYPE_IDENT)
        {
            report_error(parser,
                         "The only token allowed to follow a '~' is an identifier... instead found: '%.*s'...\n",
                         fprint_token(member_type));
        }

        member_type = parser_get_next_lexer_token(parser);
        if(member_type.token_type != TOKEN_TYPE_OPEN_PAREN)
        {
            report_error(parser,
                         "Expected to find an open paren following this suspected deconstructor definition, instead found: '%.*s'...\n",
                         fprint_token(member_type));
        }

        consume_member_lambda_code_block(parser, member_type, token_out);
    }

    return(result);
}

internal_api AST_node_t*
generate_structure_AST(parser_t *parser)
{
    AST_node_t *result = null;

    // NOTE(Sleepster): Check if this is a templated structure, if it is templated ignore it. 
    for(const auto &attribute: parser->current_attribute_list)
    {
        if(attribute.is_template)
        {
            dynarray_reset(&parser->current_attribute_list);
            return(result);
        }
    }

    u32 structure_AST_flags = 0;
    lexer_token_t structure_name_token = parser_get_next_lexer_token(parser);
    // NOTE(Sleepster): Set a bookmark at the '{' here... Search for a name at the end of the decl. 
    parser_push_bookmark(parser, structure_name_token);
    // NOTE(Sleepster): If this is a valid name, check many things:
    // 1.) Is this a declaration?
    // 2.) Does it inherit from another structure?
    if(structure_name_token.token_type == TOKEN_TYPE_IDENT)
    {
        // NOTE(Sleepster): If there is a brace or colon after the identifier, then we're good. Otherwise leave.
        lexer_token_t peek_token = parser_peek_next_lexer_token(parser);
        if(!(peek_token.token_type == TOKEN_TYPE_OPEN_BRACE ||
             peek_token.token_type == TOKEN_TYPE_COLON))
        {
            return(result);
        }
    }

    string_t structure_name = {};
    if(structure_name_token.token_type == TOKEN_TYPE_IDENT)
    {
        structure_name = structure_name_token.data;
    }

    // NOTE(Sleepster): Check if there is an inheritance ':' marker and eat it. 
    lexer_token_t peek_token = parser_peek_next_lexer_token(parser, 1);
    if(peek_token.token_type == TOKEN_TYPE_COLON)
    {
        while(peek_token.token_type != TOKEN_TYPE_OPEN_BRACE)
        {
            peek_token = parser_get_next_lexer_token(parser);
        }
    }

    consume_code_block(parser, &peek_token);
    peek_token = parser_get_next_lexer_token(parser);
    if(peek_token.token_type == TOKEN_TYPE_IDENT)
    {
        structure_name = peek_token.data;
    }

    parser_pop_bookmark(parser);

    if(structure_name.data == null || structure_name.count == 0)
    {
        structure_AST_flags |= AST_TYPE_MODIFIER_FLAG_ANONYMOUS;
    }

    // NOTE(Sleepster): Check for inheritance data. 
    AST_node_t *inheritance_data = null;
    lexer_token_t token = parser_peek_next_lexer_token(parser);
    if(token.token_type == TOKEN_TYPE_COLON)
    {
        parser_get_next_lexer_token(parser);

        // NOTE(Sleepster): Get the qualifier 
        token = parser_get_next_lexer_token(parser);
        if(token.token_type != TOKEN_TYPE_PUBLIC &&
           token.token_type != TOKEN_TYPE_PRIVATE &&
           token.token_type != TOKEN_TYPE_PROTECTED)
        {
            report_error(parser, 
                         "Attempted to parse a structure of name: '%.*s' and it's inheritance data... Expected either: 'public', 'private', or 'protected'. Instead found: '%.*s'...\n",
                         fprint_token(structure_name_token), fprint_token(token));
        }

        lexer_token_t inherited_struct = parser_get_next_lexer_token(parser);
        if(inherited_struct.token_type != TOKEN_TYPE_IDENT)
        {
            report_error(parser,
                         "Expected to find the name of a structure following a declaration of type: 'struct %.*s: ' instead found: '%.*s'...\n",
                         fprint_token(structure_name_token), inherited_struct);
        }

        inheritance_data            = AST_create_new_node(&parser->arena, parser->active_decl_context);
        inheritance_data->node_type = AST_NODE_TYPE_INHERITANCE_INFO;

        code_type_t *inherited_type = parser_register_code_type(parser ,c_string_make_copy(&parser->arena, inherited_struct.data));
        inheritance_data->inheritance_info.inheritance_type = token.token_type;
        inheritance_data->inheritance_info.inherited_data   = inheritance_data;
        inheritance_data->type.code_type                    = inherited_type;

        token = parser_get_next_lexer_token(parser);
    }

    // NOTE(Sleepster): Parse the structure 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE || structure_name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        if(token.token_type == TOKEN_TYPE_OPEN_BRACE && inheritance_data == null)
        {
            parser_get_next_lexer_token(parser);
        }

        AST_node_t *structure  = AST_create_new_node(&parser->arena, parser->active_decl_context);
        structure->node_type   = AST_NODE_TYPE_STRUCTURE;
        structure->type.flags  = structure_AST_flags;
        structure->line_number = parser->lexer.current_stream->line_number;
        if(structure_name.data != null && structure_name.count > 0)
        {
            structure->identifier  = c_string_make_copy(&parser->arena, structure_name);
        }

        // NOTE(Sleepster): Copy the attributes 
        if(parser->current_attribute_list.used > 0)
        {
            dynarray_copy(&structure->attributes, &parser->current_attribute_list);
            dynarray_reset(&parser->current_attribute_list);
        }

        // NOTE(Sleepster): Handle inheritence 
        if(inheritance_data)
        {
            structure->struct_decl.inherited_type_info = inheritance_data;
        }

        // NOTE(Sleepster): Push it's declaration context 
        if(structure->identifier.data != null && structure->identifier.count > 0)
        {
            declaration_context_t *context = parser_create_declaration_context(parser, 
                                                                               structure->identifier, 
                                                                               parser->active_decl_context);
            parser_push_decl_context(parser, context);
            structure->decl_context = context;
        }

        // NOTE(Sleepster): Parse each of the members, allowing lambdas but ignoring constructor & deconstructors. 
        while(token.token_type != TOKEN_TYPE_CLOSE_BRACE)
        {
            AST_node_t *member = parse_structure_member(parser, structure, &token);
            if(member)
            {
                AST_add_member(structure, member);
            }
        }

        lexer_token_t end_token = parser_get_next_lexer_token(parser);
        if(end_token.token_type == TOKEN_TYPE_IDENT)
        {
            end_token = parser_get_next_lexer_token(parser);
        }

        result = structure;
        if(structure->identifier.data != null && structure->identifier.count > 0)
        {
            parser_pop_decl_context(parser);
        }
    }
    else
    {
        report_error(parser,
                     "Tried to parse a structure, however did not find the '{' where we expected too, instead found: '%.*s'",
                     fprint_token(token));
    }

    if(result->identifier.data != null && result->identifier.count > 0)
    {
        code_type_t *type = parser_register_code_type(parser, result->identifier);
        type->type_data = result;

        result->type.code_type = type;
    }

    AST_node_t *node = hash_table_get_element(&parser->active_decl_context->code_decls, result->identifier);
    if(node)
    {
        node->next_overload = result;
    }
    else
    {
        hash_table_add_element(&parser->active_decl_context->code_decls, &result, result->identifier);
    }

    return(result);
}
#endif

internal_api AST_node_t*
generate_enum_AST(parser_t *parser)
{
    AST_node_t *result = null;

    lexer_token_t token;
    lexer_token_t name_token = parser_get_next_lexer_token(parser);
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        token = parser_get_next_lexer_token(parser);
    }
    else if(name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        token = name_token;
    }

    if(token.token_type != TOKEN_TYPE_OPEN_BRACE && token.token_type != TOKEN_TYPE_SEMICOLON)
    {
        report_error(parser,
                     "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
                     fprint_token(name_token), fprint_token(token));
    }

    // NOTE(Sleepster): Same as a structured type, if it's an open brace it's a definition 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        AST_node_t *enum_root = AST_create_new_node(&parser->arena, parser->active_decl_context);
        enum_root->node_type  = AST_NODE_TYPE_ENUM;

        // NOTE(Sleepster): Copy the attributes 
        if(parser->current_attribute_list.used > 0)
        {
            dynarray_copy(&enum_root->attributes, &parser->current_attribute_list);
            dynarray_reset(&parser->current_attribute_list);
        }

        // NOTE(Sleepster): Record the name of the enum if there is one. 
        code_type_t *enum_type = null;
        if(name_token.token_type == TOKEN_TYPE_IDENT) 
        {
            enum_root->identifier = c_string_make_copy(&parser->arena, name_token.data);
        }
        else 
        {
            enum_root->identifier = c_string_make_copy(&parser->arena, STR("anonymous"));
        }

        for(;;)
        {
            if(token.token_type == TOKEN_TYPE_CLOSE_BRACE) break;

            lexer_token_t enum_member_token = parser_get_next_lexer_token(parser);
            if(enum_member_token.token_type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if(enum_member_token.token_type != TOKEN_TYPE_IDENT)
            {
                report_error(parser, "Expected to find an identifier when parsing type information for an enum... Instead found: '%.*s'...\n", fprint_token(enum_member_token));
            }

            AST_node_t *member = AST_create_new_node(&parser->arena, parser->active_decl_context);
            member->node_type  = AST_NODE_TYPE_ENUM_MEMBER;
            member->identifier = c_string_make_copy(&parser->arena, enum_member_token.data);
            member->type.code_type = enum_type;

            // NOTE(Sleepster): Eat whatever comes after the member name... If it's an '=' then this is an expression 
            token = parser_get_next_lexer_token(parser);
            if(token.token_type == TOKEN_TYPE_EQUALS)
            {
                member->expression.info = generate_expression_AST(parser, 0, &token);
            }
            
            AST_add_member(enum_root, member);
            hash_table_add_element(&parser->active_decl_context->enum_symbols, &member, member->identifier);
        }

        token = parser_get_next_lexer_token(parser);
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            string_t alias_name = c_string_make_copy(&parser->arena, token.data);
            enum_root->identifier = alias_name;

            token = parser_get_next_lexer_token(parser);
        }
        if(token.token_type != TOKEN_TYPE_SEMICOLON)
        {
            report_error(parser,
                         "Finished parsing a enum type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
                         fprint_token(token));
        }

        result = enum_root;
        if(result->identifier.data != null && result->identifier.count > 0)
        {
            code_type_t *type = parser_register_code_type(parser, enum_root->identifier);
            type->type_data = result;

            result->type.code_type = type;
            for(AST_node_t *current_argument = result->struct_decl.first_member;
                current_argument;
                current_argument = current_argument->next_sibling)
            {
                current_argument->type.code_type = type;
            }
        }
    }

    AST_node_t *node = hash_table_get_element(&parser->active_decl_context->code_decls, result->identifier);
    if(node)
    {
        node->next_overload = result;
    }
    else
    {
        hash_table_add_element(&parser->active_decl_context->code_decls, &result, result->identifier);
    }

    return(result);
}

internal_api AST_type_t 
parser_create_lambda_type(parser_t *parser, AST_node_t *expression)
{
    // NOTE(Sleepster): 
    // In this case we don't bother recording an overload or anything. This stage is purely for TYPE REGISTRY
    code_type_t *lambda_type = parser_register_code_type(parser, expression->identifier);

    AST_type_t new_type = {};
    new_type.flags      = AST_TYPE_MODIFIER_FLAG_PROCEDURE;
    new_type.code_type  = lambda_type;

    return(new_type);
}

internal_api AST_node_t* 
generate_lambda_AST(parser_t     *parser, 
                    lexer_token_t return_type_token, 
                    u32           return_type_pointer_depth, 
                    bool8         return_type_is_const)
{
    AST_node_t *lambda = null;

    bool8 parse_lambda = true;
    for(const auto &attribute: parser->current_attribute_list)
    {
        if(attribute.is_template)
        {
            dynarray_reset(&parser->current_attribute_list);
            parse_lambda = false;
        }
    }

    if(parse_lambda)
    {
        AST_node_t *return_type = AST_create_new_node(&parser->arena, parser->active_decl_context);
        return_type->node_type  = AST_NODE_TYPE_LAMBDA_RETURN_TYPE;
        return_type->identifier = c_string_make_copy(&parser->arena, return_type_token.data);

        AST_type_t *return_type_data = &return_type->type;
        return_type_data->code_type  = parser_register_code_type(parser, return_type->identifier);

        return_type_data->flags         = return_type_pointer_depth > 0 ? AST_TYPE_MODIFIER_FLAG_POINTER : 0;
        return_type_data->pointer_depth = return_type_pointer_depth;
        if(return_type_is_const)
        {
            return_type_data->flags |= AST_TYPE_MODIFIER_FLAG_CONST;
        }

        lexer_token_t procedure_name_token = parser_get_next_lexer_token(parser);
        if(procedure_name_token.token_type != TOKEN_TYPE_IDENT)
        {
            for(u32 pointer_index = 0;
                pointer_index < return_type_pointer_depth;
                ++pointer_index)
            {
                procedure_name_token = parser_get_next_lexer_token(parser);
            }
        }

        // NOTE(Sleepster): Generate the lambda_node 
        if(procedure_name_token.token_type != TOKEN_TYPE_IDENT)
        {
            report_error(parser,
                         "Expected to find an identifier for the name of this lambda, instead found: '%.*s'\n",
                         fprint_token(procedure_name_token));
        }

        printf("Found lambda: '%.*s'...\n", fprint_token(procedure_name_token));
        lambda = AST_create_new_node(&parser->arena, parser->active_decl_context);
        lambda->lambda.return_type = return_type;
        if(parser->current_attribute_list.used > 0)
        {
            dynarray_copy(&lambda->attributes, &parser->current_attribute_list);
            dynarray_reset(&parser->current_attribute_list);
        }

        argument_list_t list = parse_argument_list(parser, lambda);
        lambda->lambda.first_argument = list.first_argument;
        lambda->lambda.argument_count = list.argument_count;

        // NOTE(Sleepster): Fill in the data related to the lambda. 
        lambda->node_type  = AST_NODE_TYPE_LAMBDA;
        lambda->identifier = c_string_make_copy(&parser->arena, procedure_name_token.data);
        lambda->type       = parser_create_lambda_type(parser, lambda);

        AST_node_t *node = hash_table_get_element(&parser->active_decl_context->code_decls, lambda->identifier);
        if(node)
        {
            add_AST_overload(node, lambda);
        }
        else
        {
            hash_table_add_element(&parser->active_decl_context->code_decls, &lambda, lambda->identifier);
        }

        // NOTE(Sleepster): If the last token here is an open paren, this is a procedure body,
        // eat the body of the procedure to prevent issues with namespaces.
        lexer_token_t token = lexer_peek_token(&parser->lexer, 1);
        if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
        {
            u32 brace_depth = 0;
            do {
                token = lexer_get_next_token(&parser->lexer);
                if(token.token_type == TOKEN_TYPE_CLOSE_BRACE)
                {
                    --brace_depth;
                }
                else if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
                {
                    ++brace_depth;
                }
            }while(brace_depth > 0);
        }
    }
    else
    {
        lexer_token_t token = parser_get_next_lexer_token(parser);
        while(token.token_type != TOKEN_TYPE_OPEN_BRACE && 
              token.token_type != TOKEN_TYPE_SEMICOLON)
        {
            token = parser_get_next_lexer_token(parser);
        }

        if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
        {
            u32 current_depth = 1;
            while(current_depth > 0)
            {
                token = parser_get_next_lexer_token(parser);
                if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
                {
                    ++current_depth;
                }
                else if(token.token_type == TOKEN_TYPE_CLOSE_BRACE)
                {
                    --current_depth;
                }
            }
        }
    }

    return(lambda);
}

internal_api AST_node_t* 
generate_typedef_AST(parser_t *parser)
{
    AST_node_t *result = null;
    lexer_token_t type_token = parser_get_next_lexer_token(parser);
    switch(type_token.token_type)
    {
        case TOKEN_TYPE_STRUCT:
        case TOKEN_TYPE_UNION:
        {
            result = generate_structure_AST(parser);
        }break;
        case TOKEN_TYPE_ENUM:
        {
            result = generate_enum_AST(parser);
        }break;
        case TOKEN_TYPE_IDENT:
        {
            lexer_token_t alias_token = parser_peek_next_lexer_token(parser);

            u32 pointer_depth = 0;
            while(alias_token.token_type == TOKEN_TYPE_ASTERISK)
            {
                alias_token = parser_peek_next_lexer_token(parser, pointer_depth + 2);
                ++pointer_depth;
            }

            if(alias_token.token_type == TOKEN_TYPE_IDENT)
            {
                lexer_token_t final_token = parser_peek_next_lexer_token(parser, pointer_depth + 2);
                // NOTE(Sleepster): 
                // For something as simple as this typedef expression, 
                // we do not need an AST since that is insanely redundant...
                if(final_token.token_type == TOKEN_TYPE_SEMICOLON)
                {
                    code_type_t *main_type = parser_search_for_code_type(parser, type_token.data);
                    parser_register_code_type(parser, c_string_make_copy(&parser->arena, alias_token.data), main_type);

                    printf("FOUND TYPE ALIAS: '%.*s' OF TYPE: '%.*s'...\n",
                           fprint_token(alias_token), fprint_token(type_token));
                }
                else if(final_token.token_type == TOKEN_TYPE_OPEN_PAREN)
                {
                    // NOTE(Sleepster): Lambda. 
                    result = generate_lambda_AST(parser, 
                                                 type_token, 
                                                 pointer_depth, 
                                                 false);
                }
            }
        }break;
        case TOKEN_TYPE_EOF:
        {
            printf("Ignoring file: '%.*s'...\n", fprint_string(parser->filename));
        }break;
    }

    return(result);
}

internal_api code_attribute_t
create_template_attribute(parser_t *parser)
{
    code_attribute_t result = {};
    result.is_template = true;
    
    argument_list_t list = parse_argument_list(parser, null);
    result.template_data.arguments      = list.first_argument;
    result.template_data.argument_count = list.argument_count;

    return(result);
}

#if 0
internal_api void
AST_handle_macro(parser_t *parser, lexer_token_t token)
{
    s32 index = 0;
    while(index != -1)
    {
        string_t current_line = lexer_eat_lines(&parser->arena, &parser->lexer, 1);
        index = c_string_find_first_char_from_left(current_line, '\\');
    }
    lexer_eat_lines(&parser->arena, &parser->lexer, 1);
}
#endif
