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

// TODO(Sleepster): Wide strings? string_u16_t?

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

// MACROS
#define STR(x)   (string_t){.data = (byte*)x, .count = c_string_length(x)}
#define C_STR(x) ((char *)x.data)
///////////////////////////////////////////

#define STRING_BUILDER_BUFFER_SIZE (4096 - sizeof(string_builder_buffer_t))

typedef struct string_builder_buffer
{
    s64                           bytes_allocated;
    s64                           bytes_used;
    struct string_builder_buffer *next_buffer;
}string_builder_buffer_t;

typedef struct string_builder
{
    bool8                    is_initialized;

    usize                    new_buffer_size;
    string_builder_buffer_t *current_buffer;
    s64                      total_allocated;

    byte                     initial_bytes[STRING_BUILDER_BUFFER_SIZE];
}string_builder_t;

//////////// API DEFINITIONS //////////////
void                     c_string_builder_init(string_builder_t *builder, usize new_buffer_size);
string_builder_buffer_t* c_string_builder_get_base_buffer(string_builder_t *builder);
string_builder_buffer_t* c_string_builder_get_current_buffer(string_builder_t *builder);
byte*                    c_string_builder_get_buffer_data(string_builder_buffer_t *buffer);
bool8                    c_string_builder_create_new_buffer(string_builder_t *builder);
void                     c_string_builder_append(string_builder_t *builder, string_t data);
void                     c_string_builder_append_value(string_builder_t *builder, void *value_ptr, byte len);
string_t                 c_string_builder_get_string(string_builder_t *builder);
void                     c_string_builder_write_to_file(file_t *file, string_builder_t *builder);
s32                      c_string_builder_get_string_length(string_builder_t *builder);

// TODO(Sleepster):
void c_string_builder_ensure_contiguous_space(string_builder_t *builder, usize byte_count);
///////////////////////////////////////////


#endif // C_STRING_H

