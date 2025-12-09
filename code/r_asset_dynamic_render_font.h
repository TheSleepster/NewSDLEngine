#if !defined(R_ASSET_DYNAMIC_RENDER_FONT_H)
/* ========================================================================
   $File: r_asset_dynamic_render-font.h $
   $Date: December 09 2025 01:59 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_ASSET_DYNAMIC_RENDER_FONT_H
#include <ft2build.h>
#include FT_FREETYPE_H

#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_hash_table.h>
#include <c_dynarray.h>

#include <r_asset_texture.h>

typedef struct dynamic_render_font_varient dynamic_render_font_varient_t;

/*=============================================
  =============== UNICODE STUFF ===============
  =============================================*/
inline u8 UTF8_trailing_bytes[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

inline u8 UTF8_initial_bytemask[] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
inline u8 UTF8_first_byte_mask[]  = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

inline u32 UTF8_offsets[] = {0x00000000, 0x00003080, 0x000e2080, 
                             0x03c82080, 0xfa082080, 0x82082080};

#define  UTF16_MAX_CHARACTER          0x0010FFFF
#define  UTF32_MAX_CHARACTER          0x7FFFFFFF
#define  UTF32_REPLACEMENT_CHARACTER  0x0000FFFD

inline s32
FT_ROUND(s32 X)
{
    if (X >= 0) return (X + 0x1f) >> 6;
    return -(((-X) + 0x1f) >> 6);
}

inline u8*
unicode_next_character(u8 *character)
{
    u8 character_bytes = 1 + UTF8_trailing_bytes[*character];
    return(character + character_bytes);
}
////////////////////////////////

typedef struct dynamic_render_font
{
    bool8                                    is_valid;
    bool8                                    is_initialized;
    
    FT_Face                                  font_face;
    string_t                                 filename;
    
    memory_arena_t                           font_arena;
    string_t                                 loaded_data;

    DynArray(dynamic_render_font_varient_t*) pixel_sizes;
}dynamic_render_font_t;

typedef struct dynamic_render_font_page
{
    bool8                            is_full;
    bool8                            bitmap_valid;
    bool8                            bitmap_dirty;

    hash_table_t                     glyph_lookup;
    texture2D_t                      font_atlas;

    s32                              bitmap_cursor_x;
    s32                              bitmap_cursor_y;

    dynamic_render_font_varient_t   *owner_varient;
    struct dynamic_render_font_page *next_page;
}dynamic_render_font_page_t;

typedef struct dynamic_render_font_varient
{
    s64                         pixel_size;

    s64                         line_spacing;
    s64                         max_ascender;
    s64                         max_descender;
    s32                         y_center_offset;
    s32                         typical_ascender;
    s32                         typical_descender;
    s32                         em_width;
    s32                         default_unknown_character;
    s32                         default_utf32_unknown_character;

    dynamic_render_font_t      *parent;
    dynamic_render_font_page_t *first_page;
}dynamic_render_font_varient_t;

typedef struct font_glyph
{
    vec2_t   atlas_offset;
    vec2_t   glyph_size;
    vec2_t   glyph_render_size;

    string_t hash_key;

    // RENDERING //
    s32 offset_x;
    s32 offset_y;

    s32 advance;
    s32 ascent;
    ////////////////

    dynamic_render_font_page_t *owner_page;
}font_glyph_t;

/*=============================================
  =============== FONT DATA API ===============
  =============================================*/
asset_handle_t                 s_asset_font_get(asset_manager_t *asset_manager, string_t font_name);
u32                            s_UTF8_convert_UTF32(u8 *character);
font_glyph_t*                  s_asset_font_get_utf8_glyph(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, u8 *character);
void                           s_asset_font_load_data(memory_arena_t *arena, asset_manager_t *asset_manager, asset_handle_t handle);
dynamic_render_font_varient_t* s_asset_font_create_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size);
dynamic_render_font_varient_t* s_asset_font_get_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size);

#endif // R_ASSET_DYNAMIC_RENDER_FONT_H

