/* ========================================================================
   $File: apollo.cpp $
   $Date: October 06 2026 02:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_synchronization.h>

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION 

#include <c_file_api.h>
#include <c_string.h>
#include <c_program_flag_handler.h>
#include <c_math.h>
#include <c_dynarray.h>

#include <p_platform_data.h>

#include <c_heap_allocator.cpp>
#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_global_context.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_threadpool.cpp>

#include "apollo_hash_table.cpp"

#include <stdio.h>
#include <clang-c/Index.h>

struct code_decl_structure_t;
struct code_decl_lambda_t;

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
    CXType   code_type;
    CXType   base_type; // for pointers
    s32      metatype;
    s32      pointer_depth;
    s32      flags;
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

    code_decl_structure_t   *nested_struct;
    code_decl_lambda_t      *member_function;
    expression_value_t       value;
    bool8                    has_value;
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

struct code_decl_lambda_t: public code_decl_t
{
    const char                    *identifier;
    dynarray_t<code_decl_member_t> arguments;
    CXType                         return_type;
};

struct apollo_state_t
{
    memory_arena_t         arena;
    CXIndex                clang_index;

    // NOTE(Sleepster): Translation unit
    CXTranslationUnit      working_TU;
    CXCursor               TU_cursor;

    dynarray_t<code_decl_structure_t> structures;
    dynarray_t<code_decl_structure_t> enums;
    dynarray_t<code_decl_lambda_t>    functions;

    apollo_hash_table_t<CXString, CXType, 4096> type_record;
};

global apollo_state_t *state;

template<>
struct hash_traits_t<CXString>
{
    static u64
    get_hash_value(const CXString &key)
    {
        const char *string = clang_getCString(key);
        u64 result = fnv1a_hash((char*)string, strlen(string));

        return(result);
    }

    static bool8
    hashes_equal(const CXString &A, const CXString &B)
    {
        bool8 result = false;
        const char *Astring = clang_getCString(A);
        const char *Bstring = clang_getCString(B);
            
        u64 Ahash = fnv1a_hash((char*)Astring, strlen(Astring));
        u64 Bhash = fnv1a_hash((char*)Bstring, strlen(Bstring));
        if(Ahash == Bhash)
        {
            result = true;
        }

        return(result);
    }
};

/*
==============================
TYPE ALIASES & NAMESPACES
==============================
*/

