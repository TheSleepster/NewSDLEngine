/* ========================================================================
   $File: c_string.cpp $
   $Date: December 08 2025 08:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <c_string.h>
#include <c_math.h>
#include <c_global_context.h>

#include <c_file_api.h>
#include <c_file_watcher.h>

ENGINE_API char *
c_string_null_terminated(memory_arena_t *arena, string_t data)
{
    char *result = null;
    result = c_arena_push_array(arena, char, data.count + 1);

    memcpy((void*)result, (void*)data.data, data.count);
    result[data.count + 1] = '\0';

    return(result);
}

ENGINE_API u32
c_string_length(const char *c_string)
{
    Assert(c_string);

    s32 result = 0;
    while(*c_string != 0) 
    {
        ++result;
        ++c_string;
    }

    return(result);
}

ENGINE_API bool8
c_string_is_valid(string_t string)
{
    return((string.count != 0) && (string.data != null));
}

ENGINE_API string_t
c_string_create(const char *c_string)
{
    string_t result;
    result.count = c_string_length(c_string);
    result.data  = (byte*)c_string;

    return(result);
}

ENGINE_API string_t
c_string_create_with_length(byte *data, u32 length)
{
    string_t result;
    result.data  = data;
    result.count = length;

    return(result);
}

ENGINE_API string_t
c_string_make_heap(memory_arena_t *arena, string_t string)
{
    Assert(string.count > 0);

    string_t result;
    result.count = string.count;
    result.data  = c_arena_push_array(arena, byte, string.count);

    MemoryCopy(result.data, string.data, string.count * sizeof(byte));
    return(result);
}

ENGINE_API bool8
c_string_compare(string_t A, string_t B)
{
    if(A.count != B.count) return(false);
    if(A.data  == B.data)  return(true); 
    return(memcmp(A.data, B.data, A.count) == 0);
}

ENGINE_API bool8
c_string_ends_with(string_t A, string_t ending)
{
    bool8 result;
    if(ending.count <= A.count)
    {
        string_t A_ending = A;
        A_ending.data  += A.count - ending.count;
        A_ending.count  = ending.count;

        result = c_string_compare(A_ending, ending);
    }
    else
    {
        result = false;
    }

    return(result);
}

ENGINE_API string_t
c_string_concat(memory_arena_t *arena, string_t A, string_t B)
{
    string_t result;
    
    result.count = A.count + B.count;
    result.data  = (byte*)c_arena_push_size(arena, (result.count + 1) * sizeof(byte));
    Assert(result.data != null);

    if(A.data != null) MemoryCopy(result.data,           A.data, A.count);
    if(B.data != null) MemoryCopy(result.data + A.count, B.data, B.count);

    result.data[result.count] = '\0';
    return(result);
}

ENGINE_API string_t
c_string_make_copy(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.data = (byte*)c_arena_push_array(arena, byte, string.count + 1);
    if(result.data)
    {
        result.count = string.count;
        Assert(string.data);
        Assert(result.data);

        memcpy(result.data, string.data, string.count);
        result.data[result.count] = '\0';
    }
    else
    {
        log_error("Failed to allocate string memory...\n");
        result.data = null;
    }

    return(result);
}

ENGINE_API string_t
c_string_sub_from_left(string_t string, u32 index)
{
    string_t result;
    result.data  = string.data;
    result.count = index;

    return(result);
}

ENGINE_API string_t
c_string_sub_from_right(string_t string, u32 index)
{
    string_t result;
    result.data  = string.data  + (index + 1);
    result.count = string.count - (index + 1);

    return(result);
}

ENGINE_API string_t
c_string_substring(string_t string, u32 first_index, u32 last_index)
{
    string_t result;
    result.data  = (string.data + first_index);
    result.count = last_index - first_index;

    return(result);
}

// NOTE(Sleepster): Allowed to consume the whole string down to count == 0. 
ENGINE_API void
c_string_advance_by(string_t *string, u32 amount)
{
    Assert(string->data + amount <= string->data + string->count);
    string->data  += amount;
    string->count -= amount;
}

ENGINE_API s32
c_string_find_first_char_from_left(string_t string, char character)
{
    s32 result = -1;
    for(u32 index = 0;
        index < string.count;
        ++index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }
    }

    return(result);
}

ENGINE_API s32
c_string_find_first_char_from_right(string_t string, char character)
{
    s32 result = -1;
    for(u32 index = string.count;
        index > 0;
        --index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }
    }

    return(result);
}

ENGINE_API s32
c_string_find_first_char_from_left_on_line(string_t string, char character)
{
    s32 result = -1;
    for(u32 index = 0;
        index < string.count;
        ++index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }

        if(found == '\n')
        {
            break;
        }
    }

    return(result);
}

ENGINE_API u32
c_string_find_first_char_from_right_on_line(string_t string, char character, u32 ending_index)
{
    u32 count = ending_index;
    if(ending_index == 0)
    {
        count = string.count;
    }

    u32 result = -1;
    for(u32 index = count;
        index > 0;
        --index)
    {
        char found = (char)string.data[index];
        if(found == character)
        {
            result = index;
            break;
        }

        if(found == '\n')
        {
            break;
        }
    }

    return(result);
}

// NOTE(Sleepster): Returns the string you passed as the filename should this not actually be a path and simply the file's name 
ENGINE_API string_t
c_string_get_filename_from_path(string_t filepath)
{
    string_t result = filepath;
    s32 ext_start = c_string_find_first_char_from_right(filepath, '/');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start + 1, filepath.count);
    }

    return(result);
}

ENGINE_API string_t
c_string_get_file_ext_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_string_find_first_char_from_left(filepath, '.');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start, filepath.count);
    }

    return(result);
}

ENGINE_API const char *
c_string_to_const_array(string_t string)
{
    return((const char *)string.data);
}

ENGINE_API string_t
c_string_to_upper(memory_arena_t *arena, string_t string)
{
    string_t result = c_string_make_copy(arena, string);
    for(u32 index = 0;
        index < result.count;
        ++index)
    {
        byte *item = (result.data + index);
        *item = toupper(*item);
    }

    return(result);
}

ENGINE_API string_t
c_string_to_lower(memory_arena_t *arena, string_t string)
{
    string_t result = c_string_make_copy(arena, string);
    for(u32 index = 0;
        index < result.count;
        ++index)
    {
        byte *character = result.data + index;
        *character = tolower(*character);
    }

    return(result);
}

ENGINE_API void 
c_string_override_file_separators(string_t *string)
{
    for(u32 string_index = 0;
        string_index < string->count;
        ++string_index)
    {
        char *character = (char*)(string->data + string_index);
        if(*character == '\\')
        {
            *character = '/';
        }
    }
}

ENGINE_API string_t 
c_string_remove_characters(byte *buffer, string_t string, char character)
{
    string_t result = {};
    result.data = buffer;

    s32 new_string_count = 0;
    for(u32 string_index = 0;
        string_index < string.count;
        ++string_index)
    {
        char *current_char = (char *)(string.data + string_index);
        if(*current_char != character)
        {
            buffer[new_string_count] = *current_char;
            ++new_string_count;
        }
    }

    result.count = new_string_count;
    return(result);
}

ENGINE_API string_t
c_string_get_filename_from_path_and_ext(string_t filepath)
{
    string_t result;
    u32 first_break  = c_string_find_first_char_from_right(filepath, '/');
    result           = c_string_sub_from_right(filepath, first_break);
    u32 second_break = c_string_find_first_char_from_left(result, '.');
    result.count     = second_break;

    return(result);
}

// NOTE(Sleepster): Get current line, advance the .data pointer 
ENGINE_API string_t 
c_string_read_line(string_t *data)
{
    string_t result;

    for(u32 char_index = 0;
        char_index < data->count;
        ++char_index)
    {
        char character = (char)data->data[char_index];
        if(character == '\n' || character == '\r')
        {
            result.count = char_index + 1;
            result.data  = data->data;
            
            c_string_advance_by(data, result.count);
            break;
        }
    }

    return(result);
}

ENGINE_API s32 
c_string_read_int(string_t data)
{
    s32 result = 0;
#if 0
    for(u32 index = 0;
        index < data.count;
        ++index)
    {
        result = (result * 10) + (data.data[index] - '0');
    }
#else
    char *end_ptr = (char*)(data.data + data.count);
    result = strtol((const char*)data.data, &end_ptr, 10);
#endif

    return(result);
}

ENGINE_API u32 
c_string_read_uint(string_t data)
{
    u32 result = 0;
#if 0
    for(u32 index = 0;
        index < data.count;
        ++index)
    {
        result = (result * 10) + (data.data[index] - '0');
    }
#else
    char *end_ptr = (char*)(data.data + data.count);
    result = strtoul((const char*)data.data, &end_ptr, 10);
#endif

    return(result);
}

ENGINE_API float32 
c_string_read_float32(string_t data)
{
    float32 result = 0;

    char *end_ptr = (char*)(data.data + data.count);
    result = strtof((const char*)data.data, &end_ptr);

    return(result);
}

ENGINE_API float64
c_string_read_float64(string_t data)
{
    float64 result = 0;

    char *end_ptr = (char*)(data.data + data.count);
    result = strtod((const char*)data.data, &end_ptr);

    return(result);
}

ENGINE_API string_t 
c_string_sprintf(char *buffer, u32 buffer_size, const char *string, ...)
{
    string_t result;

    va_list arg_ptr;
    va_start(arg_ptr, string);
    s32 length = vsnprintf(buffer, buffer_size, string, arg_ptr);
    va_end(arg_ptr);

    result = {
        .data  = (u8*)buffer,
        .count = (u32)length
    };

    return(result);
}

ENGINE_API bool32
c_string_is_end_of_line(string_t *current_line)
{
    bool32 result = false;

    char character = current_line->data[0]; 
    if(character == '\n' ||
       character == '\r' || 
       current_line->count == 0)
    {
        result = true;
    }

    return(result);
}

ENGINE_API bool32
c_string_is_whitespace(string_t *current_line)
{
    Expect(current_line->count > 0, "Attempted to perform this on an empty string...\n");
    bool32 result = false;

    char character = current_line->data[0];
    if(character != '\0') 
    {
        if(character == ' '  ||
           character == '\n' ||
           character == '\r' ||
           character == '\t')
        {
            result = true;
        }
    }

    return(result);
}

ENGINE_API u32
c_string_eat_whitespace(string_t *current_line)
{
    u32 result = 0;

    while(current_line->count > 0 && current_line->data[0] != '\0')
    {
        if(c_string_is_whitespace(current_line))
        {
            if(current_line->data[0] == '\n') result++;
            c_string_advance_by(current_line, 1);
        }
        else if(current_line->data[0] == '/' &&
                current_line->data[1] == '/')
        {
            c_string_advance_by(current_line, 2);
            while(!c_string_is_end_of_line(current_line))
            {
                if(current_line->data[0] == '\n') result++;
                c_string_advance_by(current_line, 1);
            }
        }
        else if(current_line->data[0] == '/' &&
                current_line->data[1] == '*')
        {
            c_string_advance_by(current_line, 2);
            while(current_line->data[0] && 
                 (current_line->count > 0) && 
                 !((current_line->data[0] == '*') && 
                   (current_line->data[1] == '/')))
            {
                if(current_line->data[0] == '\n') result++;
                c_string_advance_by(current_line, 1);
            }

            if(current_line->data[0] == '*')
            {
                c_string_advance_by(current_line, 2);
            }
        }
        else
        {
            break;
        }
    }

    return(result);
}

ENGINE_API u32
c_string_get_whitespace_size(string_t string)
{
    u32 result = 0;
    u32 old_string_count = string.count;

    c_string_eat_whitespace(&string);
    result = old_string_count - string.count;

    return(result);
}

ENGINE_API u32 
c_string_get_current_line_size(string_t string)
{
    u32 result = 0;

    string_t line = c_string_read_line(&string);
    result        = line.count;

    return(result);
}

ENGINE_API u32
c_string_find_all_instances_of(string_t string, u8 character)
{
    u32 result = 0;
    for(u32 index = 0;
        index < string.count;
        ++index)
    {
        if(character == string.data[index])
        {
            ++result;
        }
    }

    return(result);
}

ENGINE_API string_t
c_string_replace_all_instances_of(memory_arena_t *arena, string_t string, u8 character, u8 replacement)
{
    string_t result = {};

    result = c_string_make_copy(arena, string);
    for(u32 index = 0;
        index < string.count;
        ++index)
    {
        if(character == string.data[index])
        {
            result.data[index] = replacement;
        }
    }

    return(result);
}

///////////////////
// STRING BUILDER
///////////////////

ENGINE_API void
c_string_builder_init(string_builder_t *builder)
{
    builder->first_buffer.data        = builder->intial_memory;
    builder->first_buffer.capacity    = DEFAULT_STRING_BUILDER_CAPACITY;
    builder->first_buffer.bytes_used  = 0;
    builder->first_buffer.next_buffer = null;
    builder->current_buffer           = &builder->first_buffer;

    builder->is_initialized = true;
}

ENGINE_API void
c_string_builder_deinit(string_builder_t *builder)
{
    if(builder->first_buffer.next_buffer)
    {
        for(string_builder_buffer_t *current_buffer = builder->first_buffer.next_buffer;
            current_buffer;
            current_buffer = current_buffer->next_buffer)
        {
            c_free_alloc(current_buffer);
        }
    }

    builder->first_buffer.next_buffer = null;
    builder->is_initialized = false;
}

internal_api string_builder_buffer_t *
c_string_builder_get_valid_buffer(string_builder_t *builder, s32 new_size)
{
    if(!builder->is_initialized)
    {
        c_string_builder_init(builder);
    }

    string_builder_buffer_t *result = builder->current_buffer;
    if((result->bytes_used + new_size) > result->capacity)
    {
        s32 capacity = Max((s32)DEFAULT_STRING_BUILDER_CAPACITY, new_size);
        byte *data = (byte*)c_alloc(capacity + sizeof(string_builder_buffer_t), ALLOCATOR_TAG_STRINGS);

        result->next_buffer = (string_builder_buffer_t *)data;
        result               = result->next_buffer;
        result->bytes_used   = 0;
        result->capacity     = capacity;
        result->data         = data + sizeof(string_builder_buffer_t);
        result->next_buffer  = null;

        builder->current_buffer = result;
    }

    return(result);
}

ENGINE_API void
c_string_builder_append_data(string_builder_t *builder, string_t string)
{
    string_builder_buffer_t *current_buffer = c_string_builder_get_valid_buffer(builder, string.count);
    Assert(current_buffer);

    memcpy(current_buffer->data + current_buffer->bytes_used, string.data, string.count);
    current_buffer->bytes_used += string.count;
}

ENGINE_API string_t
c_string_builder_copy_builder_string(string_builder_t *builder, s32 allocation_tag)
{
    string_t result = {};

    s32 string_size = 0;
    for(string_builder_buffer_t *current_buffer = &builder->first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        string_size += current_buffer->bytes_used;
    }

    byte *string_data = (byte*)c_alloc(string_size + 1, allocation_tag);
    result.data  = string_data;
    result.count = string_size;

    s32 current_offset = 0;
    for(string_builder_buffer_t *current_buffer = &builder->first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        memcpy(result.data + current_offset, current_buffer->data, current_buffer->bytes_used);
        current_offset += current_buffer->bytes_used;
    }

    string_data[string_size] = '\0';
    return(result);
}

ENGINE_API void
c_string_builder_append_value(string_builder_t *builder, void *value, u32 value_size)
{
    string_t value_string = {(byte*)value, value_size};
    c_string_builder_append_data(builder, value_string);
}

ENGINE_API void
c_string_builder_reset(string_builder_t *builder)
{
    c_string_builder_deinit(builder);
    c_string_builder_init(builder);
}

ENGINE_API void
c_string_builder_dump_to_file(string_builder_t *builder, file_t *file)
{
    string_t builder_string = c_string_builder_copy_builder_string(builder, ALLOCATOR_TAG_CACHE);
    defer(c_free_alloc(builder_string.data));

    c_file_write_string(file, builder_string);
}

ENGINE_API void
c_string_builder_flush_to_file(string_builder_t *builder, file_t *file)
{
    c_string_builder_dump_to_file(builder, file);
    c_string_builder_reset(builder);
}

ENGINE_API void
c_string_builder_append_builder(string_builder_t *A, string_builder_t *B)
{
    string_t builder_string = c_string_builder_copy_builder_string(B, ALLOCATOR_TAG_CACHE);
    defer(c_free_alloc(builder_string.data));

    c_string_builder_append_data(A, builder_string);
}

ENGINE_API void
c_string_builder_sprintf(string_builder_t *builder, const char *string, ...)
{
    char buffer[32000] = {};

    va_list arg_ptr;
    va_start(arg_ptr, string);
    s32 length = vsnprintf(buffer, sizeof(buffer), string, arg_ptr);
    va_end(arg_ptr);

    string_t builder_string = {
        .data  = (u8*)buffer,
        .count = (u32)length
    };

    c_string_builder_append_data(builder, builder_string);
}
