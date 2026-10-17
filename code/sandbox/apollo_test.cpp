/* ========================================================================
   $File: apollo_test.cpp $
   $Date: October 12 2026 05:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#define APOLLO_IMPLEMENTATION 1
#include "../code_generator/apollo/apollo.h"
//#include <meta/test_metaprogram_file.h>

static apollo_hash_table_t<string_t, bool8, 2048> type_table;
static string_builder_t builder;

internal_api void
output_type_info(apollo_code_decl_list_t *declarations, string_t typename_string)
{
    code_decl_t *code_declaration = Apollo::get_declaration_info(typename_string);
    if(code_declaration)
    {
        if(type_table.find(code_declaration->identifier))
        {
            return;
        }

        type_table.add(true, code_declaration->identifier);

        // NOTE(Sleepster): A type we only ever saw used (u32, char, ...) has no declaration of
        // its own, so there's nothing to emit for it.
        if(code_declaration->metatype == CODE_DECL_METATYPE_STRUCTURE ||
           code_declaration->metatype == CODE_DECL_METATYPE_ENUM)
        {
            code_decl_structure_t *structure = static_cast<code_decl_structure_t*>(code_declaration);
            for(s32 member_index = 0;
                member_index < structure->members.used;
                ++member_index)
            {
                const code_decl_member_t &member = structure->members[member_index];

                string_t member_typename = convert(clang_getTypeSpelling(member.code_type));
                output_type_info(declarations, member_typename);
            }

            c_string_builder_sprintf(&builder, "struct type_info_%.*s: public type_info_struct_t {\n", fprint_string(code_declaration->identifier));
            for(s32 member_index = 0;
                member_index < structure->members.used;
                ++member_index)
            {
                const code_decl_member_t &member = structure->members[member_index];
                c_string_builder_sprintf(&builder, "\ttype_info_t *%.*s;\n", fprint_string(member.identifier));
            }
            c_string_builder_sprintf(&builder, "};\n\n");
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

    c_dynarray_add(&options, &include_path);
    c_dynarray_add(&options, &engine_define);
    c_dynarray_add(&options, &game_define);
    c_dynarray_add(&options, &renderer_define);
    apollo_code_decl_list_t *declarations = Apollo::parse_singular_file(STR("build.cpp"), options);
    if(declarations)
    {
        memory_arena_t arena = c_arena_create(MB(20), ALLOCATOR_TAG_STATIC);
        type_table.init(&arena);

        c_string_builder_init(&builder, MB(2));

        file_t out_file = c_file_open(STR("meta/test_metaprogram_file.h"), true, true);
        c_string_builder_sprintf(&builder, R"(
struct type_info_t {
    const char *name;
    const u32   size;
};

struct type_info_struct_t: public type_info_t {
    const u32          member_count;
    const type_info_t *member_array;
};

)");
        dynarray_t<apollo_procedure_call_expression_t> calls = Apollo::get_procedure_calls(STR("Athena::type_info"));
        for(const auto &call: calls)
        {
            // NOTE(Sleepster): A type_info call can hand us the type itself
            // (Athena::type_info<Foo>()) or the name of one (Athena::type_info(STR("Foo"))),
            // so we try the argument's name first and then its type. Whichever one resolves is
            // enough, output_type_info() ignores anything we've already written out.
            for(const auto &argument: call.arguments)
            {
                output_type_info(declarations, argument.identifier);
                output_type_info(declarations, convert(clang_getTypeSpelling(argument.type)));
            }
        }

        c_string_builder_dump_to_file(&out_file, &builder);
    }

    return(0);
}
