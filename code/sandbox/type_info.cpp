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
print(const char *message, Args... arguments);

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

    print("Test Message %");

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
output_type_data(char *buffer, u32 buffer_size)
{
}

template <typename T>
internal_api void 
output_type_data(char *buffer, u32 buffer_size, T &item)
{
    const type_info_t *info = Athena::type_info(item);
    if(info->metatype != ATHENA_METATYPE_PRIMITIVE)
    {
    }
    else
    {
    }
}

template <typename... Args>
internal_api void
print(const char *message, Args... arguments)
{
    u32 argument_count = get_packed_argument_count(arguments...);
    char buffer[8192];
    if(argument_count > 0)
    {
        output_type_data(buffer, sizeof(buffer), arguments...);
    }
}
