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
static memory_arena_t *type_arena;

static string_builder_t header_builder;
static string_builder_t dictionary_builder;
static string_builder_t definition_builder;
static string_builder_t type_table_builder;
static string_builder_t member_table_builder;

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
                    c_string_builder_sprintf(&dictionary_builder, "namespace MemberLists { enum class %.*s {\n", fprint_string(identifier));
                    for(const auto &member: *members)
                    {
                        if(member.metatype != CODE_DECL_METATYPE_PROCEDURE)
                        {
                            c_string_builder_sprintf(&dictionary_builder, "\t%.*s,\n", fprint_string(member.identifier));
                        }
                    }
                    c_string_builder_sprintf(&dictionary_builder, "}; }\n\n");
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

            s32 size = type_declaration->size;
            if(size < 0)
            {
                size = 0;
            }

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

int
main(void)
{
    dynarray_t<string_t> options = {};

    string_t include_path    = STR("-I../code/");
    string_t engine_define   = STR("-DENGINE_BUILD");
    string_t game_define     = STR("-DGAME_BUILD");
    string_t renderer_define = STR("-DRENDERER_HEADLESS");
    string_t generate_define = STR("-DAPOLLO_GENERATE");

    c_dynarray_add(&options, &include_path);
    c_dynarray_add(&options, &engine_define);
    c_dynarray_add(&options, &game_define);
    c_dynarray_add(&options, &renderer_define);
    c_dynarray_add(&options, &generate_define);
    apollo_code_decl_list_t *declarations = Apollo::parse_singular_file(STR("build.cpp"), options);
    if(declarations)
    {
        memory_arena_t arena = c_arena_create(MB(20), ALLOCATOR_TAG_STATIC);
        type_arena = &arena;

        dictionary_table.init(&arena);
        definition_table.init(&arena);

        c_string_builder_init(&header_builder,       MB(2));
        c_string_builder_init(&dictionary_builder,   MB(2));
        c_string_builder_init(&definition_builder,   MB(2));
        c_string_builder_init(&type_table_builder,   MB(2));
        c_string_builder_init(&member_table_builder, MB(2));

        file_t out_file = c_file_open(STR("meta/APOLLO_GENERATED_RTTI.h"), true, true);
        c_string_builder_sprintf(&header_builder, R"(#include <code_generator/apollo/apollo_runtime.h>

namespace Apollo {

)");
        c_string_builder_sprintf(&type_table_builder,   "constexpr const type_info_t *const APOLLO_type_table[] = {\n");
        c_string_builder_sprintf(&member_table_builder, "constexpr const type_info_t *const APOLLO_member_table[] = {\n");
        dynarray_t<apollo_procedure_call_expression_t> calls = Apollo::get_procedure_calls(STR("Apollo::type_info"));
        for(const auto &call: calls)
        {
            // NOTE(Sleepster): A type_info call can hand us the type itself
            // (Apollo::type_info<Foo>()) or the name of one (Apollo::type_info(STR("Foo"))),
            // so we try the argument's name first and then its type. Whichever one resolves is
            // enough, the create_declaration_type_*() functions ignore anything already written out.
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

        c_string_builder_dump_to_file(&out_file, &header_builder);
        c_string_builder_dump_to_file(&out_file, &dictionary_builder);
        c_string_builder_dump_to_file(&out_file, &definition_builder);
        c_string_builder_dump_to_file(&out_file, &type_table_builder);
        c_string_builder_dump_to_file(&out_file, &member_table_builder);
    }

    return(0);
}
