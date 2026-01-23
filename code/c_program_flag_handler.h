#if !defined(C_PROGRAM_FLAG_HANDLER_H)
/* ========================================================================
   $File: c_program_flag_handler.h $
   $Date: January 22 2026 04:28 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>
#include <c_file_api.h>
#include <c_string.h>

#define C_PROGRAM_FLAG_HANDLER_H
#define MAX_PROGRAM_FLAGS (128)

typedef enum arg_type
{
    FLAG_TYPE_BOOL,
    FLAG_TYPE_U64,
    FLAG_TYPE_FLOAT32,
    FLAG_TYPE_STRING
}arg_type_t;

typedef union program_flag_data
{
    bool32  bool32;
    u64     u64;
    float32 float32;
    char   *string;
}program_flag_data_t;

typedef struct program_flag
{
    bool8               is_valid;
    char               *name;
    char               *description;

    arg_type_t          arg_type;

    program_flag_data_t arg_value;
    program_flag_data_t default_arg_value;
}program_flag_t;

typedef struct program_flag_state
{
    program_flag_t program_flags[MAX_PROGRAM_FLAGS];
    u32                flag_counter;
}program_flag_state_t;

program_flag_t* c_program_flag_add(char *flag_name, char *flag_description);
bool32*         c_program_flag_add_bool32(char *flag_name, bool32 default_value, char *flag_description);
u64*            c_program_flag_add_size(char *flag_name, u64 default_value, char *flag_description);
float32*        c_program_flag_add_float32(char *flag_name, float32 default_value, char *flag_description);
char**          c_program_flag_add_string(char *flag_name, char *default_value, char *flag_description);
void            c_program_flag_print_flag_list(void);
void            c_program_flag_container_init(program_flag_state_t *container);
bool32          c_program_flag_parse_args(s32 argc, char **argv);

// TODO(Sleepster): 
// - [ ] MORE TYPES (double, s32, etc.)
// - [ ] Expand the API to allow for multiple program_flag_states through passing a simple pointer to one of them.
// - [ ] "c_program_flag_get_flag_by_name" and it'll return a flag value

#ifdef PROGRAM_FLAG_HANDLER_IMPLEMENTATION

static program_flag_state_t program_flag_state;

program_flag_t*
c_program_flag_add(char *flag_name, char *flag_description)
{
    program_flag_t *result = program_flag_state.program_flags + program_flag_state.flag_counter++;

    result->name            = flag_name;
    result->description     = flag_description;
    result->is_valid        = true;

    return(result);
}

bool32*
c_program_flag_add_bool32(char *flag_name, bool32 default_value, char *flag_description)
{
    bool32 *result = null;

    program_flag_t *flag = c_program_flag_add(flag_name, flag_description);

    flag->arg_value.bool32         = default_value;
    flag->default_arg_value.bool32 = default_value;
    flag->arg_type                 = FLAG_TYPE_BOOL;
    flag->is_valid                 = true;

    result = &flag->arg_value.bool32;
    return(result);
}

u64*
c_program_flag_add_size(char *flag_name, u64 default_value, char *flag_description)
{
    u64 *result = null;

    program_flag_t *flag = c_program_flag_add(flag_name, flag_description);
    flag->arg_value.u64         = default_value;
    flag->default_arg_value.u64 = default_value;
    flag->arg_type              = FLAG_TYPE_U64,
    flag->is_valid              = true;

    result = &flag->arg_value.u64;
    return(result);
}

float32*
c_program_flag_add_float32(char *flag_name, float32 default_value, char *flag_description)
{
    float32 *result = null;

    program_flag_t *flag = c_program_flag_add(flag_name, flag_description);
    flag->arg_value.float32         = default_value;
    flag->arg_type                  = FLAG_TYPE_FLOAT32,
    flag->default_arg_value.float32 = default_value;
    flag->is_valid                  = true;

    result = &flag->arg_value.float32;
    return(result);
}

char**
c_program_flag_add_string(char *flag_name, char *default_value, char *flag_description)
{
    char **result = null;

    program_flag_t *flag = c_program_flag_add(flag_name, flag_description);
    flag->arg_value.string         =  default_value;
    flag->arg_type                 =  FLAG_TYPE_STRING,
    flag->default_arg_value.string =  default_value;
    flag->is_valid                 =  true;

    result = &flag->arg_value.string;
    return(result);
}

void
c_program_flag_print_flag_list(void)
{
    log_info("Valid arguments for this program are:\n");
    for(u32 flag_index = 0;
        flag_index < program_flag_state.flag_counter;
        ++flag_index)
    {
        program_flag_t *flag = program_flag_state.program_flags + flag_index; 
        switch(flag->arg_type)
        {
            case FLAG_TYPE_BOOL:
            {
                log_info("-%s <bool>\n", flag->name);
                log_info("\tDefault Value is: '%s'\n", flag->default_arg_value.bool32 == false ? "false" : "true");
                log_info("\t%s\n", flag->description);
            }break;
            case FLAG_TYPE_U64:
            { 
                log_info("-%s <u64>\n", flag->name);
                log_info("\tDefault Value is: '%llu'\n", flag->default_arg_value.u64);
                log_info("\t%s\n", flag->description);
            }break;
            case FLAG_TYPE_FLOAT32:
            {
                log_info("-%s <float32>\n", flag->name);
                log_info("\tDefault Value is: '%f'\n", flag->default_arg_value.float32);
                log_info("\t%s\n", flag->description);
            }break;
            case FLAG_TYPE_STRING:
            {
                log_info("-%s <string>\n", flag->name);
                log_info("\tDefault Value is: '%s'\n", flag->default_arg_value.string);
                log_info("\t%s\n", flag->description);
            }break;
        }
    }
}

void
c_program_flag_container_init(program_flag_state_t *container)
{
    ZeroStruct(*container);
}

bool32
c_program_flag_parse_args(s32 argc, char **argv)
{
    bool32 success = true;
    if(argc < 2) 
    {
        log_error("No arguments...\n");
        c_program_flag_print_flag_list();
        exit(-1);
    }

    for(s32 arg_index = 1;
        arg_index < argc;
        ++arg_index)
    {
        char *arg_string = argv[arg_index];
        Assert(arg_string);

        string_t passed_arg = {
            .data  = (byte*)arg_string,
            .count = c_string_length(arg_string)
        };

        u32 dash_index = c_string_find_first_char_from_left(passed_arg, '-');
        c_string_advance_by(&passed_arg, dash_index + 1);

        bool8 arg_found = false;
        for(u32 flag_index = 0;
            flag_index < program_flag_state.flag_counter;
            ++flag_index)
        {
            program_flag_t *flag = program_flag_state.program_flags + flag_index;
            string_t flag_name = {
                .data  = (byte*)flag->name,
                .count = c_string_length(flag->name)
            };

            string_t arg_string_shallow_copy = passed_arg;
            u32 value_index = c_string_find_first_char_from_left(arg_string_shallow_copy, '=');
            if(value_index != U32_MAX)
            {
                arg_string_shallow_copy.count = value_index;
            }

            if(c_string_compare(arg_string_shallow_copy, flag_name))
            {
                c_string_advance_by(&passed_arg, value_index + 1);
                arg_found = true;

                byte *value = passed_arg.data;
                switch(flag->arg_type)
                {
                    case FLAG_TYPE_BOOL:
                    {
                        bool32 bool_value = true;
                        if(c_string_compare(passed_arg, STR("false")))
                        {
                            bool_value = false;
                        }

                        flag->arg_value.bool32 = bool_value;
                        
                        goto next;
                    }break;
                    case FLAG_TYPE_U64:
                    {
                        char *end_ptr;
                        flag->arg_value.u64 = strtoull((char*)value, &end_ptr, 10);

                        goto next;
                    }break;
                    case FLAG_TYPE_FLOAT32:
                    {
                        char *end_ptr;
                        flag->arg_value.float32 = strtof((char*)value, &end_ptr);

                        goto next;
                    }break;
                    case FLAG_TYPE_STRING:
                    {
                        flag->arg_value.string = (char*)passed_arg.data;

                        goto next;
                    }break;
                }
            }
        }
next:
        if(!arg_found) 
        {
            log_error("Argument '%s' not found...\n", arg_string);
            c_program_flag_print_flag_list();
        }
    }

    return(success);
}
#endif

#endif // C_PROGRAM_FLAG_HANDLER_H

