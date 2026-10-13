#if !defined(APOLLO_H)
/* ========================================================================
   $File: apollo.h $
   $Date: October 09 2026 04:04 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define APOLLO_H

/* 
=====================
HASH TABLE IMPL
===================== 
*/

#if defined(APOLLO_IMPLEMENTATION)
#define DYNARRAY_IMPLEMENTATION 
#endif

#include <clang-c/Index.h>
#define APOLLO_TAG(tag_type) [[clang::annotate((tag_type))]]

#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_dynarray.h>

/* NOTE(Sleepster): 
    I normally hate OOP but in this case I have no choice, We'll just have to live with it ig.
    C++ doesn't really allow us to do without it here that easily.
*/

u64
fnv1a_hash(void *data, u32 count)
{
    u64 result = 0;
    local_persist const u64 base_hash = 0xcbf29ce484222325ULL;
    local_persist const u64 FNV_prime = 0x100000001b3ULL;

    // TODO(Sleepster): 4 wide SIMD?? 
    u64 current_hash = base_hash;
    for(u32 key_index = 0;
        key_index < count;
        ++key_index)
    {
        byte key_data = ((byte*)data)[key_index];
        current_hash  = current_hash ^ key_data;
        current_hash  = current_hash * FNV_prime;
    }

    result = current_hash;
    return(result);
}

constexpr u32 MAX_HASH_PEEK_AHEADS = 5;

#define APOLLO_HASH_TABLE_HEAP_ALLOCATE(name) void *(name)(void *allocator, s64 size, void *user_data)
#define APOLLO_HASH_TABLE_HEAP_FREE(name)     void  (name)(void *allocator, void *memory, void *user_data)

typedef APOLLO_HASH_TABLE_HEAP_ALLOCATE(apollo_hash_table_allocate_t);
typedef APOLLO_HASH_TABLE_HEAP_FREE(apollo_hash_table_free_t);

APOLLO_HASH_TABLE_HEAP_ALLOCATE(apollo_hash_table_default_arena_allocator)
{
    (void)user_data;

    void *result = null;
    memory_arena_t *arena = (memory_arena_t*)allocator;

    result = c_arena_push_size(arena, size);

    return(result);
}

// NOTE(Sleepster): Declare this for the key type,
// We're already so deep on the C++ crap, no half measures I suppose
template <typename KeyType>
struct hash_traits_t;

template <>
struct hash_traits_t<string_t>
{
    static u64 
    get_hash_value(const string_t &key)
    {
        u64 result = fnv1a_hash(key.data, key.count);
        return(result);
    }

    static bool8 
    hashes_equal(const string_t &A, const string_t &B)
    {
        bool8 result = false;
        if(A.count == B.count)
        {
            if(memcmp(A.data, B.data, A.count) == 0)
            {
                result = true;
            }
        }

        return(result);
    }
};

template <>
struct hash_traits_t<u64>
{
    static u64 
    get_hash_value(const u64 &key)
    {
        u64 result = 0;
        string_t key_string = {
            .data  = (byte*)&key,
            .count = sizeof(u64)
        };
        result = c_hash_table_hash_key(key_string);

        return(result);
    }

    static bool8 
    hashes_equal(const u64 &A, const u64 &B)
    {
        bool8 result = false;

        u64 keyA = get_hash_value(A);
        u64 keyB = get_hash_value(B);
        if(keyA == keyB)
        {
            result = true;
        }

        return(result);
    }
};

// O(1 * num_buckets)
template <typename ValueType>
struct apollo_hash_element_t
{
    ValueType value;
    u64       raw_hash;
};

template <typename ValueType>
struct apollo_hash_bucket_t
{
    array_t<apollo_hash_element_t<ValueType>>   elements;
    apollo_hash_bucket_t<ValueType>            *next_bucket;
};

template <typename ValueType>
struct apollo_hash_table_iterator_t
{
    dynarray_t<apollo_hash_element_t<ValueType>*> *elements;
    s32                                            current_index;

    void      operator++() {++current_index;}
    ValueType operator*()  {return(elements->items[current_index]->value);}

    bool8
    operator!=(apollo_hash_table_iterator_t<ValueType> &other)
    {
        return(current_index != other.current_index);
    }
};

template <typename                     KeyType, 
          typename                     ValueType,
          s32                          MaxCapacity,
          apollo_hash_table_allocate_t AllocateFn = apollo_hash_table_default_arena_allocator,
          apollo_hash_table_free_t     FreeFn     = (apollo_hash_table_free_t*)null>
struct apollo_hash_table_t
{
    static constexpr s32 bucket_capacity = MaxCapacity;
    apollo_hash_bucket_t<ValueType> first_bucket;
    void *allocator;

    dynarray_t<apollo_hash_element_t<ValueType>*> used_elements;

    apollo_hash_table_iterator_t<ValueType>
    begin()
    {
        apollo_hash_table_iterator_t<ValueType> result = {};
        result.elements      = &used_elements;
        result.current_index =  0;

        return(result);
    }

    apollo_hash_table_iterator_t<ValueType>
    end()
    {
        apollo_hash_table_iterator_t<ValueType> result = {};
        result.elements      = &used_elements;
        result.current_index =  used_elements.used;

        return(result);
    }

    // NOTE(Sleepster): FUNCTIONS 
    void
    hash_bucket_init(apollo_hash_bucket_t<ValueType> *bucket)
    {
        bucket->elements.items = (apollo_hash_element_t<ValueType>*)AllocateFn(allocator, sizeof(apollo_hash_element_t<ValueType>) * bucket_capacity, null);
        bucket->elements.count = bucket_capacity;
        bucket->next_bucket = null;
    }

    apollo_hash_bucket_t<ValueType>*
    append_new_bucket(void)
    {
        apollo_hash_bucket_t<ValueType> *result = (apollo_hash_bucket_t<ValueType>*)AllocateFn(allocator, sizeof(apollo_hash_bucket_t<ValueType>), null);
        hash_bucket_init(result);

        apollo_hash_bucket_t<ValueType> *last_bucket = &first_bucket;
        while(last_bucket->next_bucket != null)
        {
            last_bucket = last_bucket->next_bucket;
        }

        last_bucket->next_bucket = result;
        return(result);
    }

