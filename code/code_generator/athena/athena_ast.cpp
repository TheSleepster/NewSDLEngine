/* ========================================================================
   $File: athena_ast.cpp $
   $Date: May 30 2026 12:10 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define AST_TYPE_MODIFIER_FLAGS(X)                                        \
    X(AST_TYPE_MODIFIER_FLAG_POINTER,  "AST_TYPE_MODIFIER_FLAG_POINTER")  \
    X(AST_TYPE_MODIFIER_FLAG_VOLATILE, "AST_TYPE_MODIFIER_FLAG_VOLATILE") \
    X(AST_TYPE_MODIFIER_FLAG_CONST,    "AST_TYPE_MODIFIER_FLAG_CONST")    \
    X(AST_TYPE_MODIFIER_FLAG_STATIC,   "AST_TYPE_MODIFIER_FLAG_STATIC")   \
    X(AST_TYPE_MODIFIER_FLAG_INLINE,   "AST_TYPE_MODIFIER_FLAG_INLINE")   \
    X(AST_TYPE_MODIFIER_FLAG_ARRAY,    "AST_TYPE_MODIFIER_FLAG_ARRAY")

enum AST_type_flags_t 
{
#define X(enum, string) enum,
AST_TYPE_MODIFIER_FLAGS(X)
#undef X
};

struct AST_type_t
{
    code_type_t *code_type;
    u32          flags;
    u32          pointer_depth;
    u32          array_size;
};

#define AST_NODE_TYPE_LIST(X) \
    X(AST_NODE_TYPE_EXPRESSION, "AST_NODE_TYPE_EXPRESSION") \
    X(AST_NODE_TYPE_STRUCTURE,  "AST_NODE_TYPE_EXPRESSION") \
    X(AST_NODE_TYPE_STRUCTURE_MEMBER, "AST_NODE_TYPE_STRUCTURE_MEMBER")

enum AST_node_type_t
{
#define X(enum, string) enum,
    AST_NODE_TYPE_LIST(X)
#undef X
};

struct AST_node_t 
{
    u32         node_type;
    string_t    identifier;

    AST_type_t  type;

    AST_node_t *first_child;
    AST_node_t *next_sibling;
};

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

