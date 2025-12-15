/* ========================================================================
   $File: r_asset_dynamic_render_font.cpp $
   $Date: December 09 2025 02:45 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_asset_manager.h>
#include <r_asset_dynamic_render_font.h>

#include <c_memory_arena.h>

asset_handle_t 
s_asset_font_get(asset_manager_t *asset_manager, string_t font_name)
{
    asset_handle_t result = {};
    asset_slot_t *valid_slot  = (asset_slot_t *)c_hash_get_value(&asset_manager->font_catalog.font_hash, font_name);
    if(valid_slot)
    {
        result.is_valid       =  true;
        result.type           =  AT_FONT;
        result.asset_slot     =  valid_slot;
        result.font           = &valid_slot->render_font;
        result.font->filename =  valid_slot->filename;
    }
    else
    {
        log_error("Font with the name: '%s' is not found in the asset data hash...\n", font_name.data);
    }

    return(result);
}

u32
s_UTF8_convert_UTF32(u8 *character)
{
    u32 result = 0;
    u8  continuation_bytes = UTF8_trailing_bytes[character[0]];
    if(continuation_bytes + 1 < 1000)
    {
        u32 utf32_char = character[0] & UTF8_initial_bytemask[continuation_bytes];
        for(s32 byte = 1;
            byte < continuation_bytes;
            ++byte)
        {
            utf32_char  = utf32_char << 6;
            utf32_char |= character[byte] & 0x3F;
        }

        if(utf32_char > UTF32_MAX_CHARACTER) utf32_char = UTF32_REPLACEMENT_CHARACTER;

        result = utf32_char;
    }
    return(result);
}

vec2_t
s_font_atlas_find_next_free_line(dynamic_render_font_page_t *page, s32 glyph_width, s32 row_height)
{
    vec2_t result;
    s32 desired_x = page->bitmap_cursor_x + glyph_width;
    if(desired_x > page->atlas_texture->bitmap.width)
    {
        page->bitmap_cursor_x  = 0;
        page->bitmap_cursor_y += page->owner_varient->max_ascender;

        result = vec2(0, page->bitmap_cursor_y);
    }
    else
    {
        result = vec2(page->bitmap_cursor_x + 1, page->bitmap_cursor_y + 1);
    }

    return(result);
}

void 
s_font_copy_glyph_data_to_page_bitmap(asset_manager_t            *asset_manager,
                                      memory_arena_t             *arena,
                                      dynamic_render_font_page_t *page,
                                      font_glyph_t               *glyph)
{
    if(!page->bitmap_valid)
    {
        texture2D_t *font_atlas = &page->atlas_handle->asset_slot->texture;
        font_atlas->bitmap.format     = BMF_RGBA32;
        font_atlas->bitmap.channels   = 4;
        font_atlas->bitmap.width      = 4096;
        font_atlas->bitmap.height     = 4096;
        font_atlas->bitmap.stride     = 32;
        font_atlas->uv_min            = vec2(0.0, 0.0);
        font_atlas->uv_max            = vec2(1.0, 1.0);
        font_atlas->has_AA            = false;
        font_atlas->filter_type       = TAAFT_NEAREST;
        font_atlas->view              = s_asset_texture_view_generate(asset_manager, null, font_atlas);
        
        page->bitmap_valid = true;
    }

    FT_Face font_face = page->owner_varient->parent->font_face;
    s32 glyph_width   = font_face->glyph->bitmap.width;
    s32 row_height    = font_face->glyph->bitmap.rows;

    glyph->offset_x   = (s16)(font_face->glyph->bitmap_left);
    glyph->offset_y   = (s16)(row_height - font_face->glyph->bitmap_top);

    glyph->advance    = (s16)(font_face->glyph->advance.x >> 6);
    glyph->ascent     = (s16)(font_face->glyph->metrics.horiBearingY >> 6);

    glyph->glyph_render_size = vec2(glyph_width, row_height);
    vec2_t bitmap_offset     = s_font_atlas_find_next_free_line(page, glyph_width, row_height);
    glyph->atlas_offset      = vec2((float32)bitmap_offset.x / (float32)page->atlas_texture->bitmap.width,
                                    (float32)bitmap_offset.y / (float32)page->atlas_texture->bitmap.height);

    glyph->glyph_size = vec2((float32)glyph_width / (float32)page->atlas_texture->bitmap.width,
                             (float32)row_height  / (float32)page->atlas_texture->bitmap.height);
    
    for(s32 row = 0;
        row < row_height;
        ++row)
    {
        for(s32 column = 0;
            column < glyph_width;
            ++column)
        {
            u8  source = font_face->glyph->bitmap.buffer[(row_height - 1 - row) * font_face->glyph->bitmap.pitch + column];
            u8 *dest   = (byte*)page->atlas_texture->bitmap.decompressed_data.data + (((u32)bitmap_offset.y + row) * page->atlas_texture->bitmap.width + ((u32)bitmap_offset.x + column)) * 4;

            dest[0] = source;
            dest[1] = source;
            dest[2] = source;
            dest[3] = source;
        }
    }
    page->bitmap_cursor_x += font_face->glyph->bitmap.width;
}

dynamic_render_font_page_t*
s_asset_font_create_new_font_page(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient)
{
    dynamic_render_font_page_t *result = c_arena_push_struct(&varient->parent->font_arena, dynamic_render_font_page_t);
    result->glyph_lookup             = c_hash_table_create_ma(&varient->parent->font_arena, 1000, sizeof(font_glyph_t));
    result->atlas_handle             = c_arena_push_struct(&varient->parent->font_arena, asset_handle_t);
    result->atlas_handle->is_valid   = true;
    result->atlas_handle->asset_slot = c_arena_push_struct(&varient->parent->font_arena, asset_slot_t);
    result->atlas_handle->asset_slot->slot_state = ASS_LOADED;
    result->atlas_handle->asset_slot->asset_type = AT_BITMAP;
    result->atlas_handle->asset_slot->texture = s_asset_texture_and_view_create(asset_manager, 
                                                                                asset_manager->font_catalog.font_allocator, 
                                                                                TEXTURE_ATLAS_SIZE, 
                                                                                TEXTURE_ATLAS_SIZE, 
                                                                                BMF_RGBA32, 
                                                                                true, 
                                                                                TAAFT_LINEAR);
    result->atlas_handle->texture = &result->atlas_handle->asset_slot->texture.view;
    result->atlas_texture = &result->atlas_handle->asset_slot->texture;
    result->owner_varient =  varient;
    result->next_page     =  null;

    return(result);
}

font_glyph_t*
s_asset_font_get_utf8_glyph(asset_manager_t               *asset_manager, 
                            dynamic_render_font_varient_t *varient, 
                            u8                            *character)
{
    font_glyph_t *result = null;

    dynamic_render_font_page_t *valid_page = 0;
    u32 UTF32_char = s_UTF8_convert_UTF32(character);
    if(UTF32_char)
    {
        string_t temp = STR((char *)&UTF32_char);
        for(dynamic_render_font_page_t *page = varient->first_page;
            page;
            page = page->next_page)
        {
            result = (font_glyph_t*)c_hash_get_value(&page->glyph_lookup, temp);
            if(result)
            {
                valid_page = page;
                break;
            }
        }

        if(!result)
        {
            dynamic_render_font_page_t *last_page = 0;
            for(dynamic_render_font_page_t *page = varient->first_page;
                page;
                page = page->next_page)
            {
                if(!page->is_full)
                {
                    valid_page = page;
                }

                if(page->next_page == null) last_page = page;
            }
            
            if(!valid_page)
            {
                dynamic_render_font_page_t *new_page = s_asset_font_create_new_font_page(asset_manager, varient);
                last_page->next_page = new_page;
            }

            FT_Error error = FT_Set_Pixel_Sizes(varient->parent->font_face, 0, varient->pixel_size);
            Assert(!error);

            u32 glyph_index = 0;
            if(UTF32_char)
            {
                glyph_index = FT_Get_Char_Index(varient->parent->font_face, UTF32_char);
                if(!glyph_index)
                {
                    log_warning("UTF32 character '%d' cannot be found...\n", UTF32_char);
                    glyph_index = varient->default_unknown_character;
                }

                error = FT_Load_Glyph(varient->parent->font_face, glyph_index, FT_LOAD_RENDER);
                Assert(!error);
            }
            else
            {
                Assert(glyph_index >= 0);
                error = FT_Load_Glyph(varient->parent->font_face, glyph_index, FT_LOAD_RENDER);
                Assert(!error);
            }

            font_glyph_t *glyph = c_arena_push_struct(&varient->parent->font_arena, font_glyph_t);

            glyph->hash_key   = c_string_make_copy(&varient->parent->font_arena, temp);
            glyph->owner_page = valid_page;
            if(asset_manager)
            {
                s_font_copy_glyph_data_to_page_bitmap(asset_manager, &varient->parent->font_arena, valid_page, glyph);
            }
            c_hash_insert_pair(&valid_page->glyph_lookup, glyph->hash_key, glyph);
            glyph->owner_page->bitmap_dirty = true;
            result = glyph;
        }
    }
    
    return(result);
}

void 
s_asset_font_load_data(memory_arena_t *arena, asset_manager_t *asset_manager, asset_handle_t handle)
{
    Assert(handle.asset_slot);
    Assert(handle.type == AT_FONT);

    asset_slot_t *asset_slot = handle.asset_slot;
    if((asset_slot->slot_state != ASS_LOADED) && (asset_slot->slot_state != ASS_QUEUED))
    {
        c_asset_manager_start_load_task(asset_manager, asset_slot, asset_manager->font_catalog.font_allocator);
        asset_slot->slot_state = ASS_QUEUED;
    }
}

inline bool8
s_asset_font_set_unknown_character(dynamic_render_font_varient_t *varient, u32 UTF32_index)
{
    bool8 result = false;
    
    u32 glyph_index = FT_Get_Char_Index(varient->parent->font_face, UTF32_index);
    if(glyph_index)
    {
        varient->default_unknown_character = glyph_index;
        result = true;
    }

    return(result);
}

dynamic_render_font_varient_t*
s_asset_font_create_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size)
{
    dynamic_render_font_varient_t *result = null;
    
    dynamic_render_font_t *font = handle.font;
    if(!font->is_initialized)
    {
        font->font_arena  = c_arena_create(MB(200));
        font->pixel_sizes = c_dynarray_create(dynamic_render_font_varient_t*);
        s_asset_font_load_data(&font->font_arena, asset_manager, handle);

        font->is_initialized = true;
    }

    if(handle.asset_slot->slot_state == ASS_LOADED)
    {
        FT_Error error = FT_New_Memory_Face(asset_manager->font_catalog.font_lib,
                                            font->loaded_data.data,
                                            font->loaded_data.count,
                                            0,
                                            &font->font_face);
        if(error == 0)
        {
            result = c_arena_push_struct(&font->font_arena, dynamic_render_font_varient_t);

            result->parent     = font;
            result->first_page = s_asset_font_create_new_font_page(asset_manager, result);

            error = FT_Set_Pixel_Sizes(font->font_face, 0, size);
            Assert(!error);

            float64 font_scale_to_pixels = font->font_face->size->metrics.y_scale / (64.0 * 65536.0);
            result->pixel_size    = size;
            result->line_spacing  =  (s64)floor(font_scale_to_pixels * font->font_face->height    + 0.5);
            result->max_ascender  =  (s64)floor(font_scale_to_pixels * font->font_face->bbox.yMax + 0.5);
            result->max_descender = -(s64)floor(font_scale_to_pixels * font->font_face->bbox.yMin + 0.5);

            // NOTE(Sleepster): Using 'm' as the baseline character
            u32 glyph_index = FT_Get_Char_Index(font->font_face, 'm');
            if(glyph_index)
            {
                FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
                result->y_center_offset = (s32)(0.5f * FT_ROUND(font->font_face->glyph->metrics.horiBearingY) + 0.5f);
            }

            glyph_index = FT_Get_Char_Index(font->font_face, 'M');
            if(glyph_index)
            {
                FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
                result->em_width = FT_ROUND(font->font_face->glyph->metrics.width);
            }

            glyph_index = FT_Get_Char_Index(font->font_face, 'T');
            if(glyph_index)
            {
                FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
                result->typical_ascender = FT_ROUND(font->font_face->glyph->metrics.horiBearingY);
            }

            glyph_index = FT_Get_Char_Index(font->font_face, 'g');
            if(glyph_index)
            {
                FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
                result->typical_descender = FT_ROUND(font->font_face->glyph->metrics.horiBearingY - font->font_face->glyph->metrics.height);
            }

            error = FT_Select_Charmap(font->font_face, FT_ENCODING_UNICODE);
            if(error)
            {
                log_error("Failure to set the charmap to unicode.... supplied font does not support Unicode...\n");
            }

            bool8 success = s_asset_font_set_unknown_character(result,        0xfffd); // Replacement character
            if(!success) success = s_asset_font_set_unknown_character(result, 0x2022); // bullet char
            if(!success) success = s_asset_font_set_unknown_character(result, (u32)'?');
            if(!success) log_warning("Unable to set the unknown character for this font...\n");

            c_dynarray_push(font->pixel_sizes, result);
        }
        else
        {
            log_error("Freetype failed too create a new memory face...\n");
        }
    }
    
    return(result);
}

dynamic_render_font_varient_t*
s_asset_font_get_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size)
{
    Assert(handle.type == AT_FONT);
    dynamic_render_font_varient_t *result = null;
    
    dynamic_render_font_t *font = handle.font;
    c_dynarray_for(font->pixel_sizes, pixel_size)
    {
        dynamic_render_font_varient_t *varient = c_dynarray_get_at_index(font->pixel_sizes, pixel_size);
        if(size == varient->pixel_size)
        {
            result = varient;
            break;
        }
    }

    if(!result)
    {
        result = s_asset_font_create_at_size(asset_manager, handle, size);
    }

    return(result);
}


