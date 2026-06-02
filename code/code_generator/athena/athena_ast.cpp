/* ========================================================================
   $File: athena_ast.cpp $
   $Date: May 30 2026 12:10 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define AST_TYPE_MODIFIER_FLAGS(X)                                        \
    X(AST_TYPE_MODIFIER_FLAG_NONE,     "AST_TYPE_MODIFIER_FLAG_NONE",     BIT(0)) \
    X(AST_TYPE_MODIFIER_FLAG_POINTER,  "AST_TYPE_MODIFIER_FLAG_POINTER",  BIT(1)) \
    X(AST_TYPE_MODIFIER_FLAG_VOLATILE, "AST_TYPE_MODIFIER_FLAG_VOLATILE", BIT(2)) \
    X(AST_TYPE_MODIFIER_FLAG_CONST,    "AST_TYPE_MODIFIER_FLAG_CONST",    BIT(3)) \
    X(AST_TYPE_MODIFIER_FLAG_STATIC,   "AST_TYPE_MODIFIER_FLAG_STATIC",   BIT(4)) \
    X(AST_TYPE_MODIFIER_FLAG_INLINE,   "AST_TYPE_MODIFIER_FLAG_INLINE",   BIT(5)) \
    X(AST_TYPE_MODIFIER_FLAG_ARRAY,    "AST_TYPE_MODIFIER_FLAG_ARRAY",    BIT(6)) \
    X(AST_TYPE_MODIFIER_FLAG_SIGNED,   "AST_TYPE_MODIFIER_FLAG_SIGNED",   BIT(7)) \
    X(AST_TYPE_MODIFIER_FLAG_FLOAT,    "AST_TYPE_MODIFIER_FLAG_FLOAT",    BIT(8)) \

enum AST_type_flags_t 
{
#define X(enum, string, value) enum = value,
AST_TYPE_MODIFIER_FLAGS(X)
#undef X
};

struct AST_type_t
{
    code_type_t *code_type;
    u32          flags;
    u32          pointer_depth;
    u32          array_size;

    string_t     literal;
    union {
        s32      int_value;
        u32      unsigned_value;
        float32  float32_value;
        float64  float64_value;
        string_t string_value;
    };
};

// TODO(Sleepster): STRUCTURE_MEMBER should probably be just "VARIABLE_DECLARATION"
#define AST_NODE_TYPE_LIST(X) \
    X(AST_NODE_TYPE_EXPRESSION, "AST_NODE_TYPE_EXPRESSION") \
    X(AST_NODE_TYPE_UNARY_EXPRESSION, "AST_NODE_TYPE_UNARY_EXPRESSION") \
    X(AST_NODE_TYPE_BINARY_EXPRESSION, "AST_NODE_TYPE_BINARY_EXPRESSION") \
    X(AST_NODE_TYPE_EXPRESSION_VALUE, "AST_NODE_TYPE_EXPRESSION_VALUE") \
    X(AST_NODE_TYPE_ARITHMATIC_OPERATOR, "AST_NODE_TYPE_ARITHMATIC_OPERATOR") \
    X(AST_NODE_TYPE_BINARY_OPERATOR, "AST_NODE_TYPE_BINARY_OPERATOR") \
    X(AST_NODE_TYPE_ASSIGNMENT, "AST_NODE_TYPE_ASSIGNMENT") \
    X(AST_NODE_TYPE_NUMBER, "AST_NODE_TYPE_NUMBER") \
    X(AST_NODE_TYPE_STRUCTURE,  "AST_NODE_TYPE_STRUCTURE") \
    X(AST_NODE_TYPE_STRUCTURE_MEMBER, "AST_NODE_TYPE_STRUCTURE_MEMBER") \
    X(AST_NODE_TYPE_ENUM,       "AST_NODE_TYPE_ENUM") \
    X(AST_NODE_TYPE_ENUM_MEMBER, "AST_NODE_TYPE_ENUM_MEMBER") \

enum AST_node_type_t
{
#define X(enum, string) enum,
    AST_NODE_TYPE_LIST(X)
#undef X
};

enum AST_node_expression_type_t
{
    AST_NODE_EXPRESSION_TYPE_BINARY,
    AST_NODE_EXPRESSION_TYPE_UNARY
};

struct AST_node_t 
{
    u32         node_type;
    string_t    identifier;

    AST_type_t  type;
    AST_node_t *next_sibling;
    union {
        // NOTE(Sleepster): For structures or enums... 
        AST_node_t *first_child;
        struct {
            u32         operator_type;
            AST_node_t *operand;
        }unary_expression;

        struct {
            u32         operator_type;
            AST_node_t *left;
            AST_node_t *right;
        }binary_expression;

        // NOTE(Sleepster): For members with a default value... 
        struct {
            AST_node_t *info;
        }expression;

        struct {
        }lambda;
    };
};

internal_api AST_node_t* generate_expression_AST(lexer_t *lexer, s32 expression_min_binding_power, lexer_token_t *out_token);


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
AST_add_child(AST_node_t *parent, AST_node_t *next_child)
{
    if(parent->first_child == null)
    {
        parent->first_child = next_child;
    }
    else
    {
        for(AST_node_t *current_child = parent->first_child;
            current_child;
            current_child = current_child->next_sibling)
        {
            if(!current_child->next_sibling)
            {
                current_child->next_sibling = next_child;
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
        value_expression->type.code_type = symbol_table_get_code_type(STR("int"));
        value_expression->type.int_value = c_string_read_int(value_token.data); 
    }
    else if(!signed_value && !is_float)
    {
        value_expression->type.code_type      = symbol_table_get_code_type(STR("unsigned int"));
        value_expression->type.unsigned_value = c_string_read_uint(value_token.data); 
    }
    else if(is_float)
    {
        value_expression->type.code_type      = symbol_table_get_code_type(STR("float"));
        value_expression->type.unsigned_value = c_string_read_float32(value_token.data); 

        lexer_token_t float_token = lexer_peek_token(lexer, 1);
        if(float_token.data.count == 1 && float_token.data.data[0] == 'f')
        {
            symbol_table_get_next_lexer_token(lexer);
        }
    }
    else
    {
        Expect(false, 
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
generate_binary_expression_AST(lexer_token_t *token, AST_node_t *left_node, AST_node_t *right_node)
{
    AST_node_t *result = AST_create_new_node(&permanent_arena);
    result->node_type  = AST_NODE_TYPE_BINARY_EXPRESSION;
    result->binary_expression.operator_type = token->token_type;
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
            case TOKEN_TYPE_IDENT:
            {
            }break;
            case TOKEN_TYPE_OPEN_PAREN:
            {
                // NOTE(Sleepster): Subexpression 
                result = generate_expression_AST(lexer, 0, token);
            }break;
            default: 
            {
                Expect(false, 
                       "Expected either an identifier, a number, or another expression when generating the prefix AST... Instead found: '%.*s'...\n",
                       token->data.count, token->data.data);
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
    AST_node_t *right_expression    = generate_expression_AST(lexer, current_infix_binding_power, token);

    AST_node_t *binary_expression = generate_binary_expression_AST(token, left_hand_expression, right_expression);
    return(binary_expression);
}

internal_api AST_node_t* 
generate_expression_AST(lexer_t *lexer, s32 expression_min_binding_power, lexer_token_t *token_out)
{
    AST_node_t *value_expression = null;

    // NOTE(Sleepster): Is the next token a number? 
#if 1
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
            token = lexer_peek_token(lexer);
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
#else
    lexer_token_t token = symbol_table_get_next_lexer_token(lexer);
    Expect(token.token_type == TOKEN_TYPE_NUMBER || token.token_type == TOKEN_TYPE_DASH || token.token_type == TOKEN_TYPE_OPEN_PAREN, 
           "When parsing an expression, we expected to find a number or an '(' immediately following the '='... instead we found: '%.*s'...\n",
           fprint_token(token));
    lexer_token_t next_token = lexer_peek_token(lexer);
    if(next_token.token_type == TOKEN_TYPE_SEMICOLON || 
       next_token.token_type == TOKEN_TYPE_COMMA || 
       token.token_type == TOKEN_TYPE_DASH ||
      (next_token.data.count == 1 && next_token.data.data[0] == 'f'))
    {
        // NOTE(Sleepster): If this is not a complex expression, just build the number node... 
        build_number_AST_node(lexer, value_expression, token);
    }
    else if(next_token.token_flags & TOKEN_FLAG_BINARY_OPERATOR || token.token_type == TOKEN_TYPE_OPEN_PAREN)
    {
        // NOTE(Sleepster): Eat the peeked_token,  
        symbol_table_get_next_lexer_token(lexer);

        // NOTE(Sleepster): Get the token to the right of the binary operator. 
        lexer_token_t right_token = symbol_table_get_next_lexer_token(lexer);
        Expect(right_token.token_type == TOKEN_TYPE_NUMBER, 
               "When parsing the right side of a binary expression, we expected to find a number... Instead we found: '%.*s'... This behavior is not yet supported...\n", 
               fprint_token(right_token));

        value_expression->expression_type         = AST_NODE_EXPRESSION_TYPE_BINARY;
        value_expression->node_type               = AST_NODE_TYPE_BINARY_OPERATOR;
        value_expression->operation.operator_type = next_token.token_type;

        AST_node_t *left_node  = AST_create_new_node(&permanent_arena);
        AST_node_t *right_node = AST_create_new_node(&permanent_arena);

        build_number_AST_node(lexer, left_node,  token);
        build_number_AST_node(lexer,right_node, right_token);

        value_expression->binary_operation.left  = left_node;
        value_expression->binary_operation.right = right_node;
    }
    else
    {
        Expect(false, 
               "When parsing an assignment expression, we found an invalid token... Token was: '%.*s'... In this position we expected either a number or a binary operation... However we find neither of these... Thus, this behavior is invalid...\n",
               fprint_token(next_token));
    }
#endif

    return(value_expression);
}

internal_api AST_node_t* 
generate_structure_AST(lexer_t *lexer)
{
    AST_node_t *result = null;

    lexer_token_t name_token = symbol_table_get_next_lexer_token(lexer);

    u64 struct_ID = INVALID_ID;
    lexer_token_t token;
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        Expect(name_token.token_type == TOKEN_TYPE_IDENT, "Expected to find the name of the structure after the 'struct' keyword, failed to find that... instead found: '%.*s'...\n",
               fprint_token(name_token));

        struct_ID = register_typename(name_token.data);
        token     = symbol_table_get_next_lexer_token(lexer);
    }
    else if(name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        token = name_token;
    }

    Expect(token.token_type == TOKEN_TYPE_OPEN_BRACE || token.token_type == TOKEN_TYPE_SEMICOLON,
           "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
           fprint_token(name_token), fprint_token(token));

    // NOTE(Sleepster): If it's a brace, it's a definition... otherwise it's just a declaration and we don't care... 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        AST_node_t *structure_root = AST_create_new_node(&permanent_arena);
        structure_root->node_type  = AST_NODE_TYPE_STRUCTURE;

        if(name_token.token_type == TOKEN_TYPE_IDENT) structure_root->identifier = c_string_make_copy(&permanent_arena, name_token.data);
        else                                          structure_root->identifier = STR("anonymous");

        for(;;)
        {
            // TODO(Sleepster): This will NOT handle member functions or namespaces... 
            lexer_token_t typename_token = symbol_table_get_next_lexer_token(lexer);
            if(typename_token.token_type == TOKEN_TYPE_CLOSE_BRACE) break;

            lexer_token_t member_name_token = symbol_table_get_next_lexer_token(lexer);

            // NOTE(Sleepster): Handle type modifier (const or volatile)
            u32 type_modifier_flags = 0;
            language_keyword_t *keyword = symbol_table_get_keyword(typename_token.data);
            do {
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

                    typename_token = symbol_table_get_next_lexer_token(lexer);
                }

                keyword = symbol_table_get_keyword(typename_token.data);
            }while(keyword->keyword_id != TOKEN_KEYWORD_INVALID);

            // NOTE(Sleepster): Handle member pointer 
            u32 pointer_depth = 0;
            if(member_name_token.token_type == TOKEN_TYPE_ASTERISK)
            {
                do {
                    member_name_token = symbol_table_get_next_lexer_token(lexer);
                    ++pointer_depth;
                }while(member_name_token.token_type == TOKEN_TYPE_ASTERISK);
            }

            if(typename_token.token_type == TOKEN_TYPE_IDENT)
            {
                AST_node_t *member_node = AST_create_new_node(&permanent_arena);

                member_node->node_type          = AST_NODE_TYPE_STRUCTURE_MEMBER;
                member_node->identifier         = c_string_make_copy(&permanent_arena, member_name_token.data); 
                member_node->type.code_type     = symbol_table_get_code_type(typename_token.data);
                member_node->type.pointer_depth = pointer_depth;
                member_node->type.flags        |= type_modifier_flags;
                if(pointer_depth > 0)
                {
                    member_node->type.flags |= AST_TYPE_MODIFIER_FLAG_POINTER;
                }

                AST_add_child(structure_root, member_node);
                 
                // NOTE(Sleepster): Eat the semicolon, if it is not a semicolon, then check for an array or a default expression value
                token = symbol_table_get_next_lexer_token(lexer);
                if(token.token_type == TOKEN_TYPE_OPEN_BRACKET)
                {
                    token = symbol_table_get_next_lexer_token(lexer);
                    if(token.token_type == TOKEN_TYPE_OPEN_PAREN)
                    {
                        // TODO(Sleepster): We won't have to do this when we handle expressions... 
                        token = symbol_table_get_next_lexer_token(lexer);

                        // NOTE(Sleepster): Eat the closing paren 
                        symbol_table_get_next_lexer_token(lexer);
                    }
                    Expect(token.token_type == TOKEN_TYPE_NUMBER, 
                           "We have somehow found the token: '%.*s' when parsing the array size of a structure member... this should not happen, it should be a number...\n",
                           fprint_token(token));

                    member_node->type.flags      |= AST_TYPE_MODIFIER_FLAG_ARRAY;
                    member_node->type.array_size  = c_string_read_int(token.data);

                    // NOTE(Sleepster): Eat the ']' then the ';'
                    token = symbol_table_get_next_lexer_token(lexer);
                    Expect(token.token_type == TOKEN_TYPE_CLOSE_BRACKET, "Expected to find token ']' when parsing a structure member array... Instead found: '%.*s'", fprint_token(token));

                    token = symbol_table_get_next_lexer_token(lexer);
                    Expect(token.token_type == TOKEN_TYPE_SEMICOLON, "Expected to find token ';' when parsing a structure member array... Instead found: '%.*s'", fprint_token(token));
                }
                else if(token.token_type == TOKEN_TYPE_EQUALS)
                {
                    // NOTE(Sleepster): Generate the assignment AST 
                    member_node->expression.info = generate_expression_AST(lexer, 0, null);
                }
            }
            else if(typename_token.token_type == TOKEN_TYPE_STRUCT || 
                    typename_token.token_type == TOKEN_TYPE_UNION)
            {
                AST_node_t *nested_structure = generate_structure_AST(lexer);
                AST_add_child(structure_root, nested_structure);
            }
        }
        
        token = symbol_table_get_next_lexer_token(lexer);
        if(token.token_type == TOKEN_TYPE_IDENT)
        {
            register_typename(token.data, struct_ID);
            token = symbol_table_get_next_lexer_token(lexer);
        }
        Expect(token.token_type == TOKEN_TYPE_SEMICOLON, 
               "Finished parsing a structured type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
               fprint_token(token));

        result = structure_root;
    }

    printf("\n\n=========== STRUCTURE_DEFINITION =============\n\n");
    printf("Structure of typename: '%.*s' found!\n", fprint_string(result->identifier));
    printf("Members are:\n");
    for(AST_node_t *current_child = result->first_child;
        current_child;
        current_child = current_child->next_sibling)
    {
        Assert(current_child->type.code_type);
        printf("\t'%.*s' with type: '%.*s'...\n", fprint_string(current_child->identifier), fprint_string(current_child->type.code_type->identifier));
        if(current_child->expression.info)
        {
            printf("Has expression of type: '%s'...\n", print_AST_node_type(current_child->expression.info->node_type));
        }
    }
    printf("==============================================\n\n");

    return(result);
}

internal_api AST_node_t*
generate_enum_AST(lexer_t *lexer)
{
    AST_node_t *result = null;
    u64 enum_ID = 0;

    lexer_token_t token;
    lexer_token_t name_token = symbol_table_get_next_lexer_token(lexer);
    if(name_token.token_type == TOKEN_TYPE_IDENT)
    {
        enum_ID = register_typename(name_token.data);
        token   = symbol_table_get_next_lexer_token(lexer);
    }
    else if(name_token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        token = name_token;
    }

    Expect(token.token_type == TOKEN_TYPE_OPEN_BRACE || token.token_type == TOKEN_TYPE_SEMICOLON,
           "When parsing as structure by the name of: '%.*s' we expected to find either an ending semicolon (for a declaration) or an '{' for the definition, however we found neither of these and instead found: '%.*s'...\n",
           fprint_token(name_token), fprint_token(token));

    // NOTE(Sleepster): Same as a structured type, if it's an open brace it's a definition 
    if(token.token_type == TOKEN_TYPE_OPEN_BRACE)
    {
        AST_node_t *enum_root = AST_create_new_node(&permanent_arena);
        enum_root->node_type = AST_NODE_TYPE_ENUM;

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

            Expect(enum_member_token.token_type == TOKEN_TYPE_IDENT, "Expected to find an identifier when parsing type information for an enum... Instead found: '%.*s'...\n", fprint_token(enum_member_token));

            AST_node_t *member = AST_create_new_node(&permanent_arena);
            member->node_type  = AST_NODE_TYPE_ENUM_MEMBER;
            member->identifier = c_string_make_copy(&permanent_arena, enum_member_token.data);

            AST_add_child(enum_root, member);

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
            register_typename(token.data, enum_ID);
            token = symbol_table_get_next_lexer_token(lexer);
        }
        Expect(token.token_type == TOKEN_TYPE_SEMICOLON, 
               "Finished parsing a enum type and expected a closing ';'... Failed to find that. Instead found, %.*s...\n",
               fprint_token(token));

        result = enum_root;
    }
    printf("\n\n=========== ENUM DEFINITION =============\n\n");
    printf("Enum of name: '%.*s' found!\n", fprint_string(result->identifier));
    printf("Members are:\n");
    for(AST_node_t *current_child = result->first_child;
        current_child;
        current_child = current_child->next_sibling)
    {
        printf("\t'%.*s'...\n", fprint_string(current_child->identifier));
        if(current_child->expression.info)
        {
            printf("Has expression of type: '%s'...\n", print_AST_node_type(current_child->expression.info->node_type));
        }
    }
    printf("==============================================\n\n");

    return(result);
}

internal_api void
generate_typedef_AST(lexer_t *lexer)
{
    lexer_token_t next_token = symbol_table_get_next_lexer_token(lexer);
    switch(next_token.token_type)
    {
        case TOKEN_TYPE_STRUCT:
        case TOKEN_TYPE_UNION:
        {
            generate_structure_AST(lexer);
        }break;
        case TOKEN_TYPE_ENUM:
        {
            generate_enum_AST(lexer);
        }break;
        case TOKEN_TYPE_IDENT:
        {
            // NOTE(Sleepster): For something as simple as this typedef expression, we do not need an AST since that is insanely redundant...
            lexer_token_t peek_token = lexer_peek_token(lexer, 1);
            if(peek_token.token_type == TOKEN_TYPE_IDENT)
            {
                lexer_token_t final_token = lexer_peek_token(lexer, 2);
                Expect(final_token.token_type == TOKEN_TYPE_SEMICOLON, 
                       "We found a typedef expression and we expected it to be formatted like: 'typedef uint32_t u32;' but we failed to find the semicolon at the end of the expression. Instead we found: '%.*s'...\n",
                       fprint_token(final_token));

                u64 main_type_ID = register_typename(next_token.data);
                register_typename(peek_token.data, main_type_ID);

                printf("FOUND TYPE ALIAS: '%.*s' OF TYPE: '%.*s'...\n",
                       fprint_token(peek_token), fprint_token(next_token));

                lexer_eat_lines(&transient_arena, lexer, 1);
            }
        }break;
    }
}

