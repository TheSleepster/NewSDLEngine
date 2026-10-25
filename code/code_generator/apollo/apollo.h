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

#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_dynarray.h>

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

/* NOTE(Sleepster): 
    I normally hate OOP but in this case I have no choice, We'll just have to live with it ig.
    C++ doesn't really allow us to do without it here that easily.
*/

// O(1 * num_buckets)
template <typename ValueType, typename KeyType>
struct apollo_hash_element_t
{
    ValueType value;
    KeyType   key;
    u64       raw_hash;
};

template <typename ValueType, typename KeyType>
struct apollo_hash_bucket_t
{
    array_t<apollo_hash_element_t<ValueType, KeyType>> elements;
    apollo_hash_bucket_t<ValueType, KeyType>          *next_bucket;
};

template <typename ValueType, typename KeyType>
struct apollo_hash_table_iterator_t
{
    dynarray_t<apollo_hash_element_t<ValueType, KeyType>*> *elements;
    s32                                                    current_index;

    void      operator++() {++current_index;}
    ValueType operator*()  {return(elements->items[current_index]->value);}

    bool8
    operator!=(apollo_hash_table_iterator_t<ValueType, KeyType> &other)
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
    apollo_hash_bucket_t<ValueType, KeyType> first_bucket;
    void *allocator;

    dynarray_t<apollo_hash_element_t<ValueType, KeyType>*> used_elements;

    apollo_hash_table_iterator_t<ValueType, KeyType>
    begin()
    {
        apollo_hash_table_iterator_t<ValueType, KeyType> result = {};
        result.elements      = &used_elements;
        result.current_index =  0;

        return(result);
    }

    apollo_hash_table_iterator_t<ValueType, KeyType>
    end()
    {
        apollo_hash_table_iterator_t<ValueType, KeyType> result = {};
        result.elements      = &used_elements;
        result.current_index =  used_elements.used;

        return(result);
    }

    // NOTE(Sleepster): FUNCTIONS 
    void
    hash_bucket_init(apollo_hash_bucket_t<ValueType, KeyType> *bucket)
    {
        bucket->elements.items = (apollo_hash_element_t<ValueType, KeyType>*)AllocateFn(allocator, 
                                                                                        sizeof(apollo_hash_element_t<ValueType, KeyType>) * bucket_capacity, 
                                                                                        null);
        bucket->elements.count = bucket_capacity;
        bucket->next_bucket = null;
    }

