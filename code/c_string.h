#if !defined(C_STRING_H)
/* ========================================================================
   $File: c_string.h $
   $Date: December 03 2025 03:26 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_STRING_H
#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_globals.h>

typedef struct file file_t;

// TODO(Sleepster): Wide strings? wstring_t?

// NOTE(Sleepster): A string is essentially just a byte array. The values in here of ASCII size,
//                  but this could potentially be UTF-8 in the future.  
typedef struct string
{
    byte *data;
    u32   count;
}string_t;

//////////// API DEFINITIONS //////////////
u32         c_string_length(const char *c_string);
bool8       c_string_is_valid(string_t string);
string_t    c_string_create(const char *c_string);
string_t    c_string_create_with_length(byte *data, u32 length);
string_t    c_string_make_heap(memory_arena_t *arena, string_t string);
bool8       c_string_compare(string_t A, string_t B);
string_t    c_string_concat(memory_arena_t *arena, string_t A, string_t B);
const char *c_string_to_const_array(string_t string);

string_t    c_string_make_copy(memory_arena_t *arena, string_t string);
string_t    c_string_sub_from_left(string_t string, u32 index);
string_t    c_string_sub_from_right(string_t string, u32 index);
string_t    c_string_substring(string_t string, u32 first_index, u32 last_index);
void        c_string_advance_by(string_t *string, u32 advance);

u32         c_string_find_first_char_from_left(string_t string,  char character);
u32         c_string_find_first_char_from_right(string_t string, char character);
string_t    c_string_get_filename_from_path(string_t filepath);
string_t    c_string_get_file_ext_from_path(string_t filepath);
string_t    c_string_get_filename_from_path_and_ext(string_t filepath);
void        c_string_override_file_separators(string_t *string);

// TODO(Sleepster): Test These
string_t    c_string_read_line(string_t *data);

s8          c_string_read_s8(string_t data);
s16         c_string_read_s16(string_t data);
s32         c_string_read_s32(string_t data);
s64         c_string_read_s64(string_t data);

u8          c_string_read_u8(string_t data);
u16         c_string_read_u16(string_t data);
u32         c_string_read_u32(string_t data);
u64         c_string_read_u64(string_t data);

float32     c_string_read_float32(string_t data);
float64     c_string_read_float64(string_t data);

bool8       c_string_read_bool8(string_t data);
bool32      c_string_read_bool32(string_t data);

string_t    c_string_format(string_t string, ...);

// MACROS
#define STR(x)   (string_t){.data = (byte*)x, .count = c_string_length(x)}
#define C_STR(x) ((const char *)x.data)
///////////////////////////////////////////
// STRING BUILDER
///////////////////////////////////////////

typedef struct string_builder_buffer
{
    byte *buffer_data;
    u32   bytes_used;
    u32   buffer_size;

    struct string_builder_buffer *next_buffer;
}string_builder_buffer_t;

// NOTE(Sleepster): We just use a memory arena here since everything within this builder will live and die together... 
typedef struct string_builder
{
    bool8                        is_initialized;
    memory_arena_t               arena;

    string_builder_buffer_t *first_buffer;
    string_builder_buffer_t *current_buffer;
    u64                          default_buffer_block_size;

    u64                          bytes_used;
    u64                          total_allocated;
}string_builder_t;

void     c_string_builder_init(string_builder_t *builder, u64 buffer_block_size);
void     c_string_builder_deinit(string_builder_t *builder);
void     c_string_builder_append_data(string_builder_t *builder, string_t data);
void     c_string_builder_append_value(string_builder_t *builder, void *value, u32 value_size);
string_t c_string_builder_get_current_string(string_builder_t *builder);

// NOTE(Sleepster): DUMP simply writes the data out and keeps the state of the builder the same, 
//                  FLUSH writes out the data, and completely resets the state of the builder
bool8 c_string_builder_dump_to_file(file_t *file, string_builder_t *builder);
bool8 c_string_builder_flush_to_file(file_t *file, string_builder_t *builder);

#endif // C_STRING_H