    void 
    init(void *passed_allocator)
    {
        allocator = passed_allocator;
        hash_bucket_init(&first_bucket);
    }

    apollo_hash_element_t<ValueType>*
    get_hash_element(KeyType &key)
    {
        apollo_hash_element_t<ValueType> *result = null;
        u64 key_hash = hash_traits_t<KeyType>::get_hash_value(key);

        u64 hash_index = key_hash % bucket_capacity;

        apollo_hash_bucket_t<ValueType> *current_bucket = &first_bucket;
        while(current_bucket)
        {
            apollo_hash_element_t<ValueType> *found = &current_bucket->elements[hash_index]; 
            if(found->raw_hash != 0 && found->raw_hash != key_hash)
            {
                u32 look_ahead = 0;
                while(look_ahead < MAX_HASH_PEEK_AHEADS)
                {
                    hash_index = (hash_index + 1) % bucket_capacity;
                    ++look_ahead;

                    found = &current_bucket->elements[hash_index]; 
                    if(found->raw_hash == key_hash)
                    {
                        result = found;
                        goto return_value;
                    }
                }
                current_bucket = current_bucket->next_bucket;
            }
            else
            {
                result = found;
                break;
            }
        }

return_value:
        return(result);
    }

    apollo_hash_element_t<ValueType>*
    get_or_create_hash_element(KeyType &key)
    {
        apollo_hash_element_t<ValueType> *result = get_hash_element(key);
        if(!result)
        {
            u64 key_hash   = hash_traits_t<KeyType>::get_hash_value(key);
            u64 hash_index = key_hash % bucket_capacity;

            apollo_hash_bucket_t<ValueType> *current_bucket = &first_bucket;
            while(current_bucket)
            {
                apollo_hash_element_t<ValueType> *found = &current_bucket->elements[hash_index];
                if(found->raw_hash != 0)
                {
                    hash_index = (hash_index + 1) % bucket_capacity;
                    if(hash_index == 0)
                    {
                        current_bucket = current_bucket->next_bucket;
                    }
                }
                else
                {
                    result = found;
                    break;
                }
            }

            if(!result)
            {
                // NOTE(Sleepster): New bucket, the rest are full 
                current_bucket = append_new_bucket();
                result = &current_bucket->elements[key_hash % bucket_capacity];
            }
        }

        return(result);
    }

    ValueType
    find(KeyType &key)
    {
        ValueType result = {};

        apollo_hash_element_t<ValueType> *element = get_hash_element(key);
        if(element && element->raw_hash != 0)
        {
            result = element->value;
        }

        return(result);
    }

    ValueType*
    find_pointer(KeyType &key)
    {
        ValueType *result = null; 

        apollo_hash_element_t<ValueType> *element = get_hash_element(key);
        if(element && element->raw_hash != 0)
        {
            result = &element->value;
        }

        return(result);
    }

    // TODO(Sleepster): If we use this elsewhere we should make this a ref again. The reason it isn't right now
    // is because there's an issue with pointers I'm too lazy to solve where we can't simply take a ref
    // to a pointer because the C++ compiler complains that it can't store a temporary copy to a memory location.
    // Actually a very valid and true complaint which is why we just take this by value here. We'll need an actual
    // solution if we use this table elsewhere.
    void 
    add(ValueType value, KeyType &key)
    {
        apollo_hash_element_t<ValueType> *element = get_or_create_hash_element(key);

        u64 key_hash = hash_traits_t<KeyType>::get_hash_value(key);
        element->raw_hash = key_hash;
        element->value    = value;

        c_dynarray_add(&used_elements, &element);
    }
};

/* 
===================================
APOLLO TYPES AND UTILITIES
===================================
*/

#if defined(APOLLO_IMPLEMENTATION)

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#include <c_program_flag_handler.h>
#endif

struct code_decl_structure_t;
struct code_decl_procedure_t;

enum expression_value_type_t
{
    EXPRESSION_VALUE_TYPE_INVALID,
    EXPRESSION_VALUE_TYPE_BOOLEAN,
    EXPRESSION_VALUE_TYPE_SIGNED_LONG,
    EXPRESSION_VALUE_TYPE_UNSIGNED_LONG,
    EXPRESSION_VALUE_TYPE_FLOAT,
    EXPRESSION_VALUE_TYPE_DOUBLE,
    EXPRESSION_VALUE_TYPE_STRING_LITERAL,
    EXPRESSION_VALUE_TYPE_SOURCE_TEXT,
};

struct expression_value_t
{
    s32 type;
    union {
        bool8  boolean_value;
        s64    signed_long;
        u64    unsigned_long;
        float  float_value;
        double double_value;
        string_t string;
    };
};

struct inheritance_decl_info_t
{
    const char           *identifier;
    CXType                inherited_type;
    CXCursor              base_type_declaration;
    CX_CXXAccessSpecifier access_type;
    bool8                 is_virtual;
};

enum code_decl_metatype_t
{
    CODE_DECL_METATYPE_INVALID,
    CODE_DECL_METATYPE_STRUCTURE,
    CODE_DECL_METATYPE_ENUM,
    CODE_DECL_METATYPE_LAMBDA,
};

enum 
{
    CODE_DECL_FLAG_INVALID = BIT(0),
    CODE_DECL_FLAG_VOLATILE = BIT(1),
    CODE_DECL_FLAG_POINTER = BIT(2),
    CODE_DECL_FLAG_CONST = BIT(3)
};

struct code_decl_t
{
    string_t identifier;

    CXType   code_type;
    CXType   base_type; // for pointers
    CXCursor decl;

    s32      metatype;
    s32      pointer_depth;
    s32      flags;

    dynarray_t<string_t> attributes;
};

struct code_decl_array_info_t: public code_decl_t
{
    CXType array_type;
    s32    array_size;
    code_decl_array_info_t *next_dimension;
};

struct code_decl_member_t: public code_decl_t
{
    u32         size;
    u32         offset;
    const char *identifier;

    code_decl_structure_t *nested_struct;
    code_decl_procedure_t *member_function;
    expression_value_t     value;
    bool8                  has_value;
    struct {
        s32                     dimension_count;
        code_decl_array_info_t *first_dimension;
    }array_info;
};

struct code_decl_structure_t: public code_decl_t
{
    bool8 nested_structure;