    apollo_hash_bucket_t<ValueType, KeyType>*
    append_new_bucket(void)
    {
        apollo_hash_bucket_t<ValueType, KeyType> *result = (apollo_hash_bucket_t<ValueType, KeyType>*)AllocateFn(allocator, 
                                                                                                                 sizeof(apollo_hash_bucket_t<ValueType, KeyType>), 
                                                                                                                 null);
        hash_bucket_init(result);

        apollo_hash_bucket_t<ValueType, KeyType> *last_bucket = &first_bucket;
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

    // NOTE(Sleepster): A key is placed at the first free slot at or after its home slot, so
    // lookups have to probe the same way - a bounded peek-ahead makes them disagree and leaves
    // keys sitting in the table that find() can never reach.
    apollo_hash_element_t<ValueType, KeyType>*
    get_hash_element(KeyType &key)
    {
        u64 key_hash   = hash_traits_t<KeyType>::get_hash_value(key);
        u64 hash_index = key_hash % bucket_capacity;

        apollo_hash_bucket_t<ValueType, KeyType> *current_bucket = &first_bucket;
        while(current_bucket)
        {
            for(;hash_index < (u64)bucket_capacity; ++hash_index)
            {
                apollo_hash_element_t<ValueType, KeyType> *found = &current_bucket->elements[hash_index];
                if(found->raw_hash == 0)
                {
                    // NOTE(Sleepster): Free slot, so nothing was ever placed past here either.
                    break;
                }
                if(found->raw_hash == key_hash && hash_traits_t<KeyType>::hashes_equal(found->key, key))
                {
                    return(found);
                }
            }

            current_bucket = current_bucket->next_bucket;
            hash_index = key_hash % bucket_capacity; // Every bucket starts at this key's home slot.
        }

        return(null);
    }

    apollo_hash_element_t<ValueType, KeyType>*
    get_or_create_hash_element(KeyType &key)
    {
        apollo_hash_element_t<ValueType, KeyType> *result = get_hash_element(key);
        if(!result)
        {
            u64 key_hash   = hash_traits_t<KeyType>::get_hash_value(key);
            u64 hash_index = key_hash % bucket_capacity;

            apollo_hash_bucket_t<ValueType, KeyType> *current_bucket = &first_bucket;
            while(current_bucket)
            {
                apollo_hash_element_t<ValueType, KeyType> *found = &current_bucket->elements[hash_index];
                if(found->raw_hash != 0)
                {
                    hash_index = (hash_index + 1) % bucket_capacity;
                    if(hash_index == 0)
                    {
                        current_bucket = current_bucket->next_bucket;
                        hash_index     = key_hash % bucket_capacity;
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

        apollo_hash_element_t<ValueType, KeyType> *element = get_hash_element(key);
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

        apollo_hash_element_t<ValueType, KeyType> *element = get_hash_element(key);
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
        apollo_hash_element_t<ValueType, KeyType> *element = get_or_create_hash_element(key);

        // NOTE(Sleepster): Re-adding a key updates the entry we already have rather than creating
        // a second one, so only brand new entries belong in the iteration list.
        bool8 is_new_element = element->raw_hash == 0;

        u64 key_hash = hash_traits_t<KeyType>::get_hash_value(key);
        element->raw_hash = key_hash;
        element->key       = key;
        element->value     = value;
        if(is_new_element)
        {
            c_dynarray_add(&used_elements, &element);
        }
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
    CODE_DECL_METATYPE_PROCEDURE,
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

    s32      size;
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
    u32 offset;

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

    // NOTE(Sleepster): Set while we walk this declaration's members, so a type that refers back
    // to itself doesn't walk into itself forever.
    bool8 members_collected;

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

// NOTE(Sleepster): Although the general ideal usage is that of
// for(const auto &declaration: declarations)
// {
// }
//
// (where in the above case "declarations" is a passed in pointer of apollo_code_decl_list_t)
// I don't want to explictly permit this since it's implicitly just going to use ALL and you 
// may not know these other options exist.
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
UTILITIES
==============================
*/

string_t
convert(CXString string)
{
    string_t result = {};
    result = STR(clang_getCString(string));

    return(result);
}

/*
==============================
DECLARATION REGISTRATION
==============================
*/

internal_api void collect_attributes(CXCursor cursor, code_decl_t *declaration);
internal_api code_decl_structure_t * traverse_structure(CXCursor cursor, code_decl_t *code_decl);
internal_api code_decl_structure_t * traverse_enum(CXCursor cursor, code_decl_t *code_decl);
internal_api code_decl_procedure_t *  traverse_function(CXCursor cursor, code_decl_t *code_decl);
internal_api code_decl_t *             register_type_alias(CXCursor current_cursor);

// NOTE(Sleepster): A name we've only seen used, a type that exists without a body, and a
// complete type, in that order of usefulness. This is how register_declaration() settles a
// contested key, instead of each caller rolling its own rules.
enum code_decl_priority_t
{
    CODE_DECL_PRIORITY_REFERENCE,
    CODE_DECL_PRIORITY_DECLARATION,
    CODE_DECL_PRIORITY_DEFINITION,
};

internal_api bool8
is_record_cursor(CXCursorKind kind)
{
    return(kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl ||
           kind == CXCursor_UnionDecl  || kind == CXCursor_EnumDecl);
}

internal_api code_decl_priority_t
get_declaration_priority(code_decl_t *declaration)
{
    code_decl_priority_t result = CODE_DECL_PRIORITY_REFERENCE;
    if(declaration->metatype == CODE_DECL_METATYPE_STRUCTURE ||
       declaration->metatype == CODE_DECL_METATYPE_ENUM)
    {
        // NOTE(Sleepster): Typedefs and using-aliases report themselves as definitions, so what
        // the entry we're comparing against *is* is part of the question, not just what we bring.
        result = clang_isCursorDefinition(declaration->decl)
            ? CODE_DECL_PRIORITY_DEFINITION
            : CODE_DECL_PRIORITY_DECLARATION;
    }
    else if(declaration->metatype != CODE_DECL_METATYPE_INVALID)
    {
        result = CODE_DECL_PRIORITY_DEFINITION; // Procedures are keyed by USR, so they're unique.
    }

    return(result);
}

// NOTE(Sleepster): The one place a declaration enters the table. Returns whichever declaration
// owns the key afterwards: ours if we won.
internal_api code_decl_t *
register_declaration(code_decl_t *declaration, string_t key)
{
    apollo_decl_table_t &declarations = apollo_internal_state->code_declarations.all;
    code_decl_t *existing = declarations.find(key);
    if(!existing)
    {
        declarations.add(declaration, key);
        return(declaration);
    }
    if(existing == declaration ||
       get_declaration_priority(existing) >= get_declaration_priority(declaration))
    {
        return(existing);
    }

    dynarray_t<string_t> attributes = existing->attributes;
    if(existing->metatype == CODE_DECL_METATYPE_PROCEDURE)
    {
        *static_cast<code_decl_procedure_t*>(existing) = *(static_cast<code_decl_procedure_t*>(declaration));
    }
    else
    {
        *static_cast<code_decl_structure_t*>(existing) = *(static_cast<code_decl_structure_t*>(declaration));
    }

    existing->attributes = attributes;
    for(string_t &attribute: declaration->attributes)
    {
        c_dynarray_add(&existing->attributes, &attribute);
    }

    return(existing);
}

// NOTE(Sleepster): A name we've seen used with nothing declarable behind it (u32, char, ...).
// Structure shaped so the registrar can upgrade one in place, and bottom of the priority list so
// it can never shadow a real declaration.
internal_api void
register_type_reference(CXType type, string_t key)
{
    code_decl_structure_t *new_reference = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
    new_reference->code_type  = type;
    new_reference->identifier = key;
    new_reference->size       = clang_Type_getSizeOf(type);

    register_declaration(new_reference, key);
}

// NOTE(Sleepster): Registers whatever this cursor declares, or null if we don't track it.
internal_api code_decl_t *
register_cursor(CXCursor cursor)
{
    code_decl_t *result = null;

    CXCursorKind kind = clang_getCursorKind(cursor);
    if(is_record_cursor(kind))
    {
        if(kind == CXCursor_TypedefDecl || kind == CXCursor_TypeAliasDecl)
        {
            result = register_type_alias(cursor);
        }

        code_decl_structure_t *declaration = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);

        result = (kind == CXCursor_EnumDecl)
            ? traverse_enum(cursor, declaration) 
            : traverse_structure(cursor, declaration);
    }

    return(result);
}

internal_api code_decl_t *
register_type_alias(CXCursor current_cursor)
{
    string_t alias_key  = convert(clang_getCursorSpelling(current_cursor));
    CXType   underlying = clang_getTypedefDeclUnderlyingType(current_cursor);

    // NOTE(Sleepster): Canonical is what gets us through an alias of an alias to the type it names.
    code_decl_t *result = register_cursor(clang_getTypeDeclaration(clang_getCanonicalType(underlying)));
    if(result)
    {
        collect_attributes(current_cursor, result);

        // NOTE(Sleepster): The alias is the name the rest of the code actually uses, so it's the one
        // generators print. Both names stay keys for this one declaration.
        result->identifier = alias_key;
        apollo_internal_state->code_declarations.all.add(result, alias_key);
    }
    else
    {
        register_type_reference(underlying, alias_key);
        result = null;
    }

    return(result);
}

// NOTE(Sleepster): A type we saw used on a member, an argument or an array. Canonical gets us
// from 'string_t' to 'struct string'; the key stays the spelling that was actually written.
internal_api void
register_type(CXType type)
{
    string_t typename_key = convert(clang_getTypeSpelling(type));
    if(type.kind == CXType_Pointer ||
       type.kind == CXType_BlockPointer)
    {
        type = clang_getPointeeType(type);
    }

    CXCursor declaration = clang_getTypeDeclaration(clang_getCanonicalType(type));
    if(!is_record_cursor(clang_getCursorKind(declaration)) || !register_cursor(declaration))
    {
        register_type_reference(type, typename_key);
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

                string_t attribute_data = STR(attribute_content);
                c_dynarray_add(&code_decl->attributes, &attribute_data);
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
    CXType current = type;

    while(current.kind == CXType_Pointer)
    {
        current = clang_getPointeeType(current);
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
                        member->value.string = STR((char *)string_literal);
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

    field->identifier = STR(clang_getCString(member_identifier));
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

internal_api code_decl_structure_t * traverse_structure(CXCursor structure, code_decl_t *code_decl);
internal_api code_decl_procedure_t *  traverse_function(CXCursor cursor, code_decl_t *code_decl);

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
                // NOTE(Sleepster): register_cursor() hands back whichever declaration owns this
                // type's key, which is the one to point at even when someone else got there first
                // with a better version of it.
                member.nested_struct = static_cast<code_decl_structure_t*>(register_cursor(clang_getTypeDeclaration(type)));
                if(member.nested_struct)
                {
                    member.nested_struct->nested_structure = true;
                }
            }
            c_dynarray_add(&output->members, &member);
        }break;
        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        {
            member.metatype  = CODE_DECL_METATYPE_STRUCTURE;
            member.code_type = clang_getCursorType(current_cursor);
            member.size      = clang_Type_getSizeOf(member.code_type);
            if(clang_Cursor_isAnonymousRecordDecl(current_cursor))
            {
                return(CXChildVisit_Recurse);
            }

            member.nested_struct = static_cast<code_decl_structure_t*>(register_cursor(current_cursor));
            if(member.nested_struct)
            {
                member.nested_struct->nested_structure = true;
            }
        }break;
        case CXCursor_CXXMethod:
        {
            member.metatype   = CODE_DECL_METATYPE_PROCEDURE;
            member.code_type  = clang_getCursorType(current_cursor);
            member.identifier = convert(clang_getCursorSpelling(current_cursor));
            member.size       = 0;

            member.member_function = traverse_function(current_cursor,
                c_arena_push_struct(&apollo_internal_state->arena, code_decl_procedure_t));
            if(member.member_function)
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

// NOTE(Sleepster): Fills in `code_decl`, registers it, and returns whichever declaration owns
// the type's key - comparing that against what we passed in tells us if we belong in the lists.
internal_api code_decl_structure_t *
traverse_structure(CXCursor structure, code_decl_t *code_decl)
{
    code_decl_structure_t *structure_info = static_cast<code_decl_structure_t*>(code_decl);
    *structure_info = {};

    structure_info->metatype  = CODE_DECL_METATYPE_STRUCTURE;
    structure_info->code_type = clang_getCanonicalType(clang_getCursorType(structure));
    structure_info->decl      = structure;
    structure_info->size      = clang_Type_getSizeOf(structure_info->code_type);

    CXCursor template_info = clang_getSpecializedCursorTemplate(structure);
    if(!clang_Cursor_isNull(template_info))
    {
        structure = template_info;
    }

    collect_attributes(structure, code_decl);
    structure_info->identifier = convert(clang_getCursorSpelling(structure));

    // NOTE(Sleepster): Registering before walking the members is what stops a type that refers
    // back to itself from walking into itself forever.
    code_decl_structure_t *owner = static_cast<code_decl_structure_t*>(
        register_declaration(structure_info, structure_info->identifier));

    // NOTE(Sleepster): The pattern of a class template that's never been instantiated is a
    // record cursor with no record type behind it, and no member data to walk either way.
    if(owner->code_type.kind == CXType_Record && !owner->members_collected)
    {
        owner->members_collected = true;
        clang_visitChildren(structure, traverse_members, owner);
    }

    return(owner);
}

/*
==========================
FUNCTIONS / CALL EXPRESSIONS 
==========================
*/

// NOTE(Sleepster): Same contract as traverse_structure. Procedures are keyed by USR since
// overloads share a name, so they never collide with the type entries.
internal_api code_decl_procedure_t *
traverse_function(CXCursor cursor, code_decl_t *code_decl)
{
    code_decl_procedure_t *procedure_info = static_cast<code_decl_procedure_t *>(code_decl);
    *procedure_info = {};

    CXType procedure_type = clang_getCursorType(cursor);

    collect_attributes(cursor, code_decl);

    procedure_info->code_type   = procedure_type;
    procedure_info->metatype    = CODE_DECL_METATYPE_PROCEDURE;
    procedure_info->return_type = clang_getResultType(procedure_type);
    procedure_info->decl        = cursor;
    procedure_info->identifier  = convert(clang_getCursorSpelling(cursor));
    procedure_info->size        = 0;

    // NOTE(Sleepster): Registering before walking the arguments, as with structures.
    code_decl_procedure_t *owner = static_cast<code_decl_procedure_t*>(register_declaration(procedure_info, convert(clang_getCursorUSR(cursor))));
    if(owner == procedure_info && clang_getNumArgTypes(procedure_type) > 0)
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

    return(owner);
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
            string_t string_value       = clang_string_value;
            
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
    // the folded expression. 
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

// NOTE(Sleepster): Same contract as traverse_structure. Enums now go through the same
// registration path, so a definition overrides a forward declaration like it does for structures.
internal_api code_decl_structure_t *
traverse_enum(CXCursor cursor, code_decl_t *code_decl)
{
    code_decl_structure_t *enum_info = static_cast<code_decl_structure_t*>(code_decl);
    *enum_info = {};

    enum_info->metatype  = CODE_DECL_METATYPE_ENUM;
    enum_info->code_type = clang_getCursorType(cursor);
    enum_info->decl      = cursor;
    enum_info->identifier = convert(clang_getCursorSpelling(cursor));

    code_decl_structure_t *owner = static_cast<code_decl_structure_t*>(register_declaration(enum_info, enum_info->identifier));

    if(!owner->members_collected)
    {
        owner->members_collected = true;

        clang_visitChildren(cursor,
        [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
            code_decl_structure_t *enum_info = static_cast<code_decl_structure_t*>(user_data);
            if(clang_getCursorKind(current_cursor) == CXCursor_EnumConstantDecl)
            {
                CXTypeKind underlying_type_kind = clang_getCanonicalType(clang_getEnumDeclIntegerType(enum_info->decl)).kind;
                bool8 is_unsigned = (underlying_type_kind >= CXType_Char_U && underlying_type_kind <= CXType_UInt128);

                code_decl_member_t enum_member = {};
                enum_member.identifier  = convert(clang_getCursorSpelling(current_cursor));
                enum_member.code_type   = clang_getCursorType(current_cursor);
                enum_member.has_value   = true;
                enum_member.value.type  = is_unsigned 
                                              ? EXPRESSION_VALUE_TYPE_UNSIGNED_LONG 
                                              : EXPRESSION_VALUE_TYPE_SIGNED_LONG;

                if(is_unsigned) enum_member.value.unsigned_long = clang_getEnumConstantDeclUnsignedValue(current_cursor);
                else            enum_member.value.signed_long   = clang_getEnumConstantDeclValue(current_cursor);

                c_dynarray_add(&enum_info->members, &enum_member);

                return(CXChildVisit_Continue);
            }

            return(CXChildVisit_Recurse);
        }, owner);
    }

    return(owner);
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

            // NOTE(Sleepster): Coming back with the declaration we passed in means we own its
            // key and belong in the list, anything else is a duplicate of a better declaration.
            if(traverse_structure(current_cursor, structure_info) == structure_info)
            {
                c_dynarray_add(&decl_list->structures, (code_decl_t**)&structure_info);
            }
        }break;
        case CXCursor_EnumDecl:
        {
            code_decl_structure_t *enum_decl = c_arena_push_struct(&apollo_internal_state->arena, code_decl_structure_t);
            if(traverse_enum(current_cursor, enum_decl) == enum_decl)
            {
                c_dynarray_add(&decl_list->enums, (code_decl_t**)&enum_decl);
            }
        }break;
        case CXCursor_FunctionDecl:
        {
            code_decl_procedure_t *procedure_info = c_arena_push_struct(&apollo_internal_state->arena, code_decl_procedure_t);

            // NOTE(Sleepster): Procedures are keyed by USR, so "am I the owner" is whether the
            // thing that came back from registering is the thing we just filled in.
            if(traverse_function(current_cursor, procedure_info) == procedure_info)
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

dynarray_t<code_decl_t *>
get_declarations_with_attribute(apollo_code_decl_list_t *declarations, string_t attribute)
{
    dynarray_t<code_decl_t *> result = {};
    for(code_decl_t *declaration: declarations->all)
    {
        if(has_attribute(declaration, attribute))
        {
            c_dynarray_add(&result, &declaration);
        }
    }

    return(result);
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

// NOTE(Sleepster): Enum, or a structure. Aliases are registered as keys for the declaration
// they alias, so 'string' and 'string_t' both land on the same structure. The result can still
// be a reference-only declaration, so check the metatype before casting to a structure.
code_decl_t*
get_declaration_info(string_t requested_typename)
{
    return(apollo_internal_state->code_declarations.all.find(requested_typename));
}
} // NAMESPACE APOLLO

#endif // APOLLO_H

