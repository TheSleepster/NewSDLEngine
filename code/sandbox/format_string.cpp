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
c_string_format_value(string_t *final_string, T &argument)
{
    s32 result = 0;

    const type_info_t *type_info = Apollo::type_info<T>();
    switch(type_info->metatype)
    {
        default: { InvalidCodePath; }break;
        case APOLLO_METATYPE_PRIMITIVE:
        {
        }break;
        case APOLLO_METATYPE_STRUCTURE:
        {
        }break;
        case APOLLO_METATYPE_ENUM:
        {
        }break;
    }

    return(result);
}

template<typename... Arguments>
string_t
c_string_format(string_t format, Arguments &&...arguments)
{
    string_t result = format;
    
    s32 format_input = c_string_find_first_char_from_left(format, '%');
    if(format_input != -1)
    {
        s32 bytes_to_write = c_string_format_value(&result, c_forward<Arguments>(arguments)...);
        result.count += bytes_to_write;
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
#if 0 
    c_string_format("%, %", "test_string", 4);
#endif

    return(0);
}