    dynarray_t<code_decl_member_t>       members;
    dynarray_t<inheritance_decl_info_t*> inheritance_info;
};

struct code_decl_procedure_t: public code_decl_t
{
    dynarray_t<code_decl_member_t> arguments;
    CXType                         return_type;
};

struct apollo_procedure_argument_t
{
    CXType   type;
    string_t identifier;
};

struct apollo_procedure_call_expression_t
{
    string_t identifier;
    string_t filename;
    u32      line_number;

    dynarray_t<apollo_procedure_argument_t> arguments;
};

using apollo_decl_table_t = apollo_hash_table_t<string_t, code_decl_t *, 2048>;
struct apollo_code_decl_list_t
{
    // NOTE(Sleepster): Includes sturctures, enums, and lambdas.     
    apollo_decl_table_t       all;

    dynarray_t<code_decl_t*> structures;
    dynarray_t<code_decl_t*> enums;
    dynarray_t<code_decl_t*> procedures;

    // NOTE(Sleepster): These are used by the Apollo::lookup functions 
    dynarray_t<apollo_procedure_call_expression_t> call_expressions;
};

struct type_declaration_t
{
    CXType       type;
    code_decl_t *declaration_info;
};

struct apollo_state_t
{
    memory_arena_t         arena;
    CXIndex                clang_index;

    // NOTE(Sleepster): Translation unit
    CXTranslationUnit      working_TU;
    CXCursor               TU_cursor;

    apollo_code_decl_list_t code_declarations;
};

global apollo_state_t *apollo_internal_state;

/*
==============================
TYPE ALIASES & NAMESPACES
==============================
*/

internal_api void collect_attributes(CXCursor cursor, code_decl_t *declaration);

internal_api void
register_type_alias(CXCursor current_cursor)
{
    CXType underlying_type   = clang_getTypedefDeclUnderlyingType(current_cursor);
    CXString type_alias_name = clang_getCursorSpelling(current_cursor);

    string_t typename_key = STR(clang_getCString(type_alias_name));

    string_t typename_alias = c_string_make_copy(&apollo_internal_state->arena, STR(clang_getCString(type_alias_name)));
    CXCursor declaration_info = clang_getTypeDeclaration(underlying_type);
    if(declaration_info.kind == CXCursor_StructDecl ||
       declaration_info.kind == CXCursor_UnionDecl  ||
       declaration_info.kind == CXCursor_EnumDecl)
    {
        // NOTE(Sleepster): Set the typename to be correct, this way we correctly override names like
        // typedef struct string {}string_t;
        // to that of string_t rather than that of just string.
        string_t alias_typename = STR(clang_getCString(clang_getCursorSpelling(declaration_info)));
        code_decl_t *lookup = apollo_internal_state->code_declarations.all.find(alias_typename);
        if(lookup)
        {
            // NOTE(Sleepster): We have to collect attributes on this typedef otherwise a:
            // typedef struct string {}string_t 
            // will lack any attributes.
            lookup->identifier = typename_alias;
            collect_attributes(current_cursor, lookup);
        }
    }

    code_decl_t *type = apollo_internal_state->code_declarations.all.find(typename_key);
    if(!type)
    {
        code_decl_t *new_declaration = c_arena_push_struct(&apollo_internal_state->arena, code_decl_t);
        new_declaration->code_type  = underlying_type;
        new_declaration->decl       = current_cursor;
        new_declaration->identifier = typename_alias; 

        apollo_internal_state->code_declarations.all.add(new_declaration, typename_key);
    }
    else
    {
        clang_disposeString(type_alias_name);
    }
}

internal_api void
register_type(CXType type)
{
    if(type.kind == CXType_Pointer ||
       type.kind == CXType_BlockPointer)
    {
        type = clang_getPointeeType(type);
    }

    CXString type_name = clang_getTypeSpelling(type);
    string_t typename_key = STR(clang_getCString(type_name));

    code_decl_t *found = apollo_internal_state->code_declarations.all.find(typename_key);
    if(!found)
    {
        code_decl_t *new_type = c_arena_push_struct(&apollo_internal_state->arena, code_decl_t);
        new_type->code_type  = type;
        new_type->identifier = typename_key;

        apollo_internal_state->code_declarations.all.add(new_type, typename_key);
    }
    else
    {
        clang_disposeString(type_name);
    }
}

/*
==========================
ATTRIBUTE STUFF
==========================
*/

internal_api void
collect_attributes(CXCursor cursor, code_decl_t *declaration)
{
    clang_visitChildren(cursor, 
    [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
        code_decl_t *code_decl = static_cast<code_decl_t*>(user_data);    
        switch(clang_getCursorKind(current_cursor))
        {
            case CXCursor_AnnotateAttr:
            {
                CXString attribute_string = clang_getCursorSpelling(current_cursor);
                const char *attribute_content = clang_getCString(attribute_string);

                string_t attribute_data = c_string_make_copy(&apollo_internal_state->arena, STR(attribute_content));
                c_dynarray_add(&code_decl->attributes, &attribute_data);

                clang_disposeString(attribute_string);
            }break;
        }

        return(CXChildVisit_Continue);
    }, declaration);
}

/*
==========================
DECLARATIONS
==========================
*/

internal_api s32
get_pointer_depth(CXType type, CXType *out_base_type)
{
    s32 depth = 0;
    CXType current = clang_getCanonicalType(type);

    while(current.kind == CXType_Pointer)
    {
        current = clang_getCanonicalType(clang_getPointeeType(current));
        ++depth;
    }

    if(out_base_type) *out_base_type = current;
    return(depth);
}

internal_api bool8 
is_value_expression(CXCursorKind kind)
{
    switch(kind)
    {
        case CXCursor_IntegerLiteral: case CXCursor_FloatingLiteral:
        case CXCursor_StringLiteral: case CXCursor_CharacterLiteral:
        case CXCursor_CXXBoolLiteralExpr: case CXCursor_UnaryOperator:
        case CXCursor_BinaryOperator: case CXCursor_CompoundAssignOperator:
        case CXCursor_ConditionalOperator: case CXCursor_InitListExpr:
        case CXCursor_DeclRefExpr: case CXCursor_MemberRefExpr:
        case CXCursor_ArraySubscriptExpr: case CXCursor_CallExpr:
        case CXCursor_CXXFunctionalCastExpr: case CXCursor_CXXStaticCastExpr:
        case CXCursor_CXXConstCastExpr: case CXCursor_CXXDynamicCastExpr:
        case CXCursor_CXXReinterpretCastExpr: case CXCursor_GNUNullExpr:
        case CXCursor_UnexposedExpr:
        {
            return(true);
        }break;
        default: 
        {
            return(false);
        }break;
    }
}

