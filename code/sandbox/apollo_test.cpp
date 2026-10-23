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

static apollo_hash_table_t<string_t, bool8, 2048> dictionary_table;
static apollo_hash_table_t<string_t, bool8, 2048> definition_table;

static string_builder_t header_builder;
static string_builder_t dictionary_builder;
static string_builder_t definition_builder;
static string_builder_t type_table_builder;
static string_builder_t member_table_builder;

internal_api void
create_declaration_type_info(apollo_code_decl_list_t *declarations, string_t typename_string)
{
    code_decl_t *code_declaration = Apollo::get_declaration_info(typename_string);
    if(code_declaration)
    {
        if(!dictionary_table.find(code_declaration->identifier))
        {
            dictionary_table.add(true, code_declaration->identifier);

            // NOTE(Sleepster): A type we only ever saw used (u32, char, ...) has no declaration of
            // its own, so there's nothing to emit for it.
            if(code_declaration->metatype == CODE_DECL_METATYPE_STRUCTURE ||
               code_declaration->metatype == CODE_DECL_METATYPE_ENUM)
            {
                // NOTE(Sleepster): Make sure the types contained in this structure are already declared
                code_decl_structure_t *structure = static_cast<code_decl_structure_t*>(code_declaration);
                for(s32 member_index = 0;
                    member_index < structure->members.used;
                    ++member_index)
                {
                    const code_decl_member_t &member = structure->members[member_index];

                    string_t member_typename = convert(clang_getTypeSpelling(member.code_type));
                    create_declaration_type_info(declarations, member_typename);
                }

                // NOTE(Sleepster): output the type info structures.
                c_string_builder_sprintf(&dictionary_builder, "struct type_info_%.*s: public type_info_struct_t {\n\tunion {\n\t\tconst type_info_member_t *member_array[%d];\n\t\tstruct {\n", 
                                         fprint_string(code_declaration->identifier), structure->members.used);
                for(s32 member_index = 0;
                    member_index < structure->members.used;
                    ++member_index)
                {
                    const code_decl_member_t &member = structure->members[member_index];
                    c_string_builder_sprintf(&dictionary_builder, "\t\t\tconst type_info_member_t *%.*s;\n", fprint_string(member.identifier));
                }
                c_string_builder_sprintf(&dictionary_builder, "\t\t}members;\n");
                c_string_builder_sprintf(&dictionary_builder, "\t};\n");
                c_string_builder_sprintf(&dictionary_builder, "};\n\n");
            }
            else if(code_declaration->metatype == CODE_DECL_METATYPE_PROCEDURE)
            {
                code_decl_procedure_t *proc = static_cast<code_decl_procedure_t*>(code_declaration);
                for(s32 argument_index = 0;
                    argument_index < proc->arguments.used;
                    ++argument_index)
                {
                    const code_decl_member_t &argument = proc->arguments[argument_index];
                    string_t argument_typename = convert(clang_getTypeSpelling(argument.code_type));

                    create_declaration_type_info(declarations, argument_typename);
                }

                c_string_builder_sprintf(&dictionary_builder, "struct type_info_%.*s: public type_info_procedure_t {\n\tunion {\n\t\tconst type_info_member_t argument_array[%d];\nstruct {\n", 
                                         fprint_string(proc->identifier), proc->arguments.used);
                for(s32 argument_index = 0;
                    argument_index < proc->arguments.used;
                    ++argument_index)
                {
                    const code_decl_member_t &argument = proc->arguments[argument_index];
                    c_string_builder_sprintf(&dictionary_builder, "\t\tconst type_info_member_t *%.*s;\n", fprint_string(argument.identifier));
                }
                c_string_builder_sprintf(&dictionary_builder, "\t}members;\n\n");
                c_string_builder_sprintf(&dictionary_builder, "};\n\n");
            }
            else
            {
                // NOTE(Sleepster): Just get the real type... 
                CXType element_type = code_declaration->code_type;
                while(element_type.kind == CXType_Pointer)
                {
                    element_type = clang_getPointeeType(element_type);
                }

                string_t typename_string = convert(clang_getTypeSpelling(element_type));
                c_string_builder_sprintf(&dictionary_builder, "struct type_info_%.*s: public type_info_t { long padding; };\n", 
                                         fprint_string(typename_string));
            }
        }
    }
}

