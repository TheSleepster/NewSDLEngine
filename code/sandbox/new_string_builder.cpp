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

#define STRING_BUILDER_INITIAL_BUFFER_SIZE (2048)
#define STRING_BUILDER_BUFFER_DEBUG_FLAG   (0xBEEF)

typedef struct new_string_builder_buffer
{
    struct new_string_builder_buffer *next_buffer;

    s64   buffer_size;
    s64   buffer_used;

    byte *data;
}new_string_builder_buffer_t;

typedef struct new_string_builder
{
    bool32         is_initialized;
    memory_arena_t arena;

    // NOTE(Sleepster): Stack buffer to prevent stupid heap allocations 
    struct {
        new_string_builder_buffer_t *next_buffer;
        s64                          buffer_size;
        s64                          buffer_used;
        byte                         data[STRING_BUILDER_INITIAL_BUFFER_SIZE];
    }first_buffer;
    new_string_builder_buffer_t *last_buffer;

    // NOTE(Sleepster): These two concern themselves only with the STRING related allocations.
    // Not the allocations for the string_buffers themselves.
    s64    bytes_used;
    s64    bytes_allocated;
    s64    builder_buffer_block_size;
}new_string_builder_t;

void
string_builder_init(new_string_builder_t *builder, s64 buffer_size)
{
    Assert(builder->is_initialized == false);
    builder->arena                     = c_arena_create(MB(10));
    builder->builder_buffer_block_size = buffer_size;
    builder->is_initialized            = true;

    builder->last_buffer = (new_string_builder_buffer_t*)&builder->first_buffer;
}

void
string_builder_deinit(new_string_builder_t *builder)
{
    (void)builder;
}

internal_api new_string_builder_buffer_t*
string_builder_create_new_buffer(new_string_builder_t *builder)
{
    new_string_builder_buffer_t *result = null;
    if(!builder->is_initialized)
    {
        string_builder_init(builder, STRING_BUILDER_INITIAL_BUFFER_SIZE);
    }

    result = c_arena_push_struct(&builder->arena, new_string_builder_buffer_t);
    result->data        = c_arena_push_array(&builder->arena, byte, builder->builder_buffer_block_size);
    result->buffer_size = builder->builder_buffer_block_size;
    result->buffer_used = 0;

    builder->bytes_allocated += builder->builder_buffer_block_size;

    builder->last_buffer->next_buffer = result;
    builder->last_buffer              = result;

    return(result);
}

internal_api true_inline new_string_builder_buffer_t*
string_builder_get_first_buffer(new_string_builder_t *builder)
{
    new_string_builder_buffer_t *result = (new_string_builder_buffer_t*)&builder->first_buffer;
    result->data        = builder->first_buffer.data;
    result->buffer_size = STRING_BUILDER_INITIAL_BUFFER_SIZE;

    return(result);
}

internal_api true_inline new_string_builder_buffer_t*
string_builder_get_current_buffer(new_string_builder_t *builder)
{
    new_string_builder_buffer_t *result = (builder->last_buffer) ? builder->last_buffer : string_builder_get_first_buffer(builder);
    if(result->buffer_used == result->buffer_size)
    {
        result = string_builder_create_new_buffer(builder);
    }

    return(result);
}

void
string_builder_append_data(new_string_builder_t *builder, string_t data)
{
    s64 bytes_written = 0;
    while(bytes_written != data.count)
    {
        new_string_builder_buffer_t *current_buffer = string_builder_get_current_buffer(builder);
        s64 bytes_to_write = Min(data.count - bytes_written, current_buffer->buffer_size - current_buffer->buffer_used);

        memcpy(current_buffer->data + current_buffer->buffer_used, data.data, bytes_to_write);
        current_buffer->buffer_used += bytes_to_write;
        builder->bytes_used         += bytes_to_write;

        bytes_written += bytes_to_write;
    }
}

void
string_builder_get_builder_string(new_string_builder_t *builder, string_t *buffer)
{
    if(builder->bytes_used > 0)
    {
        Assert(buffer->count >= builder->bytes_used);

        // NOTE(Sleepster): Grab contents of the initial_buffer before anything 
        s32 initial_offset = 0;
        for(new_string_builder_buffer_t *current_buffer = string_builder_get_first_buffer(builder);
            current_buffer;
            current_buffer = current_buffer->next_buffer)
        {
            memcpy(buffer->data + initial_offset, current_buffer->data, current_buffer->buffer_used);
            initial_offset += current_buffer->buffer_used;
        }
    }
}

int
main(void)
{
    new_string_builder_t builder = {};

    string_t string = c_file_read_entirety(STR("../code/vk_backend_core.h"));
    string_builder_append_data(&builder, string);

    memory_arena_t arena = c_arena_create(MB(builder.bytes_used));
    string_t builder_string = {
        .data  = c_arena_push_array(&arena, byte, builder.bytes_used),
        .count = builder.bytes_used 
    };

    string_builder_get_builder_string(&builder, &builder_string);
    printf("%.*s\n", fprint_string(builder_string));

    return(0);
}