// NOTE(Sleepster): We can't use this for attribute gathering since this only gives us the text relative
// to that of the cursor passed here, meaning attributes from before and after the cursor cannot be found correctly.
internal_api char *
get_source_text(CXCursor cursor)
{
    CXTranslationUnit translation_unit = clang_Cursor_getTranslationUnit(cursor);
    CXToken *tokens; 
    u32      token_count;
    clang_tokenize(translation_unit, clang_getCursorExtent(cursor), &tokens, &token_count);

    s32 capacity = 1; 
    for(u32 token_index = 0; 
        token_index < token_count; 
        ++token_index)
    {
        CXString token_string = clang_getTokenSpelling(translation_unit, tokens[token_index]);
        capacity += strlen(clang_getCString(token_string)) + 1;
        clang_disposeString(token_string);
    }

    char *result = (char*)c_arena_push_size(&apollo_internal_state->arena, capacity);
    result[0] = '\0';
    for(u32 token_index = 0; 
        token_index < token_count; 
        ++token_index)
    {
        CXString token_string = clang_getTokenSpelling(translation_unit, tokens[token_index]);
        strcat(result, clang_getCString(token_string));
        if(token_index + 1 < token_count) strcat(result, " ");

        clang_disposeString(token_string);
    }
    clang_disposeTokens(translation_unit, tokens, token_count);

    return(result);
}

internal_api CXChildVisitResult 
evaluate_default_expression(CXCursor declaration, CXCursor, CXClientData user_data)
{
    code_decl_member_t *member = static_cast<code_decl_member_t*>(user_data);
    if(is_value_expression(clang_getCursorKind(declaration)))
    {
        CXCursorKind cursor_kind = clang_getCursorKind(declaration);
        if(cursor_kind != CXCursor_CXXBoolLiteralExpr)
        {
            CXEvalResult evaluation = clang_Cursor_Evaluate(declaration);
            if(evaluation)
            {
                CXEvalResultKind result = clang_EvalResult_getKind(evaluation);
                switch(result)
                {
                    case CXEval_Int:
                    {
                        member->has_value = true;

                        bool8 is_unsigned = clang_EvalResult_isUnsignedInt(evaluation);
                        if(is_unsigned)
                        {
                            member->value.type = EXPRESSION_VALUE_TYPE_UNSIGNED_LONG;
                            member->value.unsigned_long = clang_EvalResult_getAsUnsigned(evaluation);
                        }
                        else
                        {
                            member->value.type = EXPRESSION_VALUE_TYPE_SIGNED_LONG;
                            member->value.signed_long = clang_EvalResult_getAsLongLong(evaluation);
                        }
                    }break;
                    case CXEval_Float:
                    {
                        member->has_value = true;
                        float64 double_value = clang_EvalResult_getAsDouble(evaluation);
                        float32 float_value  = static_cast<float32>(double_value);
                        if(float_value == double_value)
                        {
                            member->value.type = EXPRESSION_VALUE_TYPE_FLOAT;
                            member->value.float_value = float_value;
                        }
                        else
                        {
                            member->value.type = EXPRESSION_VALUE_TYPE_DOUBLE;
                            member->value.double_value = double_value;
                        }
                    }break;
                    case CXEval_StrLiteral:
                    {
                        member->has_value = true;

                        const char *string_literal = clang_EvalResult_getAsStr(evaluation);
                        member->value.type   = EXPRESSION_VALUE_TYPE_STRING_LITERAL;
                        member->value.string = c_string_make_copy(&apollo_internal_state->arena, STR((char *)string_literal));
                    }break;
                }

                clang_EvalResult_dispose(evaluation);
            }

            if(!member->has_value)
            {
                if(cursor_kind == CXCursor_DeclRefExpr)
                {
                    member->has_value = true;

                    const char *cursor_spelling = clang_getCString(clang_getCursorSpelling(declaration));
                    member->value.type   = EXPRESSION_VALUE_TYPE_STRING_LITERAL;
                    member->value.string = STR((char *)cursor_spelling);
                }
                else
                {
                    member->has_value    = true;
                    member->value.type   = EXPRESSION_VALUE_TYPE_SOURCE_TEXT;
                    member->value.string = STR(get_source_text(declaration));
                }
            }
        }
        else
        {
            // NOTE(Sleepster): Boolean 
            member->has_value = true;
            member->value.type = EXPRESSION_VALUE_TYPE_BOOLEAN;
            member->value.boolean_value = clang_Cursor_isNull(declaration) ? 0 : 1;
        }
    }
    else
    {
        return(CXChildVisit_Continue);
    }

    return(CXChildVisit_Break);
}

