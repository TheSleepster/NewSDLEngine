/* ========================================================================
   $File: apollo_test.cpp $
   $Date: October 12 2026 05:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#define APOLLO_IMPLEMENTATION 1
#include "../code_generator/apollo/apollo.h"

static apollo_hash_table_t<string_t, bool8, 2048> dictionary_table;
static apollo_hash_table_t<string_t, bool8, 2048> definition_table;
static apollo_hash_table_t<string_t, bool8, 2048> type_mapping_table;
static memory_arena_t *type_arena;

static string_builder_t header_builder;
static string_builder_t forward_builder;
static string_builder_t dictionary_builder;
static string_builder_t definition_builder;
static string_builder_t template_builder;
static string_builder_t type_table_builder;
static string_builder_t member_table_builder;

static string_builder_t entity_builder;

internal_api CXType
unwrap_type(CXType type)
{
    while(type.kind == CXType_Pointer ||
          type.kind == CXType_ConstantArray ||
          type.kind == CXType_IncompleteArray)
    {
        if(type.kind == CXType_Pointer)
        {
            type = clang_getPointeeType(type);
        }
        else
        {
            type = clang_getArrayElementType(type);
        }
    }

    return(type);
}

// NOTE(Sleepster): Keep the spelling the source used, but drop pointer/array
// syntax because the member itself already records that.
internal_api string_t
get_type_identifier(code_decl_t *code_declaration)
{
    string_t typename_string = code_declaration->identifier;

    for(s32 character_index = 0;
        character_index < typename_string.count;
        ++character_index)
    {
        if(typename_string.data[character_index] == '*' ||
           typename_string.data[character_index] == '[')
        {
            typename_string = convert(clang_getTypeSpelling(unwrap_type(code_declaration->code_type)));
            break;
        }
    }

    byte *buffer = c_arena_push_size(type_arena, typename_string.count + 1);
    for(s32 character_index = 0;
        character_index < typename_string.count;
        ++character_index)
    {
        char character = (char)typename_string.data[character_index];
        if((character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9'))
        {
            buffer[character_index] = (byte)character;
        }
        else
        {
            buffer[character_index] = (byte)'_';
        }
    }
    buffer[typename_string.count] = 0;

    string_t result = { .data = buffer, .count = typename_string.count };
    return(result);
}

// NOTE(Sleepster): Pointers to records are registered by their pointee and
// arrays by their element, so try the raw spelling before the base spelling.
internal_api string_t
get_member_typename(const code_decl_member_t &member)
{
    string_t result = convert(clang_getTypeSpelling(member.code_type));
    if(!Apollo::get_declaration_info(result))
    {
        result = convert(clang_getTypeSpelling(unwrap_type(member.code_type)));
    }

    return(result);
}

// NOTE(Sleepster): Reopen the namespaces so the name doesn't depend on them already being declared.
internal_api void
create_forward_declaration(CXCursor cursor, string_t expression, const char *keyword, bool8 is_enum)
{
    s32 namespace_count = 0;
    s32 offset = 0;
    for(s32 index = 0; index + 1 < expression.count; ++index)
    {
        if(expression.data[index] == ':' && expression.data[index + 1] == ':')
        {
            c_string_builder_sprintf(&forward_builder, "namespace %.*s { ",
                                     index - offset, (char *)(expression.data + offset));
            offset = index + 2;
            ++index;
            ++namespace_count;
        }
    }

    c_string_builder_sprintf(&forward_builder, "%s %.*s", keyword,
                             (s32)(expression.count - offset),
                             (char *)(expression.data + offset));

    if(is_enum)
    {
        // NOTE(Sleepster): The definition must agree with this declaration's underlying type.
        string_t underlying = convert(clang_getTypeSpelling(clang_getCanonicalType(clang_getEnumDeclIntegerType(cursor))));
        c_string_builder_sprintf(&forward_builder, " : %.*s", fprint_string(underlying));
    }
    c_string_builder_sprintf(&forward_builder, ";");

    while(namespace_count--)
    {
        c_string_builder_sprintf(&forward_builder, " }");
    }
    c_string_builder_sprintf(&forward_builder, "\n");
}

// NOTE(Sleepster): Point a reflected type at its metadata using its canonical spelling, so a
// typedef lands on the real declaration and the keyword keeps a function like vec2() from
// hiding the record.
internal_api void
create_type_mapping(code_decl_t *declaration)
{
    CXType canonical = clang_getCanonicalType(declaration->code_type);
    CXCursor type_declaration = clang_getTypeDeclaration(canonical);
    CXCursorKind declaration_kind = clang_getCursorKind(type_declaration);
    CXCursorKind parent_kind = clang_getCursorKind(clang_getCursorSemanticParent(type_declaration));
    string_t expression = convert(clang_getTypeSpelling(canonical));

    const char *keyword = (declaration_kind == CXCursor_StructDecl) ? "struct" :
                          (declaration_kind == CXCursor_ClassDecl)  ? "class"  :
                          (declaration_kind == CXCursor_UnionDecl)  ? "union"  :
                          (declaration_kind == CXCursor_EnumDecl)   ? "enum"   : null;

    bool8 is_enum = (declaration_kind == CXCursor_EnumDecl);
    bool8 is_nested = parent_kind == CXCursor_StructDecl || parent_kind == CXCursor_ClassDecl ||
                      parent_kind == CXCursor_UnionDecl;
    bool8 is_template = !is_enum && !clang_Cursor_isNull(clang_getSpecializedCursorTemplate(type_declaration));

    // NOTE(Sleepster): Only a top-level, non-template record with a spellable name can be declared.
    if(!keyword || clang_Cursor_isAnonymous(type_declaration) || is_nested || is_template ||
       memchr(expression.data, '(', expression.count) ||
       memchr(expression.data, '<', expression.count) ||
       type_mapping_table.find(expression))
    {
        return;
    }
    type_mapping_table.add(true, expression);

    create_forward_declaration(type_declaration, expression, keyword, is_enum);
    c_string_builder_sprintf(&template_builder, "template <> constexpr const type_info_t *type_info_value<%s %.*s> = &DEFAULT_%.*s.info;\n",
                             keyword, fprint_string(expression), fprint_string(get_type_identifier(declaration)));
}

internal_api void
create_declaration_type_info(apollo_code_decl_list_t *declarations, string_t typename_string)
{
    code_decl_t *code_declaration = Apollo::get_declaration_info(typename_string);
    if(code_declaration)
    {
        string_t identifier = get_type_identifier(code_declaration);
        if(!dictionary_table.find(identifier))
        {
            dictionary_table.add(true, identifier);

            dynarray_t<code_decl_member_t> *members = null;
            const char *count_name = null;
            const char *pointer_name = null;
            const char *array_name = null;
            const char *items_name = null;

            if(code_declaration->metatype == CODE_DECL_METATYPE_STRUCTURE ||
               code_declaration->metatype == CODE_DECL_METATYPE_ENUM)
            {
                code_decl_structure_t *structure = static_cast<code_decl_structure_t*>(code_declaration);
                members = &structure->members;
                count_name = "member_count";
                pointer_name = "member_pointer";
                array_name = "member_array";
                items_name = "members";
            }
            else if(code_declaration->metatype == CODE_DECL_METATYPE_PROCEDURE)
            {
                code_decl_procedure_t *procedure = static_cast<code_decl_procedure_t*>(code_declaration);
                members = &procedure->arguments;
                count_name = "argument_count";
                pointer_name = "argument_pointer";
                array_name = "argument_array";
                items_name = "arguments";
            }

            if(members)
            {
                s32 member_count = 0;
                for(const auto &member: *members)
                {
                    if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                    {
                        ++member_count;
                    }
                }

                c_string_builder_sprintf(&dictionary_builder, R"(struct type_info_%.*s {
    const type_info_t info;
    const unsigned long %s;
    const type_info_member_t *const *%s;
)", fprint_string(identifier), count_name, pointer_name);

                if(member_count > 0)
                {
                    c_string_builder_sprintf(&dictionary_builder, R"(    union {
        const type_info_member_t *%s[%d];
        struct {
)", array_name, member_count);

                    for(const auto &member: *members)
                    {
                        if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                        {
                            c_string_builder_sprintf(&dictionary_builder, "\t\t\tconst type_info_member_t *%.*s;\n", fprint_string(member.identifier));
                        }
                    }
                    c_string_builder_sprintf(&dictionary_builder, "\t\t}%s;\n\t};\n", items_name);
                }
                c_string_builder_sprintf(&dictionary_builder, "};\n");
                c_string_builder_sprintf(&dictionary_builder, "extern const type_info_%.*s DEFAULT_%.*s;\n\n",
                                         fprint_string(identifier), fprint_string(identifier));

                if(member_count > 0 && code_declaration->metatype == CODE_DECL_METATYPE_STRUCTURE)
                {
                    c_string_builder_sprintf(&dictionary_builder, "namespace MemberLists {\nenum class %.*s {\n", fprint_string(identifier));
                    for(const auto &member: *members)
                    {
                        if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                        {
                            c_string_builder_sprintf(&dictionary_builder, "\t%.*s,\n", fprint_string(member.identifier));
                        }
                    }
                    c_string_builder_sprintf(&dictionary_builder, "};\n}\n\n");
                }

                for(const auto &member: *members)
                {
                    if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                    {
                        string_t member_typename = get_member_typename(member);
                        create_declaration_type_info(declarations, member_typename);
                    }
                }
            }
            else
            {
                c_string_builder_sprintf(&dictionary_builder, "struct type_info_%.*s { const type_info_t info; };\n",
                                         fprint_string(identifier));
                c_string_builder_sprintf(&dictionary_builder, "extern const type_info_%.*s DEFAULT_%.*s;\n\n",
                                         fprint_string(identifier), fprint_string(identifier));
            }
        }
    }
}

internal_api void
create_declaration_type_data(apollo_code_decl_list_t *declarations, string_t typename_string)
{
    code_decl_t *type_declaration = Apollo::get_declaration_info(typename_string);
    if(type_declaration)
    {
        string_t identifier = get_type_identifier(type_declaration);
        if(!definition_table.find(identifier))
        {
            definition_table.add(true, identifier);

            s32 size = Max(type_declaration->size, 0);

            dynarray_t<code_decl_member_t> *members = null;
            const char *member_count_name = null;
            const char *member_pointer_name = null;
            const char *member_array_name = null;
            const char *members_name = null;

            if(type_declaration->metatype == CODE_DECL_METATYPE_STRUCTURE ||
               type_declaration->metatype == CODE_DECL_METATYPE_ENUM)
            {
                code_decl_structure_t *structure = static_cast<code_decl_structure_t*>(type_declaration);
                members = &structure->members;
                member_count_name = "member_count";
                member_pointer_name = "member_pointer";
                member_array_name = "member_array";
                members_name = "members";

                create_type_mapping(type_declaration);
            }
            else if(type_declaration->metatype == CODE_DECL_METATYPE_PROCEDURE)
            {
                code_decl_procedure_t *procedure = static_cast<code_decl_procedure_t*>(type_declaration);
                members = &procedure->arguments;
                member_count_name = "argument_count";
                member_pointer_name = "argument_pointer";
                member_array_name = "argument_array";
                members_name = "arguments";
            }

            if(members)
            {
                s32 member_count = 0;
                for(const auto &member: *members)
                {
                    if(member.metatype == CODE_DECL_METATYPE_PROCEDURE)
                    {
                        continue;
                    }

                    ++member_count;

                    string_t member_typename = get_member_typename(member);
                    create_declaration_type_data(declarations, member_typename);

                    code_decl_t *member_type = Apollo::get_declaration_info(member_typename);
                    s32 member_size = member.size < 0 ? 0 : member.size;

                    c_string_builder_sprintf(&definition_builder, "constexpr type_info_member_t DEFAULT_member_%.*s_%.*s = {\n\t.info = { .name = \"%.*s\", .size = %d, .metatype = %d },\n",
                                             fprint_string(identifier), fprint_string(member.identifier),
                                             fprint_string(member.identifier), member_size, member.metatype);

                    if(member_type)
                    {
                        string_t member_type_identifier = get_type_identifier(member_type);
                        c_string_builder_sprintf(&definition_builder, "\t.type_info = &DEFAULT_%.*s.info,\n",
                                                 fprint_string(member_type_identifier));
                    }

                    c_string_builder_sprintf(&definition_builder, "\t.offset = %u,\n", member.offset);
                    if(member.has_value)
                    {
                        s32 value_type = member.value.type;
                        switch(value_type)
                        {
                            case EXPRESSION_VALUE_TYPE_BOOLEAN:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .int32_value = %d },\n",
                                                         value_type, member.value.boolean_value ? 1 : 0);
                            }break;
                            case EXPRESSION_VALUE_TYPE_SIGNED_LONG:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .int64_value = %lld },\n",
                                                         value_type, (long long)member.value.signed_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_UNSIGNED_LONG:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .uint64_value = %llu },\n",
                                                         value_type, (unsigned long long)member.value.unsigned_long);
                            }break;
                            case EXPRESSION_VALUE_TYPE_FLOAT:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .float32_value = %g },\n",
                                                         value_type, (double)member.value.float_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_DOUBLE:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .float64_value = %g },\n",
                                                         value_type, member.value.double_value);
                            }break;
                            case EXPRESSION_VALUE_TYPE_STRING_LITERAL:
                            case EXPRESSION_VALUE_TYPE_SOURCE_TEXT:
                            {
                                c_string_builder_sprintf(&definition_builder, "\t.value_type = %d,\n\t.value = { .string_value = \"%.*s\" },\n",
                                                         value_type, fprint_string(member.value.string));
                            }break;
                        }
                    }

                    c_string_builder_sprintf(&definition_builder, "};\n\n");
                    c_string_builder_sprintf(&member_table_builder, "\t&DEFAULT_member_%.*s_%.*s.info,\n",
                                             fprint_string(identifier), fprint_string(member.identifier));
                }

                c_string_builder_sprintf(&definition_builder, "constexpr type_info_%.*s DEFAULT_%.*s = {\n\t.info = { .name = \"%.*s\", .size = %d, .metatype = %d },\n\t.%s = %d,\n",
                                         fprint_string(identifier), fprint_string(identifier),
                                         fprint_string(identifier), size, type_declaration->metatype,
                                         member_count_name, member_count);

                if(member_count > 0)
                {
                    c_string_builder_sprintf(&definition_builder, "\t.%s = DEFAULT_%.*s.%s,\n\t.%s = {\n",
                                             member_pointer_name, fprint_string(identifier), member_array_name,
                                             members_name);
                    for(const auto &member: *members)
                    {
                        if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                        {
                            c_string_builder_sprintf(&definition_builder, "\t\t.%.*s = &DEFAULT_member_%.*s_%.*s,\n",
                                                     fprint_string(member.identifier), fprint_string(identifier),
                                                     fprint_string(member.identifier));
                        }
                    }
                    c_string_builder_sprintf(&definition_builder, "\t},\n");
                }
                else
                {
                    c_string_builder_sprintf(&definition_builder, "\t.%s = nullptr,\n", member_pointer_name);
                }

                c_string_builder_sprintf(&definition_builder, "};\n\n");
            }
            else
            {
                c_string_builder_sprintf(&definition_builder, "constexpr type_info_%.*s DEFAULT_%.*s = { .info = { .name = \"%.*s\", .size = %d, .metatype = %d } };\n\n",
                                         fprint_string(identifier), fprint_string(identifier),
                                         fprint_string(identifier), size, type_declaration->metatype);
            }

            c_string_builder_sprintf(&type_table_builder, "\t&DEFAULT_%.*s.info,\n", fprint_string(identifier));
        }
    }
}

internal_api void
generate_apollo_structure_type_info_header(apollo_code_decl_list_t *declarations)
{
    memory_arena_t *arena = type_arena;

    dictionary_table.init(arena);
    definition_table.init(arena);
    type_mapping_table.init(arena);

    c_string_builder_init(&header_builder,       MB(2));
    c_string_builder_init(&forward_builder,      MB(2));
    c_string_builder_init(&dictionary_builder,   MB(2));
    c_string_builder_init(&definition_builder,   MB(2));
    c_string_builder_init(&template_builder,     MB(2));
    c_string_builder_init(&type_table_builder,   MB(2));
    c_string_builder_init(&member_table_builder, MB(2));

    file_t out_file = c_file_open(STR("meta/APOLLO_GENERATED_RTTI.h"), true, true);
    c_string_builder_sprintf(&header_builder, R"(// THIS FILE IS GENERATED BY THE sandbox/apollo_test.cpp FILE
#if !defined(APOLLO_GENERATED_STRUCTURES)
#include <code_generator/apollo/apollo_runtime.h>

)");
    c_string_builder_sprintf(&dictionary_builder, "namespace Apollo {\n\n");
    dictionary_table.add(true, STR("void"));
    dictionary_table.add(true, STR("bool"));
    dictionary_table.add(true, STR("char"));
    dictionary_table.add(true, STR("signed char"));
    dictionary_table.add(true, STR("unsigned char"));
    dictionary_table.add(true, STR("short"));
    dictionary_table.add(true, STR("unsigned short"));
    dictionary_table.add(true, STR("int"));
    dictionary_table.add(true, STR("unsigned int"));
    dictionary_table.add(true, STR("long"));
    dictionary_table.add(true, STR("unsigned long"));
    dictionary_table.add(true, STR("long long"));
    dictionary_table.add(true, STR("unsigned long long"));
    dictionary_table.add(true, STR("float"));
    dictionary_table.add(true, STR("double"));
    dictionary_table.add(true, STR("long double"));
    dictionary_table.add(true, STR("wchar_t"));
    dictionary_table.add(true, STR("char16_t"));
    dictionary_table.add(true, STR("char32_t"));

    definition_table.add(true, STR("void"));
    definition_table.add(true, STR("bool"));
    definition_table.add(true, STR("char"));
    definition_table.add(true, STR("signed char"));
    definition_table.add(true, STR("unsigned char"));
    definition_table.add(true, STR("short"));
    definition_table.add(true, STR("unsigned short"));
    definition_table.add(true, STR("int"));
    definition_table.add(true, STR("unsigned int"));
    definition_table.add(true, STR("long"));
    definition_table.add(true, STR("unsigned long"));
    definition_table.add(true, STR("long long"));
    definition_table.add(true, STR("unsigned long long"));
    definition_table.add(true, STR("float"));
    definition_table.add(true, STR("double"));
    definition_table.add(true, STR("long double"));
    definition_table.add(true, STR("wchar_t"));
    definition_table.add(true, STR("char16_t"));
    definition_table.add(true, STR("char32_t"));

    c_string_builder_sprintf(&type_table_builder,   "constexpr const type_info_t *const APOLLO_type_table[] = {\n");
    c_string_builder_sprintf(&member_table_builder, "constexpr const type_info_t *const APOLLO_member_table[] = {\n");
    dynarray_t<apollo_procedure_call_expression_t> calls = Apollo::get_procedure_calls(STR("Apollo::type_info"));
    for(const auto &call: calls)
    {
        // NOTE(Sleepster): A call can pass the type or its name, so try both; the
        // create_declaration_type_* functions ignore anything already written.
        for(const auto &argument: call.arguments)
        {
            create_declaration_type_info(declarations, argument.identifier);
            create_declaration_type_data(declarations, argument.identifier);

            string_t argument_type_string = convert(clang_getTypeSpelling(argument.type));
            create_declaration_type_info(declarations, argument_type_string);
            create_declaration_type_data(declarations, argument_type_string);
        }
    }
    c_string_builder_sprintf(&type_table_builder, "\tnullptr,\n};\n");
    c_string_builder_sprintf(&member_table_builder, "\tnullptr,\n};\n\n} // namespace Apollo\n");
    c_string_builder_sprintf(&member_table_builder, "#endif // APOLLO_GENERATED_STRUCTURES");

    c_string_builder_dump_to_file(&out_file, &header_builder);
    c_string_builder_dump_to_file(&out_file, &forward_builder);
    c_string_builder_dump_to_file(&out_file, &dictionary_builder);
    c_string_builder_dump_to_file(&out_file, &definition_builder);
    c_string_builder_dump_to_file(&out_file, &template_builder);
    c_string_builder_dump_to_file(&out_file, &type_table_builder);
    c_string_builder_dump_to_file(&out_file, &member_table_builder);
}

internal_api void
generate_entity_structure_functions(apollo_code_decl_list_t *declarations)
{
    c_string_builder_init(&entity_builder, MB(2));

    dynarray_t<code_decl_t *> entity_decls = Apollo::get_declarations_with_attribute(declarations, STR("Game_Entity"));
    file_t entity_file = c_file_open(STR("meta/GENERATED_entity_data.cpp"), true, true);

    dynarray_t<code_decl_procedure_t*> tagged_procedures = {};
    code_decl_structure_t *archetype_declaration = null;
    for(code_decl_t *declaration: entity_decls)
    {
        if(declaration->metatype == CODE_DECL_METATYPE_PROCEDURE)
        {
            code_decl_procedure_t *proc = static_cast<code_decl_procedure_t*>(declaration);
            c_dynarray_add(&tagged_procedures, proc);
        }
        else if(c_string_compare(declaration->identifier, STR("entity_archetype_t")))
        {
            archetype_declaration = static_cast<code_decl_structure_t*>(declaration);
        }
    }

    c_string_builder_sprintf(&entity_builder, R"(
struct entity_create_info_t {
)");

    dynarray_t<u64> unique_arguments = {};
    for(code_decl_procedure_t *procedure: tagged_procedures)
    {
        for(auto &argument: procedure->arguments)
        {
            u64 parameter_hash = fnv1a_hash(argument.identifier.data, argument.identifier.count);
            if(c_dynarray_add_if_unique(&unique_arguments, &parameter_hash))
            {
                string_t argument_type = convert(clang_getTypeSpelling(unwrap_type(argument.code_type)));
                string_t argument_identifier = argument.identifier;
                c_string_builder_sprintf(&entity_builder, "\t%.*s",
                                         fprint_string(argument_type));
                if(argument.flags & CODE_DECL_FLAG_POINTER)
                {
                    c_string_builder_sprintf(&entity_builder, "*");
                }

                c_string_builder_sprintf(&entity_builder, " %.*s;\n",
                                         fprint_string(argument_identifier));
            }
        }
    }

    c_string_builder_sprintf(&entity_builder, "};\n");
    c_string_builder_sprintf(&entity_builder, "entity_t *s_entity_create_of_type(u32 entity_archetype, entity_create_info_t *create_info);\n");
    c_string_builder_sprintf(&entity_builder, "\nentity_t *s_entity_create_of_type(u32 entity_archetype, entity_create_info_t *create_info) {\n\tentity_t *result = nullptr;\n\tswitch(entity_archetype) {\n");
    for(s32 member_index = 1;
        member_index < (archetype_declaration->members.used - 1);
        ++member_index)
    {
        code_decl_member_t *member = &archetype_declaration->members[member_index];
        c_string_builder_sprintf(&entity_builder, "\t\tcase %.*s: {\n", fprint_string(member->identifier));

        code_decl_procedure_t *procedure = tagged_procedures[member_index - 1];

        c_string_builder_sprintf(&entity_builder, "\t\t\tresult = %.*s(", fprint_string(procedure->identifier));
        for(s32 argument_index = 0;
            argument_index < procedure->arguments.used;
            ++argument_index)
        {
            code_decl_member_t *argument = &procedure->arguments[argument_index];

            c_string_builder_sprintf(&entity_builder, "create_info->%.*s", fprint_string(argument->identifier));
            if(argument_index < procedure->arguments.used - 1)
            {
                c_string_builder_sprintf(&entity_builder, ", ");
            }
        }
        c_string_builder_sprintf(&entity_builder, ");\n\t\t}break;\n");
    }
    c_string_builder_sprintf(&entity_builder, "\t}\n\treturn(result);\n}\n");

    c_string_builder_dump_to_file(&entity_file, &entity_builder);
}

int
main(void)
{
    c_global_context_init();
    dynarray_t<string_t> options = {};

    memory_arena_t arena = c_arena_create(MB(20), ALLOCATOR_TAG_STATIC);
    type_arena = &arena;

    string_t include_path    = STR("-I../code/");
    string_t engine_define   = STR("-DENGINE_BUILD");
    string_t game_define     = STR("-DGAME_DLL_BUILD");
    string_t renderer_define = STR("-DRENDERER_HEADLESS");

    c_dynarray_add(&options, &include_path);
    c_dynarray_add(&options, &engine_define);
    c_dynarray_add(&options, &game_define);
    c_dynarray_add(&options, &renderer_define);
    apollo_code_decl_list_t *declarations = Apollo::parse_singular_file(STR("build.cpp"), options);
    if(declarations)
    {
        generate_apollo_structure_type_info_header(declarations);
        generate_entity_structure_functions(declarations);
    }

    return(0);
}
