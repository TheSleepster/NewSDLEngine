/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: January 06 2026 11:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb/stb_image.h>

#define DYNARRAY_IMPLEMENTATION
#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_global_context.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <c_tokenizer.h>
#include <code_generator/athena/athena.h>

#include <asset_file_packer/jfd_asset_file.h>

#if 0
#include <meta/ATHENA_GENERATED_RTTI.h>
#endif

internal_api void  asset_file_load_packages(asset_manager_t *asset_manager, asset_file_data_t *asset_file, jfd_file_header_t *header);
internal_api bool8 initialize_asset_file_contents(asset_manager_t *asset_manager, asset_file_data_t *asset_file, string_t filepath);

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(asset_manager_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

/*===============================
  ========== TEXTURES ===========
  =============================== */

bitmap_t
s_asset_bitmap_create(asset_manager_t *asset_manager, 
                      asset_slot_t    *asset_slot, 
                      u32              width,
                      u32              height, 
                      u32              channels,
                      bitmap_format_t  format)
{
    bitmap_t result  = {};
    result.width     = width;
    result.height    = height;
    result.channels  = channels;
    result.format    = format;

    u32   pixel_count = width * height * channels;
    byte *pixel_data  = c_za_push_array(asset_manager->asset_allocator, byte, pixel_count, ZA_TAG_STATIC);
    result.pixels = {
        .data  = pixel_data,
        .count = pixel_count
    };

    return(result);
}

bitmap_t
s_asset_bitmap_init(string_t pixels, s32 width, s32 height, s32 channels, u32 format)
{
    bitmap_t result = {};
    result.width    = width;
    result.height   = height;
    result.channels = channels;
    result.format   = format;
    result.pixels   = pixels;

    return(result);
}

bitmap_t 
s_asset_bitmap_create(asset_manager_t *asset_manager, s32 width, s32 height, s32 channels, u32 format)
{
    bitmap_t result;

    u32 pixel_count = width * height * channels;
    string_t pixels = {
        .data  = c_za_alloc(asset_manager->asset_allocator, pixel_count, ZA_TAG_TEXTURE),
        .count = pixel_count 
    };

    result = s_asset_bitmap_init(pixels, width, height, channels, format);
    return(result);
}

texture2D_t 
s_asset_texture_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    texture2D_t result = {};
    string_t asset_data = slot->package_entry->asset_data;

    s32 width;
    s32 height;
    s32 channels;

    byte *pixel_data  = stbi_load_from_memory(asset_data.data, asset_data.count, &width, &height, &channels, 4);
    u32   pixel_count = (u32)(width * height * channels);
    string_t pixels = {
        .data  = pixel_data,
        .count = pixel_count 
    };
    Assert(pixel_data != null);
    Assert(pixel_count > 0);

    result.ID       = name_hash;
    result.bitmap   = s_asset_bitmap_init(pixels, width, height, channels, BMF_RGBA32_SRGB);
    result.gpu_data = RHI_image_create_from_bitmap(&result.bitmap);

    return(result);
}

/*===========================================
  =============== ANIMATION2D ===============
  =========================================== */

#if 0
animation_source2D_t
s_asset_animation_source_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    animation_source2D_t result = {};
    return(result);
}
#endif

/*===============================
  =========== SHADERS ===========
  =============================== */

shader_t
s_asset_shader_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    shader_t result;
    result.ID = name_hash;
    slot->ID  = name_hash;

    asset_manager->RHI_context->backend_shader_create(&result, slot->package_entry->asset_data);

    return(result);
}

/*===============================
  ========== MATERIALS ==========
  =============================== */

// NOTE(Sleepster): We need to pass a pointer to the actual structure to actually store the file data
void
material_file_parse_item(string_t filename, void *parent_data, tokenizer_t *tokenizer, const type_info_t *parent_type_data, token_data_t name_token)
{
    // NOTE(Sleepster): Eating the colon... 
    c_tokenizer_get_next_token(tokenizer);
    token_data_t value_token = c_tokenizer_get_next_token(tokenizer);

    // NOTE(Sleepster): Handle enums stuff. 
    const type_info_member_t *struct_member = Athena::get_member(parent_type_data, name_token.string); 

    const type_info_member_t *enum_member = (type_info_member_t*)Athena::type_info(value_token.string);
    const type_info_struct_t *enum_data   = Athena::get_struct_info_from_member(enum_member);
    if(struct_member)
    {
        byte *data_ptr = (byte*)parent_data + struct_member->offset; 
        if(struct_member->type_info->type_id == TYPE_string_t)
        {
            // NOTE(Sleepster): We'll need to ignore quotes in the tokenizer 
            string_t *string_data = (string_t*)data_ptr;
            *string_data = value_token.string;
        }
        else if(enum_data)
        {
            // NOTE(Sleepster): rare instance of a do {}while(); loop being genuinely useful. We NEED this to happen at least once.
            token_data_t peek_token = c_tokenizer_peek_token(tokenizer);
            do {
                const type_info_member_t *enum_member = Athena::get_member(enum_data, value_token.string);
                u32 *value = (u32*)data_ptr;

                *value |= enum_member->offset;

                peek_token  = c_tokenizer_peek_token(tokenizer);
                if(peek_token.type == TT_Seperator)
                {
                    peek_token  = c_tokenizer_get_next_token(tokenizer);
                    value_token = c_tokenizer_get_next_token(tokenizer);
                }
            }while(peek_token.type == TT_Seperator);
        }
        else
        {
            memcpy(data_ptr, value_token.string.data, struct_member->type_info->size);
        }
    }
    else if(c_string_compare(name_token.string, STR("shader")))
    {
        log_warning("Currently, shader variables are not supported...\n");
    }
    else
    {
        log_error("Item of name: '%.*s' was found in the material file: '%s' however this item is not contained within the %s structure...\n",
                  name_token.string.count, C_STR(name_token.string), 
                  C_STR(filename),
                  parent_type_data->type_name);

        log_info("Valid members for structure of type name: '%s' are as follows:\n", parent_type_data->type_name);

        const type_info_struct_t *parent_struct = Athena::as_structure(parent_type_data);
        for(u32 member_index = 0;
            member_index < parent_struct->member_count;
            ++member_index)
        {
            const type_info_member_t *member = parent_struct->members + member_index;
            log_info("[%d]: %s...\n", member_index, member->type_info->type_name);
        }
    }
}