// NOTE(Sleepster): For structure members, lambda arguments, and enum members 
internal_api void
parse_field_declaration(CXCursor declaration, code_decl_member_t *field)
{
    CXType   member_type       = clang_getCursorType(declaration);
    CXString member_identifier = clang_getCursorSpelling(declaration);
    s64      member_size       = clang_Type_getSizeOf(member_type);
    s64      member_offset     = (clang_Cursor_getOffsetOfField(declaration) / 8);

    field->identifier = clang_getCString(member_identifier);
    field->code_type  = member_type;
    field->size       = member_size;
    field->offset     = member_offset;
    field->decl       = declaration;

    s32 flags = 0;
    if(clang_isConstQualifiedType(member_type))
    {
        flags |= CODE_DECL_FLAG_CONST;
    }
    if(clang_isVolatileQualifiedType(member_type))
    {
        flags |= CODE_DECL_FLAG_VOLATILE;
    }

    field->flags = flags;
    collect_attributes(declaration, field);

    bool8 has_constant_array = false;
    CXType cursor_type = clang_getCursorType(declaration);
    switch(cursor_type.kind)
    {
        case CXType_Pointer:
        case CXType_LValueReference:
        {
            CXType pointee_type;
            field->pointer_depth = get_pointer_depth(member_type, &pointee_type);
            field->base_type     = pointee_type;
        }break;
        case CXType_ConstantArray:
        {
            // NOTE(Sleepster): C Array 
            field->array_info.first_dimension = c_arena_push_struct(&apollo_internal_state->arena, code_decl_array_info_t);

            code_decl_array_info_t *current_dimension = field->array_info.first_dimension;
            while(cursor_type.kind == CXType_ConstantArray)
            {
                current_dimension->array_size = clang_getArraySize(cursor_type);
                cursor_type = clang_getArrayElementType(cursor_type);
                current_dimension->array_type = cursor_type;

                ++field->array_info.dimension_count;
                if(cursor_type.kind == CXType_ConstantArray)
                {
                    current_dimension->next_dimension = c_arena_push_struct(&apollo_internal_state->arena, code_decl_array_info_t);
                }
            }

            register_type(cursor_type);
            has_constant_array = true;
        }break;
    }

    if(!has_constant_array)
    {
        clang_visitChildren(declaration, evaluate_default_expression, field);
        register_type(member_type);
    }
}

/*
==========================
STRUCTURES
==========================
*/

internal_api bool8 traverse_structure(CXCursor structure, code_decl_t *code_decl);
internal_api bool8 traverse_function(CXCursor cursor, code_decl_t *code_decl);

internal_api CXChildVisitResult
traverse_members(CXCursor current_cursor, CXCursor previous_parent, CXClientData user_data)
{
    (void)previous_parent;

    code_decl_member_t member = {};
    code_decl_structure_t *output = (code_decl_structure_t *)user_data;

    CXCursorKind cursor_kind = clang_getCursorKind(current_cursor);
    switch(cursor_kind)
    {
        case CXCursor_FieldDecl:
        {
            // NOTE(Sleepster): An elaborated type is sometihng like "struct {}matrices;" 
            parse_field_declaration(current_cursor, &member);

            CXType type = clang_getCursorType(current_cursor);
            if(type.kind == CXType_Record || type.kind == CXType_Elaborated)
            {
                CXCursor declaration = clang_getTypeDeclaration(type);
                if(!clang_Cursor_isNull(declaration) &&
                  ( clang_getCursorKind(declaration) == CXCursor_StructDecl ||
                    clang_getCursorKind(declaration) == CXCursor_UnionDecl))
                {
                    CXString nested_structure_name        = clang_getCursorSpelling(declaration);
                    string_t nested_structure_name_string = STR(clang_getCString(nested_structure_name));

                    code_decl_t *found = apollo_internal_state->code_declarations.all.find(nested_structure_name_string);
                    if(!found)
                    {
                    }
                    else
                    {
                        member.nested_struct = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
                        traverse_structure(declaration, member.nested_struct);
                    }
                }

                member.nested_struct->nested_structure = true;
            }
            c_dynarray_add(&output->members, &member);
        }break;
        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        {
            member.metatype  = CODE_DECL_METATYPE_STRUCTURE;
            member.code_type = clang_getCursorType(current_cursor);
            if(clang_Cursor_isAnonymousRecordDecl(current_cursor))
            {
                return(CXChildVisit_Recurse);
            }

            member.nested_struct = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
            traverse_structure(current_cursor, member.nested_struct);

            member.nested_struct->nested_structure = true;
            c_dynarray_add(&apollo_internal_state->code_declarations.structures, (code_decl_t**)&member.nested_struct);
        }break;
        case CXCursor_CXXMethod:
        {
            member.metatype  = CODE_DECL_METATYPE_LAMBDA;
            member.code_type = clang_getCursorType(current_cursor);

            CXString procedure_type_string = clang_getCursorSpelling(current_cursor);
            member.identifier = clang_getCString(procedure_type_string);

            member.member_function = c_arena_push_struct(&apollo_internal_state->arena, code_decl_procedure_t);
            bool8 add = traverse_function(current_cursor, member.member_function);
            if(add)
            {
                c_dynarray_add(&output->members, &member);
            }
        }break;
        case CXCursor_CXXBaseSpecifier:
        {
            code_decl_structure_t *info = static_cast<code_decl_structure_t *>(user_data);

            inheritance_decl_info_t *inheritance_info = c_arena_push_struct(&apollo_internal_state->arena, inheritance_decl_info_t);
            inheritance_info->inherited_type        = clang_getCursorType(current_cursor);
            inheritance_info->access_type           = clang_getCXXAccessSpecifier(current_cursor);
            inheritance_info->is_virtual            = clang_isVirtualBase(current_cursor);
            inheritance_info->base_type_declaration = clang_getTypeDeclaration(inheritance_info->inherited_type);

            CXString base_name = clang_getTypeSpelling(inheritance_info->inherited_type);
            inheritance_info->identifier = clang_getCString(base_name);

            c_dynarray_add(&info->inheritance_info, &inheritance_info);
        }break;
    }

    return(CXChildVisit_Continue);
}

internal_api bool8 
traverse_structure(CXCursor structure, code_decl_t *code_decl)
{
    bool8 result = true;
    code_decl_structure_t *structure_info = static_cast<code_decl_structure_t*>(code_decl);
    *structure_info = {};

    structure_info->metatype  = CODE_DECL_METATYPE_STRUCTURE;
    structure_info->code_type = clang_getCursorType(structure);
    structure_info->decl      = structure;
    Assert(structure_info->code_type.kind == CXType_Record);

    CXCursor template_info = clang_getSpecializedCursorTemplate(structure);
    bool8 template_instance = !clang_Cursor_isNull(template_info);
    if(template_instance)
    {
        structure = template_info;
    }

    collect_attributes(structure, code_decl);
    CXString typename_string = clang_getCursorSpelling(structure);
    string_t typename_key    = c_string_make_copy(&apollo_internal_state->arena, STR(clang_getCString(typename_string)));

    code_decl_t *type = apollo_internal_state->code_declarations.all.find(typename_key);
    if(!type)
    {
        structure_info->identifier = typename_key;

        clang_visitChildren(structure, traverse_members, structure_info);
        apollo_internal_state->code_declarations.all.add(static_cast<code_decl_t*>(structure_info), typename_key);
    }
    else
    {
        clang_disposeString(typename_string);
        result = false;
    }

    return(result);
}

