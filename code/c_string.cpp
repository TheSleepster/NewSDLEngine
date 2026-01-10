/* ========================================================================
   $File: c_string.cpp $
   $Date: December 08 2025 08:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>
#include <stdarg.h>

#include <c_string.h>
#include <c_math.h>
#include <c_globals.h>

#include <c_file_api.h>
#include <c_file_watcher.h>

u32
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

bool8
c_string_is_valid(string_t string)
{
    return((string.count != 0) && (string.data != null));
}

string_t
c_string_create(const char *c_string)
{
    string_t result;
    result.count = c_string_length(c_string);
    result.data  = (byte*)c_string;

    return(result);
}

string_t
c_string_create_with_length(byte *data, u32 length)
{
    string_t result;
    result.data  = data;
    result.count = length;

    return(result);
}

string_t
c_string_make_heap(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.count = string.count;
    result.data  = c_arena_push_array(arena, byte, string.count);

    MemoryCopy(result.data, string.data, string.count * sizeof(byte));
    return(result);
}

bool8
c_string_compare(string_t A, string_t B)
{
    if(A.count != B.count) return false;
    return(memcmp(A.data, B.data, A.count) == 0);
}

string_t
c_string_concat(memory_arena_t *arena, string_t A, string_t B)
{
    string_t result;
    
    result.count = A.count + B.count;
    result.data  = (byte*)c_arena_push_size(arena, (result.count + 1) * sizeof(byte));
    Assert(result.data != null);

    MemoryCopy(result.data,           A.data, A.count);
    MemoryCopy(result.data + A.count, B.data, B.count);

    result.data[result.count] = '\0';
    return(result);
}

string_t
c_string_make_copy(memory_arena_t *arena, string_t string)
{
    string_t result;
    result.data = (byte*)c_arena_push_array(arena, byte, string.count);
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
    }

    return(result);
}

string_t
c_string_sub_from_left(string_t string, u32 index)
{
    string_t result;
    result.data  = string.data + index;
    result.count = string.count - index;

    return(result);
}

string_t
c_string_sub_from_right(string_t string, u32 index)
{
    string_t result;
    result.data  = string.data  + (index + 1);
    result.count = string.count - (index + 1);

    return(result);
}

string_t
c_string_substring(string_t string, u32 first_index, u32 last_index)
{
    string_t result;
    result.data  = (string.data + first_index);
    result.count = last_index - first_index;

    return(result);
}

void
c_string_advance_by(string_t *string, u32 amount)
{
    Assert(amount < string->count);
    string->data  += amount;
    string->count -= amount;
}

u32
c_string_find_first_char_from_left(string_t string, char character)
{
    u32 result = -1;
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

u32
c_string_find_first_char_from_right(string_t string, char character)
{
    u32 result = -1;
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

string_t
c_string_get_filename_from_path(string_t filepath)
{
    string_t result = {};
    s32 ext_start = c_string_find_first_char_from_right(filepath, '/');
    if(ext_start != -1)
    {
        result = c_string_substring(filepath, ext_start + 1, filepath.count);
    }

    return(result);
}

string_t
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

const char *
c_string_to_const_array(string_t string)
{
    return((const char *)string.data);
}

void 
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

string_t
c_string_get_filename_from_path_and_ext(string_t filepath)
{
    string_t result;
    u32 first_break  = c_string_find_first_char_from_right(filepath, '/');
    result           = c_string_sub_from_right(filepath, first_break);
    u32 second_break = c_string_find_first_char_from_left(result, '.');
    result.count     = second_break;

    return(result);
}

string_t 
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
        }
    }

    return(result);
}

// NOTE(Sleepster): Element size is in bytes
internal_api byte*
c_string_read_value(string_t *data, u32 element_size)
{
    byte *result = null;
    result = data->data;
    result[element_size] = '\0';

    return(result);
}

s8
c_string_read_s8(string_t data)
{
    s8 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(s8));
    result = ((s8)*value_string - (s8)0x30);

    return(result);
}

s16 
c_string_read_s16(string_t data)
{
    s16 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(s16));

    for(u32 index = 0;
        index < sizeof(s16);
        ++index)
    {
        result += ((s16)value_string[index] - (s16)0x30);
    }

    return(result);
}

s32 
c_string_read_s32(string_t data)
{
    s32 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(s32));

    for(u32 index = 0;
        index < sizeof(s32);
        ++index)
    {
        result += ((s32)value_string[index] - (s32)0x30);
    }

    return(result);
}

s64 
c_string_read_s64(string_t data)
{
    s64 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(s64));

    for(u32 index = 0;
        index < sizeof(s64);
        ++index)
    {
        result += ((s64)value_string[index] - (s64)0x30);
    }

    return(result);
}

// NOTE(Sleepster): Copy pasta from above
u8
c_string_read_u8(string_t data)
{
    u8 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(u8));
    result = ((u8)*value_string - (u8)0x30);

    return(result);
}

u16 
c_string_read_u16(string_t data)
{
    u16 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(u16));

    for(u32 index = 0;
        index < sizeof(u16);
        ++index)
    {
        result += ((u16)value_string[index] - (u16)0x30);
    }

    return(result);
}

u32 
c_string_read_u32(string_t data)
{
    u32 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(u32));

    for(u32 index = 0;
        index < sizeof(u32);
        ++index)
    {
        result += ((u32)value_string[index] - (u32)0x30);
    }

    return(result);
}

u64 
c_string_read_u64(string_t data)
{
    u64 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(u64));

    for(u32 index = 0;
        index < sizeof(u64);
        ++index)
    {
        result += ((u64)value_string[index] - (u64)0x30);
    }

    return(result);
}

float32 
c_string_read_float32(string_t data)
{
    float32 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(float32));
    result = strtof((char*)value_string, null);

    return(result);
}

float64 
c_string_read_float64(string_t data)
{
    float64 result = 0;
    byte *value_string = c_string_read_value(&data, sizeof(float64));
    result = strtod((char*)value_string, null);

    return(result);
}

bool8 
c_string_read_bool8(string_t data)
{
    bool8 result = false;
    byte *value_string = c_string_read_value(&data, sizeof(bool8));

    result = ((bool8)*value_string);

    return(result);
}

bool32 
c_string_read_bool32(string_t data)
{
    bool32 result = false;
    byte *value_string = c_string_read_value(&data, sizeof(bool32));
    result = ((bool32)*value_string);

    return(result);
}

string_t
c_string_format(string_t memory, string_t string, ...)
{
    string_t result;

    va_list arg_ptr;
    va_start(arg_ptr, string);
    vsnprintf((char*)memory.data, memory.count, (char*)string.data, arg_ptr);
    va_end(arg_ptr);
    
    result.data  = memory.data;
    result.count = memory.count;

    return(result);
}

///////////////////
// STRING BUILDER
///////////////////

// TODO(Sleepster): c_string_builder_deinit 

string_builder_buffer_t*
c_string_builder_get_base_buffer(string_builder_t *builder)
{
    return((string_builder_buffer_t*)builder->initial_bytes);
}

void
c_string_builder_init(string_builder_t *builder, usize new_buffer_size)
{
    Assert(builder->is_initialized == false);
    
    builder->total_allocated = 0;
    builder->new_buffer_size = new_buffer_size;
    memset(builder->initial_bytes, 0, STRING_BUILDER_BUFFER_SIZE);

    string_builder_buffer_t *buffer = c_string_builder_get_base_buffer(builder);
    buffer->bytes_allocated         = STRING_BUILDER_BUFFER_SIZE;
    buffer->bytes_used              = 0;
    buffer->next_buffer             = null;

    builder->current_buffer   = buffer;
    builder->total_allocated += buffer->bytes_allocated;

    builder->is_initialized   = true;
}

string_builder_buffer_t*
c_string_builder_get_current_buffer(string_builder_t *builder)
{
    if(builder->current_buffer) return(builder->current_buffer);
    return(c_string_builder_get_base_buffer(builder));
}

byte*
c_string_builder_get_buffer_data(string_builder_buffer_t *buffer)
{
    return((byte*)buffer + sizeof(string_builder_buffer_t));
}

bool8
c_string_builder_create_new_buffer(string_builder_t *builder)
{
    bool8 result = false;
    usize new_size = builder->new_buffer_size > 0 ? builder->new_buffer_size : STRING_BUILDER_BUFFER_SIZE;

    byte *bytes = (byte *)AllocSize(new_size);
    if(bytes)
    {
        ZeroMemory(bytes, new_size);

        string_builder_buffer_t *buffer = (string_builder_buffer_t *)bytes;
        buffer->next_buffer     = null;
        buffer->bytes_used      = 0;
        buffer->bytes_allocated = new_size - sizeof(string_builder_buffer_t);

        string_builder_buffer_t *last_buffer = c_string_builder_get_current_buffer(builder);
        last_buffer->next_buffer  = buffer;
        builder->current_buffer   = buffer; 
        builder->total_allocated += buffer->bytes_allocated;

        result = true;
    }

    return(result);
}

void 
c_string_builder_append(string_builder_t *builder, string_t data)
{
    if(!builder->is_initialized) c_string_builder_init(builder, STRING_BUILDER_BUFFER_SIZE);

    byte *bytes  = data.data;
    u32 length = data.count;
    while(length > 0)
    {
        string_builder_buffer_t *buffer = c_string_builder_get_current_buffer(builder);
        u32 length_max = buffer->bytes_allocated - buffer->bytes_used;
        if(length_max <= 0)
        {
            bool8 success = c_string_builder_create_new_buffer(builder);
            if(!success)
            {
                log_error("Failure to expand the string builder...\n");
                return;
            }

            buffer = builder->current_buffer;
            Assert(buffer != null);

            length_max = buffer->bytes_allocated - buffer->bytes_used;
            Assert(length_max > 0);
        }

        u32 bytes_to_copy = Min(length, length_max);
        memcpy(c_string_builder_get_buffer_data(buffer) + buffer->bytes_used, bytes, bytes_to_copy);

        length -= bytes_to_copy;
        bytes  += bytes_to_copy;

        buffer->bytes_used += bytes_to_copy;
    }
}

void
c_string_builder_append_value(string_builder_t *builder, void *value_ptr, byte len)
{
    string_t builder_string;
    builder_string.data  = (byte*)c_arena_push_size(&global_context->temporary_arena, sizeof(u8) * len);
    memcpy(builder_string.data, value_ptr, len);
    builder_string.count = len;
    c_string_builder_append(builder, builder_string);
}

string_t
c_string_builder_get_string(string_builder_t *builder)
{
    string_t result = {};
    string_builder_buffer_t *buffer = c_string_builder_get_current_buffer(builder);

    while(buffer)
    {
        string_t temp_string;
        temp_string.data  = c_string_builder_get_buffer_data(buffer);
        temp_string.count = buffer->bytes_used;

        result = c_string_concat(&global_context->temporary_arena, result, temp_string);

        buffer = buffer->next_buffer;
    }

    return(result);
}

bool8 
c_string_builder_write_to_file(file_t *file, string_builder_t *builder)
{
    Assert(file->for_writing);

    bool8 result;
    string_t string_to_write = c_string_builder_get_string(builder);
    result = c_file_write_string(file, string_to_write);

    return(result);
}

s32
c_string_builder_get_string_length(string_builder_t *builder)
{
    s32 result = 0;
    string_builder_buffer_t *buffer = c_string_builder_get_base_buffer(builder);
    while(buffer)
    {
        result += buffer->bytes_used;
        buffer = buffer->next_buffer;
    }

    return(result);
}

////////////////////////////

internal_api new_string_builder_buffer_t*
c_new_string_builder_create_and_attach_buffer(new_string_builder_t *builder, u64 block_size)
{
    new_string_builder_buffer_t *result = c_arena_push_struct(&builder->arena, new_string_builder_buffer_t);
    Assert(result);

    result->buffer_size = block_size;
    result->next_buffer = null;
    result->prev_buffer = builder->current_buffer;
    result->buffer_data = c_arena_push_size(&builder->arena, block_size - sizeof(new_string_builder_buffer_t));

    builder->current_buffer->next_buffer = result;
    return(result);
}

// NOTE(Sleepster): I trust you won't call this while the builder is actually initialized.... 
void
c_new_string_builder_init(new_string_builder_t *builder, u64 buffer_block_size)
{
    ZeroStruct(*builder);

    u64 block_size = Align16(buffer_block_size + sizeof(new_string_builder_buffer_t));
    builder->arena                     =  c_arena_create(block_size);
    builder->default_buffer_block_size =  block_size;
    builder->first_buffer              =  c_new_string_builder_create_and_attach_buffer(builder, block_size);
    builder->current_buffer            =  builder->first_buffer;
    builder->is_initialized            =  true;
}

void
c_new_string_builder_deinit(new_string_builder_t *builder)
{
    c_arena_destroy(&builder->arena);
    builder->is_initialized = false;
}

internal_api void
c_new_string_builder_advance_buffer(new_string_builder_t *builder)
{
    new_string_builder_buffer_t *next_buffer = builder->current_buffer->next_buffer;
    if(!next_buffer)
    {
        next_buffer = c_new_string_builder_create_and_attach_buffer(builder, builder->default_buffer_block_size);
    }
    builder->current_buffer = next_buffer;
}

void
c_new_string_builder_append_data(new_string_builder_t *builder, string_t data)
{
    new_string_builder_buffer_t *current_buffer = builder->current_buffer;
    if(current_buffer->bytes_used + data.count > current_buffer->buffer_size)
    {
        c_new_string_builder_advance_buffer(builder);
        Assert(current_buffer->next_buffer);
        Assert(current_buffer->buffer_data);
    }

    byte *buffer_data = current_buffer->buffer_data + current_buffer->bytes_used;
    memcpy(buffer_data, data.data, data.count);

    current_buffer->bytes_used += data.count;
}

void
c_new_string_builder_append_value(new_string_builder_t *builder, void *value, u32 value_size)
{
    string_t value_string = {
        .data  = (byte*)value,
        .count = value_size
    };

    c_new_string_builder_append_data(builder, value_string);
}

string_t
c_new_string_builder_get_current_string(new_string_builder_t *builder)
{
    string_t result;
    for(new_string_builder_buffer_t *current_buffer = builder->first_buffer;
        current_buffer != null;
        current_buffer = current_buffer->next_buffer)
    {
        string_t buffer_string = c_string_create_with_length(current_buffer->buffer_data, current_buffer->bytes_used);
        result = c_string_concat(&builder->arena, result, buffer_string);
    }

    return(result);
}

bool8
c_new_string_builder_dump_to_file(file_t *file, new_string_builder_t *builder)
{
    bool8 result = false;
    string_t builder_string = c_new_string_builder_get_current_string(builder);
    result = c_file_write_string(file, builder_string);

    return(result);
}

bool8 
c_new_string_builder_flush_to_file(file_t *file, new_string_builder_t *builder)
{
    bool8 result = c_new_string_builder_dump_to_file(file, builder);
    c_arena_reset(&builder->arena);

    return(result);
}