void 
material_file_parse_block_data(string_t filename, void *parent_data, tokenizer_t *tokenizer, const type_info_t *parent_type_data, token_data_t name_token)
{
    token_data_t token = c_tokenizer_get_next_token(tokenizer);
    while(token.type != TT_ClosingBrace)
    {
        token = c_tokenizer_get_next_token(tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                c_tokenizer_eat_lines(&gc->temporary_arena, tokenizer, 1);
            }break;
            case TT_Identifier:
            {
                // NOTE(Sleepster): Peek to make sure this isn't a nested block. 
                token_data_t peek_token = c_tokenizer_peek_token(tokenizer);
                if(peek_token.type == TT_OpeningBrace)
                {
                    // NOTE(Sleepster): Nested block. 
                    if(c_string_compare(token.string, STR("render_pipeline_state")))
                    {
                        const type_info_member_t *render_pipeline_info = Athena::get_member(parent_type_data, STR("pipeline_state"));
                        RHI_pipeline_state_t *state_data = (RHI_pipeline_state_t*)((byte*)parent_data + render_pipeline_info->offset);

                        const type_info_t *type_data = Athena::type_info<RHI_pipeline_state_t>();
                        material_file_parse_block_data(filename, state_data, tokenizer, type_data, token);
                    }
                    else
                    {
                        log_error("This nested block named '%.*s' inside of our '%s' block is not a valid subblock, the only valid one right now is 'render_pipeline_state'...\n",
                                  token.string.count, C_STR(token.string),
                                  parent_type_data->type_name);
                        return;
                    }
                }
                else
                {
                    material_file_parse_item(filename, parent_data, tokenizer, parent_type_data, token);
                }
            }break;
        }
    }
}

// TODO(Sleepster): 
// For now, we're only worrying about archetypes for the sake of simplicity. Later on we WILL need instances, 
// but for the moment we can live without them
material_data_t
s_asset_material_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    material_data_t result = {};
    material_archetype_t archetype = {};

    slot->ID  = name_hash;
    archetype.ID = name_hash;
    slot->package_entry->asset_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                                              slot->package_entry->asset_data.count,
                                                              slot->package_entry->data_offset, 
                                                              null, 
                                                              asset_manager->asset_allocator, 
                                                              ZA_TAG_STATIC);
    string_t material_data = slot->package_entry->asset_data;

    tokenizer_t tokenizer = {};
    tokenizer.data = material_data;
    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                c_tokenizer_eat_lines(&gc->temporary_arena, &tokenizer, 1);
            }break;
            case TT_Identifier:
            {
                token_data_t next_peeked = c_tokenizer_peek_token(&tokenizer);
                if(next_peeked.type == TT_Colon)
                {
                    // TODO(Sleepster): For now nothing, this requires us to know what kind of file we are in so that we can properly parse this item.
                    // If we are in a .m_arch file, then it should be material_archetype_t, if we are in a .m_inst file, it should be material_instance_t
                    // hence why we're doing nothing with it right now.
                    
                    //material_file_parse_item(&tokenizer, token);
                }
                else if(next_peeked.type == TT_OpeningBrace)
                {
                    // parse_item_structure();
                    const type_info_t *struct_info = null;
                    if(c_string_compare(token.string, STR("material_archetype"))) 
                    {
                        struct_info = Athena::type_info(STR("material_archetype_t"));
                        material_file_parse_block_data(slot->owner_asset_file.file_name, &archetype, &tokenizer, struct_info, token);
                    }
                    else if(c_string_compare(token.string, STR("base_instance")))
                    {
                        struct_info = Athena::type_info(STR("material_archetype_t"));
                        material_file_parse_block_data(slot->owner_asset_file.file_name, &archetype.base_instance, &tokenizer, struct_info, token);
                    }
                    else 
                    {
                        log_error("The intializer item is neither a material_archetype or material_instance block. These are the ONLY TWO valid items that can be in this scope... Found: '%.*s'\n",
                                  token.string.count, C_STR(token.string));
                        break;
                    }
                }
                else
                {
                    log_error("Expected to find ':' or '{' immediately after token: '%.*s'... Got: '%.*s'...\n", 
                              token.string.count, C_STR(token.string),
                              next_peeked.string.count, C_STR(next_peeked.string));
                }
            }break;
        }
    }
    // TODO(Sleepster): Is loading the shader here really okay? Is it a good idea? Only time will tell.
    //
    // Right now my assumption is that if you want to make use of this material, you should just load the shader now
    // rather than wait for way later to load it when we're rendering. Seems bad to delay it that long... But who knows
    // Maybe this is bad and that's a better idea.
    archetype.ID              = c_fnv_hash_value(archetype.name.data, archetype.name.count);
    archetype.shader_handle   = s_asset_manager_acquire_asset_handle(asset_manager, archetype.shader_binary_name);

    result.material_type      = SMT_Archetype;
    result.archetype          = archetype;

    return(result);
}

/*===========================================
  ================ FONT DATA  ===============
  ===========================================*/
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

void
s_UTF32_convert_to_UTF8(string_t *buffer, u32 character)
{
    if(buffer->count >= 4)
    {
        const local_persist u32 UNICODE_MAX_LEGAL_UTF32 = 0x0010FFFFUL;

        u32 count = 0;
        if(character < 0x80)                          count = 1;
        else if(character < 0x800)                    count = 2;
        else if(character < 0x10000)                  count = 3;
        else if(character <= UNICODE_MAX_LEGAL_UTF32) count = 4;
        else 
        {
            count = 3;
            character = UTF32_REPLACEMENT_CHARACTER;
        }

        const u32 UTF_byte_mask = 0xBF;
        const u32 UTF_byte_mark = 0x80; 

        buffer->count = count;
        switch(count)
        {
            case 4:
            {
                buffer->data[3] = ((character | UTF_byte_mark) & UTF_byte_mask);
                character = character >> 6;
            };
            case 3:
            {
                buffer->data[2] = ((character | UTF_byte_mark) & UTF_byte_mask);
                character = character >> 6;
            };
            case 2:
            {
                buffer->data[1] = ((character | UTF_byte_mark) & UTF_byte_mask);
                character = character >> 6;
            };
            case 1:
            {
                buffer->data[0] = (character | UTF8_first_byte_mark[count]);
                character = character >> 6;
            }break;
        }
    }
    else
    {
        log_error("Buffer is too small for a UTF32 -> UTF8 conversion. We expect at least 4 bytes, instead got: '%d'...\n",
                  buffer->count);
    }
}

bool8
s_asset_font_set_unknown_character(dynamic_render_font_varient_t *varient, u32 UTF32_index)
{
    bool8 result = false;
    
    u32 glyph_index = FT_Get_Char_Index(varient->parent_font->font_face, UTF32_index);
    if(glyph_index)
    {
        varient->default_unknown_character = glyph_index;
        result = true;
    }

    return(result);
}

