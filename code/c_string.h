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

typedef struct file file_t;

// TODO(Sleepster): Wide strings? wstring_t?

// NOTE(Sleepster): A string is essentially just a byte array. The values in here of ASCII size,
//                  but this could potentially be UTF-8 in the future.  
typedef struct string
{
    byte *data;
    u32   count;
}string_t;

// TODO(Sleepster): - [ ] Any function in here that allocates memory should be reordered like the file API 

//////////// API DEFINITIONS //////////////
u32         c_string_length(const char *c_string);
char*       c_string_null_terminated(memory_arena_t *arena, string_t data);
bool8       c_string_is_valid(string_t string);
string_t    c_string_create(const char *c_string);
string_t    c_string_create_with_length(byte *data, u32 length);
string_t    c_string_make_heap(memory_arena_t *arena, string_t string);
bool8       c_string_compare(string_t A, string_t B);
bool8       c_string_ends_with(string_t A, string_t ending);
string_t    c_string_concat(memory_arena_t *arena, string_t A, string_t B);
const char *c_string_to_const_array(string_t string);
string_t    c_string_to_upper(string_t string);

string_t    c_string_sprintf(char *buffer, u32 buffer_size, const char *string, ...);
string_t    c_string_make_copy(memory_arena_t *arena, string_t string);
string_t    c_string_sub_from_left(string_t string, u32 index);
string_t    c_string_sub_from_right(string_t string, u32 index);
string_t    c_string_substring(string_t string, u32 first_index, u32 last_index);
void        c_string_advance_by(string_t *string, u32 advance);

u32         c_string_find_first_char_from_left(string_t string,  char character);
u32         c_string_find_first_char_from_right(string_t string, char character);
u32         c_string_find_first_char_from_left_on_line(string_t string,  char character);
// NOTE(Sleepster): This takes that "ending index" so that you don't have to parse the whole string.
// if it's 0, we just use the string length
u32         c_string_find_first_char_from_right_on_line(string_t string, char character, u32 ending_index);

string_t    c_string_get_filename_from_path(string_t filepath);
string_t    c_string_get_file_ext_from_path(string_t filepath);
string_t    c_string_get_filename_from_path_and_ext(string_t filepath);
void        c_string_override_file_separators(string_t *string);

bool32      c_string_is_whitespace(string_t *current_line);
bool32      c_string_is_end_of_line(string_t *current_line);
// NOTE(Sleepster): Returns the number of new_line characters seen
u32         c_string_eat_whitespace(string_t *current_line);
u32         c_string_get_whitespace_size(string_t string);
u32         c_string_get_current_line_size(string_t string);

string_t    c_string_read_line(string_t *data);

s32         c_string_read_int(string_t data);
u32         c_string_read_uint(string_t data);

// NOTE(Sleepster): Uses atof() which is why we need the arena.
float32     c_string_read_float(memory_arena_t *arena, string_t data);

// MACROS
#define STR(x)                (string_t){.data = (byte*)x, .count = c_string_length(x)}
#define C_STR(x)              ((const char *)x.data)
#define fprint_string(string) (string).count, C_STR((string))
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

    string_builder_buffer_t     *first_buffer;
    string_builder_buffer_t     *current_buffer;
    u64                          default_buffer_block_size;

    u64                          bytes_used;
    u64                          total_allocated;
}string_builder_t;

void     c_string_builder_init(string_builder_t *builder, u64 buffer_block_size);
void     c_string_builder_deinit(string_builder_t *builder);
void     c_string_builder_append_data(string_builder_t *builder, string_t data);
void     c_string_builder_append_value(string_builder_t *builder, void *value, u32 value_size);
string_t c_string_builder_get_current_string(string_builder_t *builder);
void     c_string_builder_reset(string_builder_t *builder);
void     c_string_builder_append_builder(string_builder_t *A, string_builder_t *B);
void     c_string_builder_sprintf(string_builder_t *builder, const char *string, ...);

// NOTE(Sleepster): DUMP simply writes the data out and keeps the state of the builder the same, 
//                  FLUSH writes out the data, and completely resets the state of the builder
bool8 c_string_builder_dump_to_file(file_t *file, string_builder_t *builder);
bool8 c_string_builder_flush_to_file(file_t *file, string_builder_t *builder);

#endif // C_STRING_H