/*
==========================
FUNCTIONS / CALL EXPRESSIONS 
==========================
*/

internal_api bool8
traverse_function(CXCursor cursor, code_decl_t *code_decl)
{
    bool8 result = true;

    code_decl_procedure_t *procedure_info = static_cast<code_decl_procedure_t *>(code_decl);
    *procedure_info = {};

    CXString procedure_name = clang_getCursorSpelling(cursor);
    CXType   procedure_type = clang_getCursorType(cursor);
    CXType   return_type    = clang_getResultType(procedure_type);    

    collect_attributes(cursor, code_decl);
    CXString procedure_type_string = clang_getCursorUSR(cursor);
    string_t procedure_type_key = STR(clang_getCString(procedure_type_string));

    code_decl_t *type = apollo_internal_state->code_declarations.all.find(procedure_type_key);
    if(!type)
    {
        procedure_info->code_type   = procedure_type; 
        procedure_info->metatype    = CODE_DECL_METATYPE_LAMBDA;
        procedure_info->return_type = return_type;
        procedure_info->decl        = cursor;
        procedure_info->identifier  = STR(clang_getCString(procedure_name));

        s32 argument_count = clang_getNumArgTypes(procedure_type);
        if(argument_count > 0)
        {
            clang_visitChildren(cursor,
            [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
                code_decl_procedure_t *procedure = static_cast<code_decl_procedure_t*>(user_data);
                if(clang_getCursorKind(current_cursor) == CXCursor_ParmDecl) 
                {
                    code_decl_member_t argument = {};
                    parse_field_declaration(current_cursor, &argument);
                    c_dynarray_add(&procedure->arguments, &argument);
                }

                return CXChildVisit_Continue;
            }, procedure_info);
        }

        apollo_internal_state->code_declarations.all.add(static_cast<code_decl_t*>(procedure_info), procedure_type_key);
    }
    else
    {
        clang_disposeString(procedure_type_string);
        clang_disposeString(procedure_name);

        result = false;
    }

    return(result);
}

internal_api CXChildVisitResult 
get_procedure_call_expression_arguments(CXCursor current_cursor, CXCursor, CXClientData user_data)
{
    auto call_expr = static_cast<apollo_procedure_call_expression_t *>(user_data);
    apollo_procedure_argument_t argument = {};

    CXCursorKind kind = clang_getCursorKind(current_cursor);
    switch(kind)
    {
        // NOTE(Sleepster): Passed Variable 
        case CXCursor_DeclRefExpr:
        {
            CXCursor declaration = clang_getCursorReferenced(current_cursor);
            CXCursorKind kind = clang_getCursorKind(declaration);
            if(kind != CXCursor_FunctionDecl && kind != CXCursor_OverloadedDeclRef)
            {
                CXType   variable_type = clang_getCursorType(current_cursor);
                CXString variable_typname = clang_getTypeSpelling(variable_type); 

                argument.identifier = STR(clang_getCString(variable_typname));
                argument.type       = variable_type;

                c_dynarray_add(&call_expr->arguments, &argument);
            }
        }break;
        case CXCursor_MemberRefExpr:
        {
            CXType type             = clang_getCursorType(current_cursor);
            CXString clang_typename = clang_getTypeSpelling(type);

            argument.identifier = STR(clang_getCString(clang_typename));
            argument.type       = type;

            c_dynarray_add(&call_expr->arguments, &argument);
        }break;
        case CXCursor_StringLiteral:
        {
            CXString string_literal = clang_getCursorSpelling(current_cursor);

            string_t clang_string_value = STR(clang_getCString(string_literal));
            string_t string_value       = c_string_make_copy(&apollo_internal_state->arena, clang_string_value);
            
            string_value = c_string_remove_characters(string_value.data, clang_string_value, '\"');

            argument.identifier = string_value;    
            argument.type       = clang_getCursorType(current_cursor);

            c_dynarray_add(&call_expr->arguments, &argument);
        }break;
        default:
        {
            // NOTE(Sleepster): Strings expressed like: STR("blah") don't show up on first pass, so we have to call recursively. 
            if(kind == CXCursor_CallExpr      || kind == CXCursor_CXXFunctionalCastExpr ||
               kind == CXCursor_UnexposedExpr || kind == CXCursor_UnaryOperator)
            {
                return(CXChildVisit_Recurse);
            }
        }break;
    }

    return(CXChildVisit_Continue);
}

internal_api string_t
get_expression_name(CXCursor current_cursor)
{
    // NOTE(Sleepster): If there is a name, we're good. Otherwise we must look deeper into
    // the foleded expression. 
    string_t result = STR(clang_getCString(clang_getCursorSpelling(current_cursor)));
    if(result.count == 0)
    {
        clang_visitChildren(current_cursor,
        [](CXCursor cursor, CXCursor, CXClientData user_data) {
            string_t *expression_name = static_cast<string_t*>(user_data);
            if(cursor.kind == CXCursor_DeclRefExpr)
            {
                CXCursor callee = clang_getCursorReferenced(cursor);
                if(callee.kind == CXCursor_OverloadedDeclRef)
                {
                    *expression_name = STR(clang_getCString(clang_getCursorSpelling(callee)));
                    return(CXChildVisit_Break);
                }
            }

            return(CXChildVisit_Continue);
        }, &result);
    }

    return(result);
}