dynamic_render_font_page_t*
s_asset_font_create_new_page(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, memory_arena_t *arena)
{
    dynamic_render_font_page_t *new_page = null; 
    new_page = c_arena_push_struct(arena, dynamic_render_font_page_t);
    Assert(new_page);

    new_page->parent_font = varient->parent_font;
    new_page->font_atlas  = s_texture_atlas_create(asset_manager, 4096, 4, BMF_RGB24_UNORM, 0);
    new_page->varient     = varient;
    c_hash_table_init(&new_page->glyphs, 
                      2087, 
                      arena, 
                     &asset_manager_hash_arena_allocate, 
                      null);

    return(new_page);
}

dynamic_render_font_varient_t*
s_asset_font_create_new_varient(asset_manager_t *asset_manager, dynamic_render_font_t *font, u32 font_size)
{
    dynamic_render_font_varient_t *result = c_arena_push_struct(&font->font_arena, dynamic_render_font_varient_t);
    result->font_size   = font_size;
    result->parent_font = font;
    result->first_page  = s_asset_font_create_new_page(asset_manager, result, &font->font_arena);

    FT_Error error = FT_Set_Pixel_Sizes(font->font_face, 0, font_size);
    Assert(error == 0);

    float64 font_scale_to_pixels = font->font_face->size->metrics.y_scale / (64.0 * 65536.0);
    result->size_to_pixels = font_scale_to_pixels;
    result->line_spacing   =  (s64)floor(font_scale_to_pixels * font->font_face->height    + 0.5);
    result->max_ascender   =  (s64)floor(font_scale_to_pixels * font->font_face->bbox.yMax + 0.5);
    result->max_descender  = -(s64)floor(font_scale_to_pixels * font->font_face->bbox.yMin + 0.5);

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

    c_dynarray_add(&font->varients, &result);

    return(result);
}

dynamic_render_font_t 
s_asset_font_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    dynamic_render_font_t result = {};
    slot->ID  = name_hash;
    if(!FT_New_Memory_Face(asset_manager->freetype_handle, 
                          slot->package_entry->asset_data.data, 
                          slot->package_entry->asset_data.count, 
                          0, 
                         &result.font_face))
    {
        result.font_arena = c_arena_create(MB(100));
    }
    else
    { 
        log_error("Failure to load font: '%.*s'...\n",
                  slot->name.count, slot->name.data);
    }

    return(result);
}

dynamic_render_font_varient_t*
s_asset_font_acquire_font_at_size(asset_manager_t *asset_manager, asset_handle_t *font_handle, u32 font_size)
{
    Assert(font_handle->slot->type == AT_Font);
    dynamic_render_font_varient_t *result = null;

    if(*font_handle->is_valid)
    {
        dynamic_render_font_t *font = font_handle->dynamic_render_font;
        for(dynamic_render_font_varient_t *varient: font->varients)
        {
            if(varient->font_size == font_size)
            {
                result = varient;
                break;
            }
        }

        if(result == null)
        {
            result = s_asset_font_create_new_varient(asset_manager, font, font_size);
        }
    }

    return(result);
}

// TODO(Sleepster): This will *NOT* correctly handle UTF8 characters as simply advancing by one doesn't work
// We need to advance by how ever many bytes the UTF character is.
vec2_t 
s_asset_font_get_string_size(asset_manager_t *asset_manager, 
                             string_t         string, 
                             asset_handle_t  *font_handle, 
                             u32              pixel_size, 
                             float32         *max_descender_out)
{
    Assert(string.count > 0);
    vec2_t result = vec2_zero();

    dynamic_render_font_varient_t *varient = s_asset_font_acquire_font_at_size(asset_manager, 
                                                                               font_handle, 
                                                                               pixel_size);
    u32 tallest_glyph = 0;
    u32 total_width   = 0;
    for(u32 string_index = 0;
        string_index < string.count;
        ++string_index)
    {
        u8 *character = string.data + string_index;
        glyph_metric_t *glyph = s_asset_font_fetch_glyph(asset_manager, varient, character);
        if(glyph->height > (s32)tallest_glyph)
        {
            tallest_glyph = glyph->height + glyph->offset_y;
        }

        total_width += glyph->advance;
    }

    if(max_descender_out)
    {
        *max_descender_out = varient->max_descender;
    }

    result = vec2(total_width, tallest_glyph);
    return(result);
}

glyph_metric_t*
s_asset_font_fetch_glyph(asset_manager_t               *asset_manager,
                         dynamic_render_font_varient_t *varient,
                         byte                          *codepoint)
{
    glyph_metric_t *result = null;
    if(varient)
    {
        u32 codepoint_UTF32 = s_UTF8_convert_UTF32(codepoint);
        string_t codepoint_data = {
            .data  = (byte*)&codepoint_UTF32,
            .count = sizeof(u32),
        };

        dynamic_render_font_page_t *last_page = null;
        dynamic_render_font_page_t *our_page  = null;

        for(dynamic_render_font_page_t *current_page = varient->first_page;
            current_page;
            current_page = current_page->next_page)
        {
            glyph_metric_t *found = c_hash_table_get_value(&current_page->glyphs, codepoint_data);
            if(found)
            {
                result   = found;
                our_page = current_page;

                break;
            }

            last_page = current_page;
        }

        if(result == null)
        {
            Assert(last_page);
            if(!last_page->is_full)
            {
                glyph_metric_t *new_glyph = c_arena_push_struct(&varient->parent_font->font_arena, glyph_metric_t);
                c_hash_table_insert_pair(&last_page->glyphs, codepoint_data, new_glyph);

                our_page = last_page;
            }
            else
            {
                last_page->next_page = s_asset_font_create_new_page(asset_manager, 
                                                                    varient, 
                                                                    &varient->parent_font->font_arena);
                our_page = last_page->next_page;
            }
        }
        Assert(our_page);
        Assert(our_page->is_full == false);

        result = c_hash_table_get_value(&our_page->glyphs, codepoint_data);
        if(result->is_fetched == false)
        {
            result->is_fetched = true;

            dynamic_render_font_t *parent = varient->parent_font;

            temporary_glyph_t *temp_glyph = our_page->temporary_glyphs + our_page->temporary_glyph_count++;
            temp_glyph->utf32_codepoint = codepoint_UTF32;
            temp_glyph->cursor_x = our_page->atlas_cursor_x;
            temp_glyph->cursor_y = our_page->atlas_cursor_y;
            temp_glyph->metrics  = result;

            u32 glyph_index = FT_Get_Char_Index(parent->font_face, codepoint_UTF32);
            FT_Load_Glyph(parent->font_face, glyph_index, FT_LOAD_DEFAULT);

            FT_Fixed advance = 0;
            FT_Get_Advance(parent->font_face, glyph_index, FT_LOAD_NO_SCALE, &advance);

            FT_Glyph_Metrics *ft_metrics = &parent->font_face->glyph->metrics;

            // NOTE(Sleepster): 
            // Shift right 6 to convert these values to pixels. 
            // Freetype stores them weirdly and uses 26.6 fixed-point pixel metrics.
            temp_glyph->glyph_width  = (u32)(ft_metrics->width  >> 6);
            temp_glyph->glyph_height = (u32)(ft_metrics->height >> 6);
            if(temp_glyph->glyph_height > our_page->tallest_y)
            {
                our_page->tallest_y = temp_glyph->glyph_height;
            }

            // NOTE(Sleepster): This is for multithreaded atlas writing later. 
            const float32 padding = 2;
            our_page->atlas_cursor_x += temp_glyph->glyph_width + padding;
            if(our_page->atlas_cursor_x >= 4096)
            {
                our_page->atlas_cursor_x  = 0;
                our_page->atlas_cursor_y += our_page->tallest_y;
            }

            if(our_page->atlas_cursor_y + our_page->tallest_y >= 4096)
            {
                our_page->is_full = true;
            }

            if(!our_page->is_dirty)
            {
                font_manager_t *font_manager = &asset_manager->font_manager;

                our_page->is_dirty = true;
                font_manager->pages_to_update[font_manager->pages_queued++] = our_page;
            }

            // NOTE(Sleepster): This glyph should not be loaded yet... If it is? Weird...
            Assert(result->is_valid == false);
        }
    }

    return(result);
}