void
register_type_alias(CXCursor current_cursor)
{
    CXType underlying_type = clang_getTypedefDeclUnderlyingType(current_cursor);
    CXString type_alias_name = clang_getCursorSpelling(current_cursor);

    CXType *type = state->type_record.get_element(type_alias_name);
    if(!type)
    {
        state->type_record.add_element(underlying_type, type_alias_name);
    }
    else
    {
        clang_disposeString(type_alias_name);
    }
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

    char *result = (char*)c_arena_push_size(&state->arena, capacity);
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
                        member->value.string = c_string_make_copy(&state->arena, STR((char *)string_literal));
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
            field->array_info.first_dimension = c_arena_push_struct(&state->arena, code_decl_array_info_t);

            code_decl_array_info_t *current_dimension = field->array_info.first_dimension;
            while(cursor_type.kind == CXType_ConstantArray)
            {
                current_dimension->array_size = clang_getArraySize(cursor_type);
                cursor_type = clang_getCanonicalType(clang_getArrayElementType(cursor_type));
                current_dimension->array_type = cursor_type;

                ++field->array_info.dimension_count;
                if(cursor_type.kind == CXType_ConstantArray)
                {
                    current_dimension->next_dimension = c_arena_push_struct(&state->arena, code_decl_array_info_t);
                }
            }

            has_constant_array = true;
        }break;
    }

    if(!has_constant_array)
    {
        clang_visitChildren(declaration, evaluate_default_expression, field);
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
            parse_field_declaration(current_cursor, &member);
            c_dynarray_add(&output->members, &member);
        }break;
        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        {
            member.metatype  = CODE_DECL_METATYPE_STRUCTURE;
            member.code_type = clang_getCanonicalType(clang_getCursorType(current_cursor));

            member.nested_struct = c_arena_push_struct(&state->arena, code_decl_structure_t);
            traverse_structure(current_cursor, member.nested_struct);

            member.nested_struct->nested_structure = true;
        }break;
        case CXCursor_CXXMethod:
        {
            member.metatype  = CODE_DECL_METATYPE_LAMBDA;
            member.code_type = clang_getCanonicalType(clang_getCursorType(current_cursor));

            CXString procedure_type_string = clang_getCursorSpelling(current_cursor);
            member.identifier = clang_getCString(procedure_type_string);

            member.member_function = c_arena_push_struct(&state->arena, code_decl_lambda_t);
            bool8 add = traverse_function(current_cursor, member.member_function);
            if(add)
            {
                c_dynarray_add(&output->members, &member);
            }
        }break;
        case CXCursor_CXXBaseSpecifier:
        {
            code_decl_structure_t *info = static_cast<code_decl_structure_t *>(user_data);

            inheritance_decl_info_t *inheritance_info = c_arena_push_struct(&state->arena, inheritance_decl_info_t);
            inheritance_info->inherited_type        = clang_getCanonicalType(clang_getCursorType(current_cursor));
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
    structure_info->code_type = clang_getCanonicalType(clang_getCursorType(structure));

    CXString typename_string = clang_getCursorSpelling(structure);

    CXType *type = state->type_record.get_element(typename_string);
    if(!type)
    {
        clang_visitChildren(structure, traverse_members, structure_info);
        state->type_record.add_element(structure_info->code_type, typename_string);
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
FUNCTIONS / LAMBDAS
==========================
*/

internal_api bool8
traverse_function(CXCursor cursor, code_decl_t *code_decl)
{
    bool8 result = true;

    code_decl_lambda_t *procedure_info = static_cast<code_decl_lambda_t *>(code_decl);
    *procedure_info = {};

    CXString procedure_name = clang_getCursorSpelling(cursor);
    CXType   procedure_type = clang_getCursorType(cursor);
    CXType   return_type    = clang_getResultType(procedure_type);

    CXString procedure_type_string = clang_getCursorUSR(cursor);
    CXType *type = state->type_record.get_element(procedure_type_string);
    if(!type)
    {
        procedure_info->code_type   = procedure_type; 
        procedure_info->metatype    = CODE_DECL_METATYPE_LAMBDA;
        procedure_info->return_type = return_type;
        procedure_info->identifier  = clang_getCString(procedure_name);

        s32 argument_count = clang_getNumArgTypes(procedure_type);
        if(argument_count > 0)
        {
            clang_visitChildren(cursor,
            [](CXCursor current_cursor, CXCursor, CXClientData user_data) {
                code_decl_lambda_t *procedure = static_cast<code_decl_lambda_t*>(user_data);
                if(clang_getCursorKind(current_cursor) == CXCursor_ParmDecl) 
                {
                    code_decl_member_t argument = {};
                    parse_field_declaration(current_cursor, &argument);
                    c_dynarray_add(&procedure->arguments, &argument);
                }

                return CXChildVisit_Continue;
            }, procedure_info);
        }

        state->type_record.add_element(procedure_info->code_type, procedure_type_string);
    }
    else
    {
        clang_disposeString(procedure_type_string);
        clang_disposeString(procedure_name);

        result = false;
    }

    return(result);
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


    CXString typename_string = clang_getCursorSpelling(cursor);
    CXType *type = state->type_record.get_element(typename_string);
    if(!type)
    {
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
traverse_translation_unit(CXCursor current_cursor, CXCursor previous_parent, CXClientData user_data)
{
    (void)user_data;
    (void)previous_parent;

    CXCursorKind type = clang_getCursorKind(current_cursor);
    switch(type)
    {
        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        {
            code_decl_t structure_info;
            bool8 add = traverse_structure(current_cursor, &structure_info);
            if(add)
            {
                c_dynarray_add(&state->structures, static_cast<code_decl_structure_t*>(&structure_info));
            }
        }break;
        case CXCursor_EnumDecl:
        {
            code_decl_t enum_decl;
            bool8 add = traverse_enum(current_cursor, &enum_decl);
            if(add)
            {
                c_dynarray_add(&state->enums, static_cast<code_decl_structure_t*>(&enum_decl));
            }
        }break;
        case CXCursor_FunctionDecl:
        {
            code_decl_t procedure_info;
            bool8 add = traverse_function(current_cursor, &procedure_info);
            if(add)
            {
                c_dynarray_add(&state->functions, static_cast<code_decl_lambda_t*>(&procedure_info));
            }
        }break;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
        {
            register_type_alias(current_cursor);
        }break;
    }

    return(CXChildVisit_Recurse);
}

int
main(int argc, char **argv)
{
    if(argc > 1) 
    {
        printf("\n\n\n\n");
        c_global_context_init();

        state = c_arena_bootstrap_allocate_struct(apollo_state_t, arena, MB(500), ALLOCATOR_TAG_STATIC);

        state->type_record.init(&state->arena);
        state->clang_index = clang_createIndex(0, 1); 
        const char* args[] = {
            "clang++", "-std=c++11",
        };

        state->working_TU = clang_parseTranslationUnit(state->clang_index,
                                                       argv[1],
                                                       args, 
                                                       sizeof(args) / sizeof(char*), 
                                                       null,
                                                       0,
                                                       CXTranslationUnit_None);
        if(state->working_TU)
        {
#if 0
            u32 diagnostic_count = clang_getNumDiagnostics(state->working_TU);
            if(diagnostic_count > 0)
            {
                fprintf(stderr, "libclang Error:\n");
                for(u32 diagnostic_index = 0;
                    diagnostic_index < diagnostic_count;
                    ++diagnostic_index)
                {
                    CXDiagnostic current_diagnostic = clang_getDiagnostic(state->working_TU, diagnostic_index);

                    CXString format_string = clang_formatDiagnostic(current_diagnostic, clang_defaultDiagnosticDisplayOptions());
                    fprintf(stderr, "\t%s\n", clang_getCString(format_string));

                    clang_disposeDiagnostic(current_diagnostic);
                    clang_disposeString(format_string);
                }
            }
#endif

            state->TU_cursor = clang_getTranslationUnitCursor(state->working_TU);
            clang_visitChildren(state->TU_cursor, traverse_translation_unit, null);

            printf("Structures:\n");
            for(code_decl_structure_t &structure: state->structures)
            {
                CXString structure_name = clang_getTypeSpelling(structure.code_type);
                printf("Structure found!: '%s':\n", clang_getCString(structure_name));
                for(s32 member_index = 0;
                    member_index < structure.members.used;
                    ++member_index)
                {
                    code_decl_member_t *member = structure.members + member_index; 
                    printf("\tMember (%d): '%s'\n", member_index, member->identifier);

                    CXString member_typename = clang_getTypeSpelling(member->code_type);
                    printf("\t\ttype:   '%s'\n", clang_getCString(member_typename));
                    printf("\t\tsize:   '%d'\n", member->size);
                    printf("\t\toffset: '%d'\n", member->offset);
                    if(member->has_value)
                    {
                        printf("\t\tMember has default expression:\n");
                        switch(member->value.type)
                        {
                            case EXPRESSION_VALUE_TYPE_SIGNED_LONG:
                            {
                                printf("\t\t\t%lld", (unsigned long long)member->value.signed_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_UNSIGNED_LONG:
                            {
                                printf("\t\t\t%llu", (unsigned long long)member->value.unsigned_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_FLOAT:
                            {
                                printf("\t\t\t%f", member->value.float_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_DOUBLE:
                            {
                                printf("\t\t\t%lf", member->value.double_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_STRING_LITERAL:
                            case EXPRESSION_VALUE_TYPE_SOURCE_TEXT:
                            {
                                printf("\t\t\t%.*s", fprint_string(member->value.string));
                            }break;
                        }
                        printf("\n");
                    }
                }
                printf("\n");
            }

            printf("Enums:\n");
            for(code_decl_structure_t &enum_decl: state->enums)
            {
                CXString enum_name = clang_getTypeSpelling(enum_decl.code_type);
                printf("Enum found!: '%s':\n", clang_getCString(enum_name));
                for(s32 member_index = 0;
                    member_index < enum_decl.members.used;
                    ++member_index)
                {
                    code_decl_member_t *member = enum_decl.members + member_index; 
                    printf("\tEnum Member (%d): '%s'\n", member_index, member->identifier);

                    CXString member_typename = clang_getTypeSpelling(member->code_type);
                    printf("\t\ttype:   '%s'\n", clang_getCString(member_typename));
                    printf("\t\tsize:   '%d'\n", member->size);
                    printf("\t\toffset: '%d'\n", member->offset);
                    if(member->has_value)
                    {
                        printf("\t\tEnum Member has default expression:\n");
                        switch(member->value.type)
                        {
                            case EXPRESSION_VALUE_TYPE_SIGNED_LONG:
                            {
                                printf("\t\t\t%lld", (unsigned long long)member->value.signed_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_UNSIGNED_LONG:
                            {
                                printf("\t\t\t%llu", (unsigned long long)member->value.unsigned_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_FLOAT:
                            {
                                printf("\t\t\t%f", member->value.float_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_DOUBLE:
                            {
                                printf("\t\t\t%lf", member->value.double_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_STRING_LITERAL:
                            case EXPRESSION_VALUE_TYPE_SOURCE_TEXT:
                            {
                                printf("\t\t\t%.*s", fprint_string(member->value.string));
                            }break;
                        }
                        printf("\n");
                    }
                }
                printf("\n");
            }

            printf("Functions: \n");
            for(code_decl_lambda_t &function: state->functions)
            {
                printf("Function of name: '%s', argument_count: '%d', return_type: '%s':\n", 
                       function.identifier, function.arguments.used, clang_getCString(clang_getTypeSpelling(function.return_type)));
                printf("Arguments:\n");
                for(s32 argument_index = 0;
                    argument_index < function.arguments.used;
                    ++argument_index)
                {
                    code_decl_member_t *argument = function.arguments + argument_index;
                    printf("\tArgument (%d): '%s'\n", argument_index, argument->identifier);

                    CXString argument_typename = clang_getTypeSpelling(argument->code_type);
                    printf("\t\ttype:   '%s'\n", clang_getCString(argument_typename));
                    if(argument->has_value)
                    {
                        printf("\t\tMember has default expression:\n");
                        switch(argument->value.type)
                        {
                            case EXPRESSION_VALUE_TYPE_SIGNED_LONG:
                            {
                                printf("\t\t\t%lld", (unsigned long long)argument->value.signed_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_UNSIGNED_LONG:
                            {
                                printf("\t\t\t%llu", (unsigned long long)argument->value.unsigned_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_FLOAT:
                            {
                                printf("\t\t\t%f", argument->value.float_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_DOUBLE:
                            {
                                printf("\t\t\t%lf", argument->value.double_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_STRING_LITERAL:
                            case EXPRESSION_VALUE_TYPE_SOURCE_TEXT:
                            {
                                printf("\t\t\t%.*s", fprint_string(argument->value.string));
                            }break;
                        }
                        printf("\n");
                    }
                }
                printf("\n");
            }
        }
        else
        {
            fprintf(stderr, "Failure to open this translation unit...\n");
        }
    }
    else
    {
        fprintf(stderr, "You cannot call this program like that... Exiting...\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\t./apollo <filename>\n");
    }

    return(0);
}