internal_api void
collect_call_expression_data(CXCursor current_cursor, string_t expression_name_string)
{
    string_t namespace_string = {};
    clang_visitChildren(current_cursor, [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
        if(clang_getCursorKind(current_cursor) == CXCursor_NamespaceRef)
        {
            string_t *namespace_content = static_cast<string_t *>(user_data);
            CXString clang_namespace_string = clang_getCursorSpelling(current_cursor);

            *namespace_content = c_string_concat(&apollo_internal_state->arena, *namespace_content, STR(clang_getCString(clang_namespace_string)));
            *namespace_content  = c_string_concat(&apollo_internal_state->arena, *namespace_content, STR("::"));
        }
        else
        {
            return(CXChildVisit_Recurse);
        }

        return(CXChildVisit_Continue);
    }, &namespace_string);

    string_t qualified_name = c_string_concat(&apollo_internal_state->arena, namespace_string, expression_name_string);

    apollo_procedure_call_expression_t call_expression = {};
    call_expression.identifier = qualified_name;

    CXSourceLocation location = clang_getCursorLocation(current_cursor);

    u32 line_number = 0;
    CXFile location_file;
    clang_getSpellingLocation(location, &location_file, &line_number, null, null);
    string_t filename = STR(clang_getCString(clang_getFileName(location_file)));

    call_expression.filename    = filename;
    call_expression.line_number = line_number;

    // NOTE(Sleepster): Template Parameters 
    CXCursor templated_call = clang_getCursorReferenced(current_cursor);
    s32 template_param_count = clang_Cursor_getNumTemplateArguments(templated_call);  
    if(template_param_count < 0)
    {
        // NOTE(Sleepster): In this case, the template info has been folded into the template
        // expression itself and thus will not appear inside the templated_call cursor itself. 
        // That means we unfortunately must do this terrible thing here with the if/else.
        clang_visitChildren(current_cursor,
        [](CXCursor template_cursor, CXCursor, CXClientData user_data) {
            auto expression = static_cast<apollo_procedure_call_expression_t*>(user_data);
            if(clang_getCursorKind(template_cursor) == CXCursor_TypeRef)
            {
                apollo_procedure_argument_t new_argument = {};
                CXType template_param_type       = clang_getCursorType(template_cursor);
                CXString template_param_typename = clang_getTypeSpelling(template_param_type);

                new_argument.identifier = STR(clang_getCString(template_param_typename));
                new_argument.type       = template_param_type;

                c_dynarray_add(&expression->arguments, &new_argument);
            }
            return(CXChildVisit_Recurse);
        }, &call_expression);
    }
    else
    {
        for(s32 template_param_index = 0;
            template_param_index < template_param_count;
            ++template_param_index)
        {
            apollo_procedure_argument_t new_argument = {};
            switch(clang_Cursor_getTemplateArgumentKind(templated_call, template_param_index))
            {
                case CXTemplateArgumentKind_Type:
                {                
                    CXType template_param_type = clang_Cursor_getTemplateArgumentType(templated_call, template_param_index);
                    CXString template_param_typename = clang_getTypeSpelling(template_param_type);

                    new_argument.identifier = STR(clang_getCString(template_param_typename));
                    new_argument.type       = template_param_type;

                    c_dynarray_add(&call_expression.arguments, &new_argument);
                }break;
                case CXTemplateArgumentKind_Integral:
                {
                    // TODO(Sleepster): Do we care about the actual value? 
                    // Right now, I don't think we do since this is simply meant to capture the types used
                    // rather than their values

                    //s64 value = clang_Cursor_getTemplateArgumentValue(templated_call, template_param_index);
                    CXType argument_type = clang_Cursor_getTemplateArgumentType(templated_call, template_param_index);
                    if(argument_type.kind == CXType_Enum)
                    {
                        CXString enum_typename = clang_getTypeSpelling(argument_type);
                        new_argument.identifier = STR(clang_getCString(enum_typename));
                        new_argument.type       = argument_type;

                        c_dynarray_add(&call_expression.arguments, &new_argument);
                    }
                }break;
            }
        }
    }

    // NOTE(Sleepster): Extract Arguments 
    clang_visitChildren(current_cursor, get_procedure_call_expression_arguments, &call_expression);
    c_dynarray_add(&apollo_internal_state->code_declarations.call_expressions, &call_expression);
}

/*
==========================
ENUMS
==========================
*/

internal_api bool8 
traverse_enum(CXCursor cursor, code_decl_t *code_decl)
{
    bool8 result = true;

    code_decl_structure_t *enum_info = static_cast<code_decl_structure_t*>(code_decl);
    *enum_info = {};

    enum_info->metatype  = CODE_DECL_METATYPE_ENUM;
    enum_info->code_type = clang_getCursorType(cursor);
    enum_info->decl      = cursor;

    CXString typename_string = clang_getCursorSpelling(cursor);
    string_t typename_key    = c_string_make_copy(&apollo_internal_state->arena, STR(clang_getCString(typename_string)));

    code_decl_t *type = apollo_internal_state->code_declarations.all.find(typename_key);
    if(!type)
    {
        enum_info->identifier = typename_key;

        clang_visitChildren(cursor,
        [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
            code_decl_structure_t *enum_info = static_cast<code_decl_structure_t*>(user_data);
            if(clang_getCursorKind(current_cursor) == CXCursor_EnumConstantDecl)
            {
                code_decl_member_t enum_member = {};
                CXString enum_member_identifier = clang_getCursorSpelling(current_cursor);
                enum_member.identifier = clang_getCString(enum_member_identifier);

                enum_member.code_type  = clang_getCursorType(current_cursor);
                enum_member.has_value  = true;
                enum_member.value.type = EXPRESSION_VALUE_TYPE_SIGNED_LONG;
                enum_member.value.signed_long = clang_getEnumConstantDeclValue(current_cursor);

                c_dynarray_add(&enum_info->members, &enum_member);

                return(CXChildVisit_Continue);
            }

            return(CXChildVisit_Recurse);
        }, enum_info);

        apollo_internal_state->code_declarations.all.add(static_cast<code_decl_t*>(enum_info), typename_key);

    }
    else
    {
        clang_disposeString(typename_string);
        result = false;
    }

    return(result);
}

/*
==========================
TRANSLATION UNIT
==========================
*/