void
s_asset_font_load_glyph(dynamic_render_font_varient_t *varient, 
                        dynamic_render_font_page_t    *page, 
                        temporary_glyph_t             *temp_glyph)
{
    glyph_metric_t *metrics = temp_glyph->metrics;
    FT_Face font_face = page->parent_font->font_face;

    FT_Error error = FT_Set_Pixel_Sizes(font_face, 0, varient->font_size);
    Assert(!error);

    u32 glyph_index = FT_Get_Char_Index(font_face, temp_glyph->utf32_codepoint);
    if(!glyph_index)
    {
        log_warning("UTF32 character '%d' cannot be found...\n", temp_glyph->utf32_codepoint);
        glyph_index = varient->default_unknown_character;
    }

    error = FT_Load_Glyph(font_face, glyph_index, FT_LOAD_RENDER);
    assert(!error);

    s32 glyph_width   = font_face->glyph->bitmap.width;
    s32 row_height    = font_face->glyph->bitmap.rows;

    metrics->offset_x   = (s16)(font_face->glyph->bitmap_left);
    metrics->offset_y   = (s16)(row_height - font_face->glyph->bitmap_top);
    metrics->advance    = (s16)(font_face->glyph->advance.x >> 6);
    metrics->ascent     = (s16)(font_face->glyph->metrics.horiBearingY >> 6);
    metrics->width      = glyph_width;
    metrics->height     = row_height;

    metrics->atlas_offset = vec2((float32)temp_glyph->cursor_x / (float32)4096,
                                 (float32)temp_glyph->cursor_y / (float32)4096);

    metrics->atlas_size = vec2((float32)glyph_width / (float32)4096,
                               (float32)row_height  / (float32)4096);

    bitmap_t *atlas_bitmap = page->font_atlas->bitmap_data;
    for(s32 row = 0;
        row < row_height;
        ++row)
    {
        for(s32 column = 0;
            column < glyph_width;
            ++column)
        {
            u8  source = font_face->glyph->bitmap.buffer[row * font_face->glyph->bitmap.pitch + column];
            u8 *dest   = (u8*)atlas_bitmap->pixels.data + ((temp_glyph->cursor_y + row) * 4096 + (temp_glyph->cursor_x + column)) * 4;

            dest[0] = source;
            dest[1] = source;
            dest[2] = source;
            dest[3] = source;
        }
    }

    metrics->owner_atlas = page->font_atlas;
    metrics->is_valid    = true;
}

/*===============================
  ========= ASSET DATA ==========
  =============================== */

// TODO(Sleepster): 
// Dude, this asset slot + handle setup is ABSOLUTE GARBAGE. Just look at something like
//
// vulkan_shader_data_t *shader = &current_group->material->material->archetype->shader.slot->shader.shader_data;
//
// Like what the fuck???
void
s_asset_manager_load_asset_data(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    Assert(slot->slot_state == ASLS_LoadQueued);
    string_t asset_data = {};
    

    // TODO(Sleepster): Only load from the asset packages in release 
    asset_file_data_t *asset_file = asset_manager->asset_files + slot->owner_asset_file_index;
    file_data_t package_file_info = c_file_get_file_system_info(asset_file->file_info.filepath);
    file_data_t asset_file_info   = c_file_get_file_system_info(slot->package_entry->fullpath);
    if(package_file_info.last_modtime > asset_file_info.last_modtime)
    {
        asset_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                             slot->package_entry->asset_data.count,
                                             slot->package_entry->data_offset, 
                                             null, 
                                             asset_manager->asset_allocator, 
                                             ZA_TAG_STATIC);
        log_info("Asset data for asset '%.*s' loaded from the asset package file...\n", fprint_string(slot->package_entry->filename));

        asset_file_data_t *source_asset_file = asset_manager->asset_files + slot->owner_asset_file_index;
        c_dynarray_add(&source_asset_file->loaded_assets, &slot);
    }
    else
    {
        asset_data = c_file_read_entirety(slot->package_entry->fullpath, 
                                          null, 
                                          asset_manager->asset_allocator, 
                                          ZA_TAG_STATIC);
        log_info("Asset data for asset '%.*s' loaded from it's local filepath...\n", fprint_string(slot->package_entry->filename));
    }

    Assert(asset_data.data != null && asset_data.count > 0);
     slot->package_entry->asset_data = asset_data;
    switch(slot->type)
    {
        case AT_Bitmap:
        {
            slot->texture = s_asset_texture_create(asset_manager, slot, name_hash);
            log_info("Loading texture data for bitmap: '%s'...\n", C_STR(slot->name));
        }break;
        case AT_Shader:
        {
            slot->shader = s_asset_shader_create(asset_manager, slot, name_hash);
            log_info("Loading shader data for: '%s'...\n", C_STR(slot->name));
        }break;
        case AT_Material:
        {
            slot->material.material_type = SMT_Archetype; 
            slot->material = s_asset_material_create(asset_manager, slot, name_hash);
            log_info("Loading material data for: '%s'...\n", C_STR(slot->name));
        }break;
        case AT_Font:
        {
            slot->dynamic_render_font = s_asset_font_create(asset_manager, slot, name_hash);
            log_info("Loading font data for: '%s'...\n", C_STR(slot->name));
        }break;
        case AT_Sound:
        {
            log_warning("Not loading sound... not currently supported...\n");
            //handle->sound = &slot->sound;
        }break;
    }

    slot->slot_state           = ASLS_Loaded;
    slot->is_valid_for_handles = true;
    slot->asset_manager        = asset_manager;
    AtomicIncrement32(&slot->package_generation);
}