internal_api void
create_declaration_type_data(apollo_code_decl_list_t *declarations, string_t typename_string)
{
    (void)declarations;

    code_decl_t *declaration = Apollo::get_declaration_info(typename_string);
    if(declaration)
    {
        if(!definition_table.find(declaration->identifier))
        {
            definition_table.add(true, declaration->identifier);
            if(declaration->metatype == CODE_DECL_METATYPE_STRUCTURE || declaration->metatype == CODE_DECL_METATYPE_ENUM)
            {
                code_decl_structure_t *structure = static_cast<code_decl_structure_t*>(declaration);
                for(const auto &member: structure->members)
                {
                    string_t member_typename = convert(clang_getTypeSpelling(member.code_type));
                    create_declaration_type_data(declarations, member_typename);
                }

                c_string_builder_sprintf(&definition_builder, "constexpr type_info_%.*s DEFAULT_%.*s = {};\n", 
                                         fprint_string(typename_string), fprint_string(typename_string));
            }
            else
            {

                // NOTE(Sleepster): Just get the real type... 
                CXType element_type = declaration->code_type;
                while(element_type.kind == CXType_Pointer)
                {
                    element_type = clang_getPointeeType(element_type);
                }

                typename_string = convert(clang_getTypeSpelling(element_type));
                c_string_builder_sprintf(&definition_builder, "constexpr type_info_%.*s DEFAULT_%.*s = {};\n", 
                                         fprint_string(typename_string), fprint_string(typename_string));
            }
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
        dictionary_table.init(&arena);
        definition_table.init(&arena);

        c_string_builder_init(&header_builder,       MB(2));
        c_string_builder_init(&dictionary_builder,   MB(2));
        c_string_builder_init(&definition_builder,   MB(2));
        c_string_builder_init(&type_table_builder,   MB(2));
        c_string_builder_init(&member_table_builder, MB(2));

        file_t out_file = c_file_open(STR("meta/test_metaprogram_file.h"), true, true);
        c_string_builder_sprintf(&header_builder, R"(struct type_info_t {
    const char *name;
    const unsigned long size = 0;
    const unsigned long metatype = 0;
};

struct type_info_member_t: public type_info_t {
    const type_info_t *type_info;
    const unsigned long offset = 0;
    const unsigned long value_type = 0;
    union {
        int                int32_value;
        long long          int64_value;
        unsigned long      uint32_value;
        unsigned long long uint64_value;
        float              float32_value;
        double             float64_value;
        const char        *string_value;
    }value;
};

struct type_info_struct_t: public type_info_t {
    const unsigned long member_count = 0;
    union {
        const type_info_member_t *member_array;
        struct {}members;
    };
};

struct type_info_procedure_t: public type_info_t {
    const unsigned long argument_count = 0;
    union {
        const type_info_member_t *argument_array;
        struct {}arguments;
    };
};

//extern constexpr type_info_t *APOLLO_type_table[];
//extern constexpr type_info_t *APOLLO_member_table[];

)");
        //c_string_builder_sprintf(&type_table_builder,   "constexpr type_info_t *APOLLO_type_table[] = {\n");
        //c_string_builder_sprintf(&member_table_builder, "constexpr type_info_t *APOLLO_member_table[] = {\n");
        dynarray_t<apollo_procedure_call_expression_t> calls = Apollo::get_procedure_calls(STR("Athena::type_info"));
        for(const auto &call: calls)
        {
            // NOTE(Sleepster): A type_info call can hand us the type itself
            // (Athena::type_info<Foo>()) or the name of one (Athena::type_info(STR("Foo"))),
            // so we try the argument's name first and then its type. Whichever one resolves is
            // enough, create_declaration_type_info() ignores anything we've already written out.
            for(const auto &argument: call.arguments)
            {
                create_declaration_type_info(declarations, argument.identifier);

                string_t argument_type_string = convert(clang_getTypeSpelling(argument.type));
                create_declaration_type_info(declarations, argument_type_string);
                create_declaration_type_data(declarations, argument_type_string);
            }
        }
        c_string_builder_sprintf(&definition_builder, "\n");
        //c_string_builder_sprintf(&type_table_builder, "};\n");
        //c_string_builder_sprintf(&member_table_builder, "};\n");

        c_string_builder_dump_to_file(&out_file, &header_builder);
        c_string_builder_dump_to_file(&out_file, &dictionary_builder);
        c_string_builder_dump_to_file(&out_file, &definition_builder);
        c_string_builder_dump_to_file(&out_file, &type_table_builder);
        c_string_builder_dump_to_file(&out_file, &member_table_builder);
    }

    return(0);
}
