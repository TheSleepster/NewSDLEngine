/* ========================================================================
   $File: format_string.cpp $
   $Date: October 29 2026 01:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <c_string.h>
#include <c_memory_arena.h>

#include "code_generator/apollo/apollo_runtime.h"
#include <meta/APOLLO_GENERATED_RTTI.h>

template <typename T>
internal_api s32
c_string_format_primitive(string_t *string, const type_info_t *info, T &value)
{
    (void)string;

    // NOTE(Sleepster): Not entirely correct because a number like "10" passed as an integer
    // will only be 2 bytes, not four. But this is just testing code.
    char buffer[9] = {};

    s32 result = 0;
    s32 element_size = info->size;

    // NOTE(Sleepster): Only print if there is a string... 
    switch(info->metatype)
    {
        case APOLLO_METATYPE_INTEGER:
        {
            // NOTE(Sleepster): Char 
            if(element_size == 1)
            {
                result += snprintf(buffer, sizeof(buffer), "%c", (char)value);
            }
            // NOTE(Sleepster): Short 
            else if(element_size == 2)
            {
                result += snprintf(buffer, sizeof(buffer), "%hd", (s16)value);
            }
            // NOTE(Sleepster): Int 
            else if(element_size == 4)
            {
                result += snprintf(buffer, sizeof(buffer), "%d", (s32)value);
            }
            // NOTE(Sleepster): Long 
            else if(element_size == 8)
            {
                result += snprintf(buffer, sizeof(buffer), "%ld", (s64)value);
            }
            else 
            {
                InvalidCodePath;
            }
        }break;
        case APOLLO_METATYPE_FLOAT:
        {
            if(element_size == 4)
            {
                result += snprintf(buffer, sizeof(buffer), "%f", (float32)value);
            }
            else if(element_size == 8)
            {
                result += snprintf(buffer, sizeof(buffer), "%lf", (float64)value);
            }
            else 
            {
                InvalidCodePath;
            }
        }break;
        case APOLLO_METATYPE_BOOL:
        {
            byte current_value = static_cast<byte>(value);
            result += snprintf(buffer, sizeof(buffer), "%s", current_value > 0 ? "True" : "False");
        }break;
        case APOLLO_METATYPE_STRING:
        {
            const char *string_value = (char *)&value;
            result += snprintf(buffer, sizeof(buffer), "%s", string_value);
        }break;
    }

    return(result);
}

template <typename T>
void
c_string_format_structure(void)
{
}

template <typename T>
internal_api s32 
c_string_format_value(string_t *final_string, T &argument)
{
    s32 result = 0;

    const type_info_t *type_info = Apollo::type_info<T>();
    switch(type_info->metatype)
    {
        default: { InvalidCodePath; }break;
        case APOLLO_METATYPE_STRUCTURE:
        {
            InvalidCodePath;
        }break;
        case APOLLO_METATYPE_ENUM:
        {
            InvalidCodePath;
        }break;
        case APOLLO_METATYPE_POINTER:
        {
            const type_info_t *referenced_type = type_info->pointer_to;
            if(referenced_type->metatype != APOLLO_METATYPE_STRUCTURE &&
               referenced_type->metatype != APOLLO_METATYPE_ENUM)
            {
                result += c_string_format_primitive(final_string, referenced_type, argument);
            }
        }break;
        case APOLLO_METATYPE_ARRAY:
        {
            const type_info_t *underlying_type = type_info->array->array_type;

            T *argument_array = static_cast<T*>(&argument);
            if(underlying_type->metatype != APOLLO_METATYPE_STRUCTURE &&
               underlying_type->metatype != APOLLO_METATYPE_ENUM)
            {
                for(u32 array_dimension = 0;
                    array_dimension < type_info->array->dimension_count;
                    ++array_dimension)
                {
                    u32 array_size = type_info->array->dimensions[array_dimension];
                    for(u32 array_index = 0;
                        array_index < array_size;
                        ++array_index)
                    {
                        T &element = argument_array[array_index];
                        result += c_string_format_primitive(final_string, underlying_type, element);
                    }
                }
            }
            else
            {
                // not printing right now...
            }
        }break;
    }

    return(result);
}

internal_api s32
c_string_format_values(string_t *final_string)
{
    (void)final_string;
    return(0);
}

template <typename T, typename... Rest>
internal_api s32
c_string_format_values(string_t *final_string, T &argument, Rest &...rest)
{
    return(c_string_format_value(final_string, argument) +
           c_string_format_values(final_string, rest...));
}

template<typename... Arguments>
string_t
c_string_format(string_t format, Arguments &&...arguments)
{
    string_t result = format;
    
    s32 format_input = c_string_find_first_char_from_left(format, '%');
    if(format_input != -1)
    {
        // NOTE(Sleepster): Count the bytes needed for the final string 
        s32 bytes_to_write = c_string_format_values(null, arguments...);
        result.count += bytes_to_write;

        // NOTE(Sleepster): Allocate the memory needed for the final string
        // NOTE(Sleepster): write the data out. 
    }
    else
    {
        Expect(false, "Attempted to call c_string_format with arguments to format, but no actual formatting requirements in the string passed...\n");
    }

    return(result);
}

int
main(void)
{
    printf("Hello, World!\n");
    c_string_format(STR("%, %, %"), 4, 1.0f, 1.0f);
#if 0 
#endif

    return(0);
}