// ===============================
// ========== ASSET MANAGER ======
// ===============================

internal_api asset_handle_t 
asset_catalog_load_default_asset(asset_catalog_t *catalog)
{
    asset_handle_t result = {};

    string_t default_asset_name;
    switch(catalog->catalog_type)
    {
        case AT_Bitmap:
        {
            default_asset_name = STR("null_sprite");
        }break;
        case AT_Shader:
        {
            default_asset_name = STR("basic_triangle");
        }break;
        case AT_Material:
        {
            default_asset_name = STR("test_material_archetype");
        }break;
        case AT_Font:
        {
            default_asset_name = STR("LiberationMono_Regular");
        }break;
    }
    Assert(catalog->asset_manager);

    u64 entry_count = catalog->asset_manager->asset_name_to_file.header.max_entries;
    u64 hash_value  = c_hash_table_value_from_key(default_asset_name.data, 
                                                  default_asset_name.count, 
                                                  entry_count);

    result.slot = s_asset_manager_get_asset_slot(catalog, default_asset_name);
    result.slot->catalog = catalog;
    result.slot->slot_state = ASLS_LoadQueued;

    s_asset_manager_load_asset_data(catalog->asset_manager, result.slot, hash_value);
    s_asset_manager_set_handle_asset_data_pointer(&result, result.slot);

    return(result);
}

void
s_asset_manager_init(asset_manager_t *asset_manager)
{
    Assert(asset_manager->is_initialized == false);
    stbi_set_flip_vertically_on_load(0);

    FT_Init_FreeType(&asset_manager->freetype_handle);
    Expect(asset_manager->freetype_handle != null, "Failure to initialize freetype...\n");

    asset_manager->manager_arena   = c_arena_create(MB(100));
    asset_manager->asset_allocator = c_za_create(GB(1));

    c_hash_table_init(&asset_manager->asset_name_to_file, 
                       ASSET_CATALOG_MAX_LOOKUPS, 
                      &asset_manager->manager_arena, 
                       asset_manager_hash_arena_allocate,
                       null);

    // NOTE(Sleepster): Initializing all entries to -1 
    memset(asset_manager->asset_name_to_file.data, -1, sizeof(s32) * ASSET_CATALOG_MAX_LOOKUPS);

    asset_manager->texture_catalog  = asset_manager->asset_catalogs + AT_Bitmap;
    asset_manager->shader_catalog   = asset_manager->asset_catalogs + AT_Shader;
    asset_manager->material_catalog = asset_manager->asset_catalogs + AT_Material;
    asset_manager->font_catalog     = asset_manager->asset_catalogs + AT_Font;
    asset_manager->sound_catalog    = asset_manager->asset_catalogs + AT_Sound;

    for(u32 catalog_index = 1;
        catalog_index < AT_Count;
        ++catalog_index)
    {
        asset_catalog_t *catalog = asset_manager->asset_catalogs + catalog_index;
        catalog->asset_manager = asset_manager;
        c_hash_table_init(&catalog->asset_lookup, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_manager->manager_arena, 
                           asset_manager_hash_arena_allocate,
                           null);

        catalog->catalog_type  = (asset_type_t)catalog_index;

        Assert(catalog->catalog_type < AT_Count);
        Assert(catalog->catalog_type > AT_Invalid);
    }
    s_asset_manager_load_asset_file(asset_manager, STR("asset_data.jfd"));

    for(u32 catalog_index = 1;
        catalog_index < AT_Count;
        ++catalog_index)
    {
        asset_catalog_t *catalog = asset_manager->asset_catalogs + catalog_index;
        if(catalog->catalog_type != AT_Sound && 
           catalog->catalog_type != AT_Material)
        {
            catalog->default_asset = asset_catalog_load_default_asset(catalog);
        }
    }

    asset_manager->is_initialized = true;
}

