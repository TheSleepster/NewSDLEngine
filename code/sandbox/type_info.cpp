/* ========================================================================
   $File: type_info.cpp $
   $Date: February 03 2026 03:30 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <stddef.h>

#include <c_base.h>
#include <c_types.h>

template <typename... Args>
internal_api void
print(string_t message, Args... arguments);

struct test_structure 
{
    int apples = 4;
    int oranges;
    int bananas;

    [[member_func]]
    char *apples_test_func(char *name = "test");
};

[[generate_function]]
void
attribute_test_function(int apples = 4)
{
}

#define ATHENA_IMPLMENTATION
#include "../code_generator/athena/athena.h"
#include "generated_test.h"

int
main(int argc, char **argv)
{
    test_structure item = {};
    const type_info_t *info = Athena::type_info(item);
#if 0
    const type_info_t *info = Athena::type_info<test_structure>();
    const type_info_t *info = Athena::type_info("test_structure");
#endif
    if(info)
    {
        const type_info_struct_t *structure = Athena::as_structure(info);
        printf("%d members\n", structure->member_count);

        for(u32 member_index = 0;
            member_index < structure->member_count;
            ++member_index)
        {
            const type_info_member_t *member = structure->members + member_index;
            printf("\tMember name: '%s'...\n", member->member_name);
            printf("\tMember size: '%d'...\n", member->type_info->size);
            printf("\tMember offset: '%d'...\n", member->offset);
        }

        printf("\n\n\n");

        //const type_info_member_t *function = Athena::get_member(info, "apples_test_func");
        const type_info_member_t *function = Athena::get_member(info, Athena::MemberLists::test_structure::apples_test_func);
        if(function)
        {
            const type_info_procedure_t *func_data = Athena::as_procedure(function);
            for(u32 arg_index = 0;
                arg_index < func_data->argument_count;
                ++arg_index)
            {
                const type_info_member_t *argument = func_data->arguments + arg_index;
                printf("Argument name: '%s'...\n", argument->member_name);
                if(argument->value.type != ATHENA_VALUE_TYPE_INVALID)
                {
                    printf("Argument Default Value:");
                    switch(argument->value.type)
                    {
                        case ATHENA_VALUE_TYPE_STRING:
                        {
                            printf(" %s\n", argument->value.string);
                        }break;
                        case ATHENA_VALUE_TYPE_INT32:
                        {
                            printf(" %d\n", argument->value.int32);
                        }break;
                    }
                }
            }
            const type_info_member_t *argument = Athena::get_argument(func_data, Athena::ArgumentLists::apples_test_func::name);
            int z = 0;

            (void)z;
            (void)argument;
        }

        const type_info_struct_t *parent = Athena::get_struct_info_from_member(function);
        int x = 0;

        (void)parent;
        (void)x;
    }

    const attribute_info_list_t *member_func_list = Athena::get_attribute_list("member_func");
    if(member_func_list)
    {
        printf("Attribute: '%s'...\n", member_func_list->attribute_name);
        for(const auto &info: *member_func_list)
        {
            printf("info: '%s'\n", info->type_name);
        }
    }

    print(STR("Test Message... Item is: '%'\n"), 4);
    return(0);
}

template <typename... Args>
static constexpr u32
get_packed_argument_count(Args... arguments)
{
    return(sizeof...(arguments));
}

// NOTE(Sleepster): Empty for no arguments 
internal_api void 
output_type_data(string_t message, string_builder_t *builder)
{
    fprintf(stdout, "%.*s", fprint_string(message));
}

template <typename T>
internal_api void 
output_type_data(string_t message, string_builder_t *builder, T &item)
{
    const type_info_t *info = Athena::type_info(item);
    switch(info->metatype)
    {
        case ATHENA_METATYPE_PRIMITIVE:
        {
#if 0
            switch(info->type_id)
            {
                case TYPE_int:
                {
                }break;
                case TYPE_long:
                {
                }break;
                case TYPE_unsigned_int:
                {
                }break;
                case TYPE_usigned_long:
                {
                }break;
                case TYPE_float:
                {
                }break;
                case TYPE_double:
                {
                }break;
                case TYPE_bool8:
                case TYPE_bool32:
                {
                }break;
            }
#endif
        }break;
        case ATHENA_METATYPE_STRUCT:
        case ATHENA_METATYPE_ENUM:
        {
        }break;
        case ATHENA_METATYPE_PROCEDURE:
        {
        }break;
    }
}

template <typename... Args>
internal_api void
print(string_t message, Args... arguments)
{
    s32 argument_count = get_packed_argument_count(arguments...);
    (void)argument_count;

    string_builder_t builder = {};
    c_string_builder_init(&builder, KB(100));
    defer(c_string_builder_deinit(&builder));

    output_type_data(message, &builder, arguments...);
}
