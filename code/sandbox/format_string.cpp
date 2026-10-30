/* ========================================================================
   $File: format_string.cpp $
   $Date: October 29 2026 01:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>
#include <c_string.h>
#include <c_memory_arena.h>

template <typename T>
struct remove_reference      {using type = T;};

template <typename T>
struct remove_reference<T&>  {using type = T;};

template <typename T>
struct remove_reference<T&&> {using type = T;};

template <typename T>
internal_api constexpr T&&
c_forward(typename remove_reference<T>::type &value)
{
    return((T&&)value);
}

template <typename T> T&&
c_forward(typename remove_reference<T>::type &&value)
{
    return((T&&)value);
}

template <typename T>
internal_api s32 
c_string_format_value(string_t *final_string, T &argument)
{
    (void)final_string;
    (void)argument;
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