void
s_asset_manager_update(asset_manager_t *asset_manager)
{
    // NOTE(Sleepster): Reload asset files 
    for(u32 asset_file_index = 0;
        asset_file_index < asset_manager->loaded_file_count;
        ++asset_file_index)
    {
        asset_file_data_t *asset_file = asset_manager->asset_files + asset_file_index; 
        if(asset_file->load_status == ASLS_ShouldReload)
        {
            for(asset_slot_t *loaded_asset: asset_file->loaded_assets)
            {
                bool8 found = false;
                for(u32 load_queue_index = 0;
                    load_queue_index < asset_manager->load_queue_size;
                    ++load_queue_index)
                {
                    asset_slot_t *load_queue_asset = asset_manager->asset_load_queue[load_queue_index];
                    if(load_queue_asset == loaded_asset)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    s_asset_manager_queue_asset_load(asset_manager, loaded_asset);
                }
            }

            string_t filepath = c_string_make_copy(&gc->temporary_arena, asset_file->file_info.filepath);

            c_dynarray_reset(&asset_file->loaded_assets);
            c_arena_reset(&asset_file->init_arena);
            c_file_close(&asset_file->file_info);

            initialize_asset_file_contents(asset_manager, asset_file, filepath);
        }
    }

    // NOTE(Sleepster): Update texture atlases 
    for(u32 atlas_index = 0;
        atlas_index < asset_manager->atlas_registry.current_atlas_count;
        ++atlas_index)
    {
        texture_atlas_t *atlas = asset_manager->atlas_registry.atlases + atlas_index;
        for(u32 texture_index = 0;
            texture_index < atlas->packed_subtexture_count;
            ++texture_index)
        {
            subtexture_data_t *data = atlas->packed_subtextures + texture_index;

            volatile u32 recorded_package_version = AtomicLoad32(&data->packed_asset.slot->package_generation);
            volatile u32 atlased_generation       = AtomicLoad32(&data->atlased_generation);
            if(recorded_package_version != atlased_generation)
            {
                s_texture_atlas_add_texture(atlas, &data->packed_asset);
            }
        }

        if(atlas->merge_counter > 0)
        {
            s_texture_atlas_pack_added_textures(asset_manager, atlas);
        }
    }

    // NOTE(Sleepster): Process items in the load queue 
    // TODO(Sleepster): Unload queue...
    for(u32 queued_load_index = 0;
        queued_load_index < asset_manager->load_queue_size;
        ++queued_load_index)
    {
        asset_slot_t *slot_to_load = asset_manager->asset_load_queue[queued_load_index];
        asset_catalog_t *catalog = asset_manager->asset_catalogs + slot_to_load->type;

        s_asset_manager_load_asset_data(asset_manager, slot_to_load, slot_to_load->ID);
        c_dynarray_add(&catalog->loaded_assets, &slot_to_load);

        slot_to_load->slot_state = ASLS_Loaded;
        slot_to_load = null;
    }

    ZeroMemory(asset_manager->asset_load_queue, sizeof(asset_slot_t*) * asset_manager->load_queue_size);
    asset_manager->load_queue_size = 0;

    // NOTE(Sleepster): Add temporary_glyphs into the proper atlas   
    font_manager_t *font_manager = &asset_manager->font_manager;
    for(u32 page_index = 0;
        page_index < font_manager->pages_queued;
        ++page_index)
    {
        dynamic_render_font_page_t *page = font_manager->pages_to_update[page_index];
        if(page->is_dirty)
        {
            for(u32 glyph_index = 0;
                glyph_index < page->temporary_glyph_count;
                ++glyph_index)
            {
                temporary_glyph_t *glyph = page->temporary_glyphs + glyph_index;
                s_asset_font_load_glyph(page->varient, page, glyph);

                ++page->loaded_glyph_count;
            }

            if(page->font_atlas->texture.gpu_data.ID != 0)
            {
                RHI_image_update_data(asset_manager->RHI_context, &page->font_atlas->texture.gpu_data);
            }
            else
            {
                RHI_sampler_create_info_t sampler_info = {
                    .filtering                  = RHI_IMAGE_FILTER_TYPE_LINEAR,
                    .anisotropy_enabled         = true,
                    .max_anisotropy             = 4,
                    .wrapu                      = RHI_IMAGE_WRAPPING_CLAMP_TO_EDGE,
                    .wrapv                      = RHI_IMAGE_WRAPPING_CLAMP_TO_EDGE,
                    .compare_ops_enabled        = false,
                    .use_normalized_coordinates = true,
                };

                RHI_image_create_info_t info = {
                    .data         = page->font_atlas->texture.bitmap.pixels,
                    .width        = 4096,
                    .height       = 4096,
                    .format       = BMF_RGBA32_UNORM,
                    .usage        = RHI_IMAGE_USAGE_SHADER_SAMPLED_IMAGE,
                    .sampler_info = sampler_info
                };
                asset_manager->RHI_context->backend_image_create(&info, &page->font_atlas->texture.gpu_data);
            }
            page->is_dirty = false;
            page->temporary_glyph_count = 0;
        }
    }
    font_manager->pages_queued = 0;
}

internal_api void
asset_file_load_packages(asset_manager_t *asset_manager, asset_file_data_t *asset_file, jfd_file_header_t *header)
{
    file_t *file_handle = &asset_file->file_info;
    u64 current_file_offset = file_handle->current_read_offset;
    for(u32 entry_index = 0;
        entry_index < header->entry_count;
        ++entry_index)
    {
        u64 data_offset = current_file_offset + sizeof(jfd_package_chunk_header_t);
        jfd_package_entry_t *entry = asset_file->package_entries + entry_index;
        entry->entry_header = (jfd_package_chunk_header_t*)(c_file_read_from_offset(file_handle, 
                                                                                    sizeof(jfd_package_chunk_header_t), 
                                                                                    current_file_offset, 
                                                                                    &asset_file->init_arena).data);
        Assert(entry->entry_header->magic_value == ASSET_FILE_CHUNK_MAGIC);
        entry->filename = c_file_read_from_offset(file_handle, 
                                                  entry->entry_header->filename_size, 
                                                  data_offset, 
                                                  &asset_file->init_arena);
        entry->fullpath = c_file_read_from_offset(file_handle,
                                                  entry->entry_header->fullpath_size,
                                                  data_offset + entry->filename.count,
                                                  &asset_file->init_arena);

        entry->asset_data.count = entry->entry_header->entry_data_size;
        entry->data_offset      = data_offset + (entry->filename.count + entry->fullpath.count);
        entry->modtime          = entry->entry_header->modtime;

        current_file_offset    += entry->entry_header->total_entry_size;

        c_hash_table_insert_pair(&asset_file->entry_hash, entry->filename, (s32)entry_index);
        c_hash_table_insert_pair(&asset_manager->asset_name_to_file, entry->filename, (s32)asset_file->ID);
        u64 hash_value = c_hash_table_value_from_key(entry->filename.data, 
                                                     entry->filename.count, 
                                                     asset_manager->asset_name_to_file.header.max_entries);

        log_debug("Inserting asset with name: '%s' with a name length of: '%d' into the name_to_file hash with file_index: '%d' hash_value: '%llu'...\n", 
                  C_STR(entry->filename), entry->filename.count, asset_file->ID, hash_value);

        asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
        asset_slot_t    *slot    = c_hash_table_get_value_ptr(&catalog->asset_lookup, entry->filename);

        Assert(entry->entry_header->asset_type == catalog->catalog_type);
        Assert(slot);

        slot->slot_state         = ASLS_Unloaded;
        slot->type               = (asset_type_t)entry->entry_header->asset_type;
        slot->name               = entry->filename;
        slot->package_entry      = entry;
        slot->owner_asset_file   = asset_file->file_info;
    }
    asset_manager->loaded_file_count += 1;
}

internal_api bool8 
initialize_asset_file_contents(asset_manager_t *asset_manager, asset_file_data_t *asset_file, string_t filepath)
{
    bool8 result = true;

    asset_file->file_info = c_file_open(filepath, false);
    if(asset_file->file_info.handle != INVALID_FILE_HANDLE)
    {
        file_t *file_handle = &asset_file->file_info;

        if(asset_file->init_arena.is_initialized == false)
        {
            asset_file->init_arena = c_arena_create(MB(500));
        }

        asset_file->is_initialized = true;
        asset_file->ID             = asset_manager->loaded_file_count;

        jfd_file_header_t *header = (jfd_file_header_t*)(c_file_read(file_handle, sizeof(jfd_file_header_t), &asset_file->init_arena).data);
        Assert(header->magic_value == ASSET_FILE_HEADER_MAGIC);

        asset_file->header          = header;
        asset_file->package_entries = c_arena_push_array(&asset_file->init_arena, jfd_package_entry_t, header->entry_count);
        c_hash_table_init(&asset_file->entry_hash, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_file->init_arena, 
                           asset_manager_hash_arena_allocate,
                           null);

        // NOTE(Sleepster): Load the packages 
        asset_file_load_packages(asset_manager, asset_file, header);
    }
    else
    {
        log_error("Failure to open file: type'%s'... invalid handle...\n", C_STR(filepath));
        result = false;
    }

    return(result);
}