internal_api CXChildVisitResult
traverse_translation_unit(CXCursor current_cursor, CXCursor parent, CXClientData user_data)
{
    apollo_code_decl_list_t *decl_list = static_cast<apollo_code_decl_list_t*>(user_data);
    CXCursorKind type = clang_getCursorKind(current_cursor);
    switch(type)
    {
        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        {
            code_decl_structure_t *structure_info = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
            bool8 add = traverse_structure(current_cursor, structure_info);
            if(add)
            {
                c_dynarray_add(&decl_list->structures, (code_decl_t**)&structure_info);
            }
        }break;
        case CXCursor_EnumDecl:
        {
            code_decl_structure_t *enum_decl = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
            bool8 add = traverse_enum(current_cursor, enum_decl);
            if(add)
            {
                c_dynarray_add(&decl_list->enums, (code_decl_t**)&enum_decl);
            }
        }break;
        case CXCursor_FunctionDecl:
        {
            code_decl_procedure_t *procedure_info = c_arena_push_struct(&apollo_internal_state->arena, code_decl_procedure_t);
            bool8 add = traverse_function(current_cursor, procedure_info);
            if(add)
            {
                c_dynarray_add(&decl_list->procedures, (code_decl_t**)&procedure_info);
            }
        }break;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
        {
            register_type_alias(current_cursor);
        }break;
        case CXCursor_CallExpr:
        {
            string_t expression_name_string = get_expression_name(current_cursor);
            collect_call_expression_data(current_cursor, expression_name_string);
        }break;
        // NOTE(Sleepster): Templated Call 
        case CXCursor_UnexposedExpr:
        {
            // NOTE(Sleepster): We check the parent here because the expression has been evaluated in
            // place of some kind of initializer. Thus we must know what the parent initializer
            // actually was to be able to correctly determine if this should be parsed.
            CXCursorKind parent_kind = clang_getCursorKind(parent);
            if(parent_kind == CXCursor_VarDecl || parent_kind == CXCursor_CompoundStmt)
            {
                string_t expression_name_string = get_expression_name(current_cursor);
                if(expression_name_string.count > 0)
                {
                    collect_call_expression_data(current_cursor, expression_name_string);
                }
            }
        }break;
    }

    return(CXChildVisit_Recurse);
}

namespace Apollo {
apollo_code_decl_list_t* 
parse_singular_file(string_t filepath, dynarray_t<string_t> array)
{
    if(!gc)
    {
        c_global_context_init();
    }

    apollo_internal_state = c_arena_bootstrap_allocate_struct(apollo_state_t, arena, MB(100), ALLOCATOR_TAG_STATIC);
    apollo_code_decl_list_t *result = &apollo_internal_state->code_declarations;

    apollo_internal_state->code_declarations.all.init(&apollo_internal_state->arena);
    apollo_internal_state->clang_index = clang_createIndex(0, 1); 
    
    s32 default_argument_count = 2;
    const char **arguments = c_arena_push_array(&apollo_internal_state->arena, const char*, array.used + default_argument_count);
    arguments[0] = "-xc++";
    arguments[1] = "-std=c++14";

    s32 additional_index = default_argument_count;
    for(s32 array_index = 0;
        array_index < array.used;
        ++array_index)
    {
        arguments[array_index + additional_index] = c_string_to_const_array(array[array_index]);
    }

    s32 option_count = additional_index + array.used;
    apollo_internal_state->working_TU = clang_parseTranslationUnit(apollo_internal_state->clang_index,
                                                   C_STR(filepath),
                                                   arguments, 
                                                   option_count, 
                                                   null,
                                                   0,
                                                   CXTranslationUnit_None);
    if(apollo_internal_state->working_TU)
    {
        apollo_internal_state->TU_cursor = clang_getTranslationUnitCursor(apollo_internal_state->working_TU);
        clang_visitChildren(apollo_internal_state->TU_cursor, traverse_translation_unit, &apollo_internal_state->code_declarations);
    }
    else
    {
        fprintf(stderr, "Failure to open this translation unit...\n");
        result = null;
    }

    return(result);
}

// NOTE(Sleepster): We deliberately only support 1 level of inheritance 
bool8 
is_subclass_of(code_decl_structure_t *structure, string_t class_name)
{
    bool8 result = false;
    for(inheritance_decl_info_t *inheritance_info: structure->inheritance_info)
    {
        if(c_string_compare(STR(inheritance_info->identifier), class_name))
        {
            result = true;
            break;
        }
    }

    return(result);
}

bool8 
is_subclass_of(code_decl_t *declaration, string_t class_name)
{
    Assert(declaration->metatype == CODE_DECL_METATYPE_STRUCTURE);
    return(is_subclass_of(static_cast<code_decl_structure_t*>(declaration), class_name));
}

bool8
has_attribute(code_decl_t *declaration, string_t attribute)
{
    bool8 result = false;
    for(string_t &found_attribute: declaration->attributes)
    {
        if(c_string_compare(found_attribute, attribute))
        {
            result = true;
            break;
        }
    }

    return(result);
}

bool8
has_attribute(code_decl_member_t *member, string_t attribute)
{
    return(has_attribute(static_cast<code_decl_t*>(member), attribute));
}

bool8 
has_attribute(code_decl_structure_t *structure, string_t attribute)
{
    return(has_attribute(static_cast<code_decl_t*>(structure), attribute));
}

bool8
has_attribute(code_decl_procedure_t *lambda, string_t attribute)
{
    return(has_attribute(static_cast<code_decl_t*>(lambda), attribute));
}

dynarray_t<apollo_procedure_call_expression_t>
get_procedure_calls(string_t procedure_name)
{
    dynarray_t<apollo_procedure_call_expression_t> result = {};
    for(auto &expression: apollo_internal_state->code_declarations.call_expressions)
    {
        if(c_string_compare(expression.identifier, procedure_name))
        {
            c_dynarray_add(&result, &expression);
        }
    }

    return(result);
}

// NOTE(Sleepster): Enum, or a structure 
code_decl_t*
get_declaration_info(string_t requested_typename)
{
    string_t typename_key = requested_typename;
    code_decl_t *result = apollo_internal_state->code_declarations.all.find(typename_key);
    if(result && (result->metatype != CODE_DECL_METATYPE_STRUCTURE && result->metatype != CODE_DECL_METATYPE_ENUM))
    {
        CXCursor declaration = clang_getTypeDeclaration(result->code_type);
        if(declaration.kind == CXCursor_StructDecl ||
           declaration.kind == CXCursor_UnionDecl  ||
           declaration.kind == CXCursor_EnumDecl)
        {
            CXString clang_name = clang_getCursorSpelling(declaration);
            typename_key = STR(clang_getCString(clang_name));

            code_decl_t *found = apollo_internal_state->code_declarations.all.find(typename_key);
            if(found)
            {
                found = result;
            }

            clang_disposeString(clang_name);
        }
    }

    return(result);
}
} // NAMESPACE APOLLO

#endif // APOLLO_H

