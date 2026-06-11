#if !defined(ATHENA_AST_H)
/* ========================================================================
   $File: athena_ast.h $
   $Date: June 05 2026 12:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define ATHENA_AST_H
struct AST_node_t;
struct code_type_t;

internal_api AST_node_t* 
generate_lambda_AST(lexer_t      *lexer, 
                    lexer_token_t return_type_token, 
                    u32           return_type_pointer_depth, 
                    bool8         return_type_is_const);

#define AST_TYPE_MODIFIER_FLAGS(X)                                                   \
    X(AST_TYPE_MODIFIER_FLAG_NONE,      "AST_TYPE_MODIFIER_FLAG_NONE",      BIT(0))  \
    X(AST_TYPE_MODIFIER_FLAG_POINTER,   "AST_TYPE_MODIFIER_FLAG_POINTER",   BIT(1))  \
    X(AST_TYPE_MODIFIER_FLAG_VOLATILE,  "AST_TYPE_MODIFIER_FLAG_VOLATILE",  BIT(2))  \
    X(AST_TYPE_MODIFIER_FLAG_CONST,     "AST_TYPE_MODIFIER_FLAG_CONST",     BIT(3))  \
    X(AST_TYPE_MODIFIER_FLAG_STATIC,    "AST_TYPE_MODIFIER_FLAG_STATIC",    BIT(4))  \
    X(AST_TYPE_MODIFIER_FLAG_INLINE,    "AST_TYPE_MODIFIER_FLAG_INLINE",    BIT(5))  \
    X(AST_TYPE_MODIFIER_FLAG_ARRAY,     "AST_TYPE_MODIFIER_FLAG_ARRAY",     BIT(6))  \
    X(AST_TYPE_MODIFIER_FLAG_SIGNED,    "AST_TYPE_MODIFIER_FLAG_SIGNED",    BIT(7))  \
    X(AST_TYPE_MODIFIER_FLAG_FLOAT,     "AST_TYPE_MODIFIER_FLAG_FLOAT",     BIT(8))  \
    X(AST_TYPE_MODIFIER_FLAG_PROCEDURE, "AST_TYPE_MODIFIER_FLAG_PROCEDURE", BIT(9))  \
    X(AST_TYPE_MODIFIER_FLAG_ANONYMOUS, "AST_TYPE_MODIFIER_FLAG_ANONYMOUS", BIT(10)) \

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
        s64      int_value;
        u64      unsigned_value;
        float32  float32_value;
        float64  float64_value;
        string_t string_value;
    };
};

// TODO(Sleepster): STRUCTURE_MEMBER should probably be just "VARIABLE_DECLARATION"
#define AST_NODE_TYPE_LIST(X) \
    X(AST_NODE_TYPE_EXPRESSION, "AST_NODE_TYPE_EXPRESSION") \
    X(AST_NODE_TYPE_LAMBDA, "AST_NODE_TYPE_LAMBDA") \
    X(AST_NODE_TYPE_LAMBDA_ARGUMENT, "AST_NODE_TYPE_LAMBDA_ARGUMENT") \
    X(AST_NODE_TYPE_LAMBDA_RETURN_TYPE, "AST_NODE_TYPE_LAMBDA_RETURN_TYPE") \
    X(AST_NODE_TYPE_CONSTEXPR, "AST_NODE_TYPE_CONSTEXPR") \
    X(AST_NODE_TYPE_UNARY_EXPRESSION, "AST_NODE_TYPE_UNARY_EXPRESSION") \
    X(AST_NODE_TYPE_BINARY_EXPRESSION, "AST_NODE_TYPE_BINARY_EXPRESSION") \
    X(AST_NODE_TYPE_EXPRESSION_VALUE, "AST_NODE_TYPE_EXPRESSION_VALUE") \
    X(AST_NODE_TYPE_ARITHMATIC_OPERATOR, "AST_NODE_TYPE_ARITHMATIC_OPERATOR") \
    X(AST_NODE_TYPE_BINARY_OPERATOR, "AST_NODE_TYPE_BINARY_OPERATOR") \
    X(AST_NODE_TYPE_ASSIGNMENT, "AST_NODE_TYPE_ASSIGNMENT") \
    X(AST_NODE_TYPE_NUMBER, "AST_NODE_TYPE_NUMBER") \
    X(AST_NODE_TYPE_LITERAL, "AST_NODE_TYPE_LITERAL") \
    X(AST_NODE_TYPE_STRUCTURE,  "AST_NODE_TYPE_STRUCTURE") \
    X(AST_NODE_TYPE_STRUCTURE_MEMBER, "AST_NODE_TYPE_STRUCTURE_MEMBER") \
    X(AST_NODE_TYPE_CONSTRUCTOR, "AST_NODE_TYPE_CONSTRUCTOR") \
    X(AST_NODE_TYPE_DECONSTRUCTOR, "AST_NODE_TYPE_DECONSTRUCTOR") \
    X(AST_NODE_TYPE_INHERITANCE_INFO,  "AST_NODE_TYPE_INHERITANCE_INFO") \
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

// TODO(Sleepster): Identifiers like "NULL"
enum AST_expression_value_type_t
{
    AST_EXPRESSION_VALUE_INVALID,
    // NOTE(Sleepster): Ident is something like 'NULL' 
    AST_EXPRESSION_VALUE_IDENT,
    // NOTE(Sleepster): Whereas a literal is something like char *name = "Test name"
    AST_EXPRESSION_VALUE_LITERAL,

    AST_EXPRESSION_VALUE_INT,
    AST_EXPRESSION_VALUE_UNSIGNED,
    AST_EXPRESSION_VALUE_FLOAT,
    AST_EXPRESSION_VALUE_DOUBLE,
};

struct AST_expression_value_t
{
    u32 type;
    union {
        s64      int_value;
        u64      unsigned_value;
        float32  float32_value;
        float64  float64_value;
        string_t identifier_value;
    };
};

// NOTE(Sleepster): 
//
// Make it so that there is a stack of namespaces and structures that is thread local.
//
// Such that we can say:
//
// push -> namespace item_manager {}
//
// push -> structure item_data_t {}
//
// pop -> structure item_data_t {}
//
// push structure item_details_t {}
//
// pop -> namespace item_manager {}

struct AST_node_t 
{
    u32         node_type;
    string_t    identifier;

    AST_type_t  type;
    AST_node_t *next_sibling;
    union {
        // NOTE(Sleepster): For structures or enums... 
        struct {
            AST_node_t *first_member;
            AST_node_t *inherited_type_info;
        }struct_decl;

        struct {
            // NOTE(Sleepster): public or private 
            u32         inheritance_type;
            AST_node_t *inherited_data;
        }inheritance_info;

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
            AST_node_t *return_type;
            AST_node_t *first_argument;
            u32         argument_count;
        }lambda;
    };
};

struct parser_t;

//internal_api AST_node_t* generate_expression_AST(lexer_t *lexer, s32 expression_min_binding_power, lexer_token_t *out_token);
internal_api AST_node_t* generate_expression_AST(parser_t *parser, s32 expression_min_binding_power, lexer_token_t *token_out);
internal_api char* print_AST_node_type(u32 node_type);

#endif // ATHENA_AST_H