bool8
s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath)
{
    Assert(asset_manager->loaded_file_count + 1 <= ASSET_MANAGER_MAX_ASSET_FILES);
    
    bool8 result = true; 
    asset_file_data_t *asset_file = null;
    for(u32 file_index = 0;
        file_index < ASSET_MANAGER_MAX_ASSET_FILES;
        ++file_index)
    {
        asset_file_data_t *found = asset_manager->asset_files + file_index;
        if(found->is_initialized == false)
        {
            asset_file = found;
            break;
        }
    } 
    Assert(asset_file);
    Assert(!asset_file->is_initialized);

    result = initialize_asset_file_contents(asset_manager, asset_file, filepath);

    return(result);
}

void
s_asset_manager_signal_asset_file_reload(asset_manager_t *asset_manager, string_t filename)
{
    // TODO(Sleepster): Hash table? 
    asset_file_data_t *our_file = null;
    for(u32 file_index = 0;
        file_index < ASSET_MANAGER_MAX_ASSET_FILES;
        ++file_index)
    {
        asset_file_data_t *found = asset_manager->asset_files + file_index;
        if(c_string_compare(found->file_info.file_name, filename))
        {
            our_file = found;
            break;
        }
    }

    if(our_file)
    {
        AtomicIncrement32(&our_file->current_package_generation);
        our_file->load_status = ASLS_ShouldReload;
    }
    else
    {
        log_warning("Could not find asset file: '%.*s'... therefore we cannot reload it.");
    }
}

internal_api asset_slot_t *
s_asset_manager_get_asset_slot(asset_catalog_t *catalog, string_t name)
{
    asset_slot *result = null;
    result = c_hash_table_get_value_ptr(&catalog->asset_lookup, name);
    if(result == null)
    {
        log_error("Failure to fetch asset '%s' from this catalog...\n", C_STR(name));
    }

    return(result);
}

true_inline void
s_asset_manager_set_handle_asset_data_pointer(asset_handle_t *handle, asset_slot_t *slot)
{
    switch(slot->type)
    {
        case AT_Bitmap:   {handle->texture             = &slot->texture;            }break;
        case AT_Shader:   {handle->shader              = &slot->shader;             }break;
        case AT_Material: {handle->material_info       = &slot->material;           }break;
        case AT_Font:     {handle->dynamic_render_font = &slot->dynamic_render_font;}break;
        //case AT_Sound:    {}break;
    }
}

void
s_asset_manager_queue_asset_load(asset_manager_t *asset_manager, asset_slot_t *slot)
{
    if(slot->slot_state == ASLS_Loaded)
    {
        s32 index    = -1;
        s32 iterator =  0;
        for(const asset_slot_t *loaded_asset: slot->catalog->loaded_assets)
        {
            if(loaded_asset == slot)
            {
                index = iterator;
                break;
            }

            ++iterator;
        }

        if(index > -1)
        {
            c_dynarray_remove(&slot->catalog->loaded_assets, (u32)index);
        }

        slot->is_valid_for_handles = false;

        // NOTE(Sleepster): Free the data. 
        c_za_free(asset_manager->asset_allocator, slot->package_entry->asset_data.data);
        if(slot->type == AT_Bitmap && slot->texture.bitmap.pixels.data)
        {
            stbi_image_free(slot->texture.bitmap.pixels.data);
        }

        log_info("Reload triggered for asset of name: '%.*s'...\n", fprint_string(slot->name));
        AtomicIncrement32(&slot->package_generation);
    }

    bool8 found = false;
    for(u32 asset_index = 0;
        asset_index < asset_manager->load_queue_size;
        ++asset_index)
    {
        asset_slot_t *found_slot = asset_manager->asset_load_queue[asset_index];
        if(found_slot->ID == slot->ID)
        {
            found = true;
        }
    }

    if(!found)
    {
        asset_manager->asset_load_queue[asset_manager->load_queue_size++] = slot;
        slot->slot_state = ASLS_LoadQueued;
    }
}

asset_handle_t
s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name)
{
    asset_handle_t result;

    u64 hash_value = c_hash_table_value_from_key(name.data, name.count, asset_manager->asset_name_to_file.header.max_entries);
    log_info("hash index for: '%.*s' is '%llu'...\n", fprint_string(name), hash_value);
    s32 file_index = c_hash_table_get_value(&asset_manager->asset_name_to_file, name);
    if(file_index != -1)
    {
        asset_file_data_t *asset_file = asset_manager->asset_files + file_index;
        s32 asset_entry_index = c_hash_table_get_value(&asset_file->entry_hash, name);
        // NOTE(Sleepster): This just SHOULD NOT be possible... 
        //                  An assert here would imply that we found the file inside of a package, but cannot locate it.
        //                  Which in any case is a bug and should be fixed immediately.
        Assert(asset_entry_index != -1);

        jfd_package_entry_t *entry = asset_file->package_entries + asset_entry_index;
        Assert(entry->entry_header->asset_type != AT_Invalid && entry->entry_header->asset_type != AT_Count);

        asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
        Assert(catalog);

        asset_slot_t *slot = s_asset_manager_get_asset_slot(catalog, name);

        result.is_valid = &slot->is_valid_for_handles;
        result.slot     = slot;
        result.slot->ID = hash_value;
        result.slot->catalog = asset_manager->asset_catalogs + result.slot->type;
        if(result.slot->slot_state == ASLS_Unloaded)
        {
            // NOTE(Sleepster): Otherwise, load it. 
            s_asset_manager_queue_asset_load(asset_manager, slot);
            slot->owner_asset_file_index = file_index;
        }

        s_asset_manager_set_handle_asset_data_pointer(&result, slot);
    }
    else
    {
        log_warning("Asset by name of: '%.*s' cannot be found in the asset file database...\n",
                    name.count, C_STR(name));
    }

    return(result);
}

// ===============================
// ======= TEXTURE ATLASES =======
// ===============================

