/* ========================================================================
   $File: new_string_builder.cpp $
   $Date: January 10 2026 07:06 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>
#include <c_file_api.h>
#include <c_memory_arena.h>
#include <c_string.h>

#if 0
ENGINE_API void
c_string_builder_init(string_builder_t *builder)
{
    builder->first_buffer.data       = builder->intial_memory;
    builder->first_buffer.capacity   = DEFAULT_STRING_BUILDER_CAPACITY;
    builder->first_buffer.bytes_used = 0;

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
            c_free_alloc(current_buffer->data);
        }
    }

    builder->is_initialized = false;
}

internal_api string_builder_buffer_t *
c_string_builder_get_valid_buffer(string_builder_t *builder, s32 new_size)
{
    string_builder_buffer_t *result = null;
    if(!builder->is_initialized)
    {
        c_string_builder_init(builder);
    }

    string_builder_buffer_t *last_buffer = null;
    for(string_builder_buffer_t *current_buffer = &builder->first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        last_buffer = current_buffer;
        if((current_buffer->bytes_used + new_size) < current_buffer->capacity)
        {
            result = current_buffer;
            break;
        }
    }

    if(!result)
    {
        byte *data = (byte*)c_alloc(DEFAULT_STRING_BUILDER_CAPACITY + sizeof(string_builder_buffer_t), ALLOCATOR_TAG_STRINGS);
        result = (string_builder_buffer_t *)data;

        result->data     = data + sizeof(string_builder_buffer_t);
        result->capacity = DEFAULT_STRING_BUILDER_CAPACITY;

        last_buffer->next_buffer = result;
    }

    return(result);
}

ENGINE_API void
c_string_builder_append_data(string_builder_t *builder, string_t string)
{
    string_builder_buffer_t *current_buffer = c_string_builder_get_valid_buffer(builder, string.count);
    Assert(current_buffer);

    memcpy(current_buffer->data, string.data, string.count);
    current_buffer->bytes_used += string.count;
}

ENGINE_API string_t
string_builder_copy_builder_string(string_builder_t *builder)
{
    string_t result = {};

    s32 string_size = 0;
    for(string_builder_buffer_t *current_buffer = &builder->first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        string_size += current_buffer->bytes_used;
    }

    byte *string_data = (byte*)c_alloc(string_size, ALLOCATOR_TAG_STRINGS);
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
    string_t builder_string = c_string_builder_copy_builder_string(builder);
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
    string_t builder_string = c_string_builder_copy_builder_string(B);
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
#endif

int
main(void)
{
    c_global_context_init();

#if 0
    string_builder_t new_builder = {};
    string_t string = STR("Ryan sucks...\n");

    c_string_builder_append_data(&new_builder, string);
    string_t new_string = c_string_builder_copy_builder_string(&new_builder);
    (void)new_string;
#endif

    return(0);
}