texture_atlas_t*
s_texture_atlas_create(asset_manager_t *asset_manager, 
                       u32              size, 
                       u32              channel_count, 
                       u32              format, 
                       u32              initial_subtexture_count)
{
    texture_atlas_registry_t *registry = &asset_manager->atlas_registry;
    texture_atlas_t *atlas = null;
    for(u32 atlas_index = 0;
        atlas_index < ArrayCount(registry->atlases);
        ++atlas_index)
    {
        texture_atlas_t *found = registry->atlases + atlas_index;
        if(found->is_valid == false)
        {
            atlas = found;
            break;
        }
    }
    // TODO(Sleepster): Might wanna do something about this assert... I just don't care right now.
    Assert(atlas);
    Assert(atlas->is_valid == false)

    registry->current_atlas_count += 1;

    atlas->texture.bitmap = s_asset_bitmap_create(asset_manager, size, size, channel_count, format);
    atlas->bitmap_data    = &atlas->texture.bitmap;
    atlas->atlas_size     = size;

    c_dynarray_reserve(&atlas->textures_to_merge, initial_subtexture_count);
    c_dynarray_reserve(&atlas->packed_subtextures, initial_subtexture_count);

    atlas->ID       = registry->current_atlas_count - 1;
    atlas->is_valid = true;

    return(atlas);
}

void
s_texture_atlas_add_texture(texture_atlas_t *atlas, asset_handle_t *texture_handle)
{
    Assert(texture_handle);
    Assert(texture_handle->slot->type == AT_Bitmap);

    c_dynarray_add(&atlas->textures_to_merge, &texture_handle);
    atlas->merge_counter += 1;
}

void
s_texture_atlas_pack_added_textures(asset_manager_t *asset_manager, texture_atlas_t *atlas)
{
    Assert(atlas->is_valid);
    if(atlas->merge_counter > 0)
    {
        u32 atlas_width       = atlas->bitmap_data->width;
        u32 atlas_height      = atlas->bitmap_data->height;
        u32 atlas_channels    = atlas->bitmap_data->channels;
        string_t atlas_pixels = atlas->bitmap_data->pixels;

        s32 texture_index = 0;
        for(const asset_handle_t *asset: atlas->textures_to_merge)
        {
            if(asset->slot->slot_state == ASLS_Loaded)
            {
                bitmap_t *asset_bitmap = &asset->slot->texture.bitmap;

                u32 padding = 1;
                u32 bitmap_width       = asset_bitmap->width;
                u32 bitmap_height      = asset_bitmap->height;
                u32 bitmap_channels    = asset_bitmap->channels;
                string_t bitmap_pixels = asset_bitmap->pixels;

                Assert(asset_bitmap->channels == atlas->bitmap_data->channels);

                // NOTE(Sleepster): Wrap to next y if needed. 
                if((atlas->atlas_cursor_x + bitmap_width) >= atlas_height)
                {
                    atlas->atlas_cursor_x  = 0;
                    atlas->atlas_cursor_y += atlas->tallest_y;
                }
                if(bitmap_height > atlas->tallest_y) atlas->tallest_y = bitmap_height;

                u32 atlas_cursor_x = atlas->atlas_cursor_x + padding;
                u32 atlas_cursor_y = atlas->atlas_cursor_y + padding;

                // NOTE(Sleepster): Copy by row. 
                for(u32 row_index = 0;
                    row_index < bitmap_height;
                    ++row_index)
                {
                    u32 atlas_bitmap_offset  = ((atlas_cursor_y + row_index) * atlas_width + atlas_cursor_x) * atlas_channels;
                    byte *atlas_pixel_offset = atlas_pixels.data + atlas_bitmap_offset;

                    u32 bitmap_offset = (row_index * bitmap_width) * bitmap_channels;
                    byte *bitmap_data_offset = bitmap_pixels.data + bitmap_offset;

                    memcpy(atlas_pixel_offset, bitmap_data_offset, bitmap_width * bitmap_channels);
                }

                vec2_t uv_min = vec2(atlas_cursor_x, atlas_cursor_y);
                vec2_t uv_max = vec2(atlas_cursor_x + bitmap_width, atlas_cursor_y + bitmap_height);

                // NOTE(Sleepster): Create the subtexture, let the owner of the sprite know this is the new texture we will draw it from. 
                s32 subtexture_index = atlas->packed_subtexture_count;
                subtexture_data_t *subtexture = asset->slot->subtexture_data;
                if(!subtexture)
                {
                    subtexture = atlas->packed_subtextures + atlas->packed_subtexture_count;
                    asset->slot->subtexture_data = subtexture;

                    ++atlas->packed_subtexture_count;
                }

                subtexture->uv_min                 = uv_min;
                subtexture->uv_max                 = uv_max;
                subtexture->offset                 = uv_min;
                subtexture->size                   = vec2(bitmap_width, bitmap_height);
                subtexture->atlas_subtexture_index = subtexture_index;
                subtexture->atlas                  = atlas;
                subtexture->atlased_generation     = asset->slot->package_generation;
                subtexture->packed_asset           = *asset;

                atlas->atlas_cursor_x = atlas_cursor_x + bitmap_width;
                c_dynarray_remove(&atlas->textures_to_merge, texture_index);

                atlas->merge_counter -= 1;
                Assert(atlas->merge_counter >= 0);
            }

            ++texture_index;
        }

        RHI_image_create_info_t info = {
            .data   = atlas->bitmap_data->pixels,
            .width  = atlas->bitmap_data->width,
            .height = atlas->bitmap_data->height,
            .format = BMF_RGBA32_SRGB,
            .usage  = RHI_IMAGE_USAGE_SHADER_SAMPLED_IMAGE,
        };

        if(atlas->texture.gpu_data.backend_image.handle == null)
        {
            asset_manager->RHI_context->backend_image_create(&info, &atlas->texture.gpu_data);
        }
        else
        {
            asset_manager->RHI_context->backend_image_update_contents(&atlas->texture.gpu_data);
        }
    }
    else
    {
        log_info("There are no textures to pack currently...\n");
    }
}

shader_t*
s_asset_get_shader_from_handle(asset_handle_t *handle) 
{
    shader_t *result = null;
    result = &handle->slot->shader;

    return(result);
}

texture2D_t*
s_asset_get_texture_from_handle(asset_handle_t *handle) 
{
    texture2D_t *result = null;
    result = &handle->slot->texture;;

    return(result);
}

material_data_t*
s_asset_get_material_data_from_handle(asset_handle_t *handle)
{
    material_data_t *result = null;
    result = &handle->slot->material;

    return(result);
}

material_archetype_t*
s_asset_get_material_archetype_from_handle(asset_handle_t *handle) 
{
    Assert(handle->material_info->material_type == SMT_Archetype);

    material_archetype_t *result = null;
    result = &handle->slot->material.archetype;

    return(result);
}

material_instance_t*
s_asset_get_material_instance_from_handle(asset_handle_t *handle)
{
    Assert(handle->material_info->material_type == SMT_Instance);

    material_instance_t *result = null;
    result = &handle->material_info->instance;

    return(result);
}

