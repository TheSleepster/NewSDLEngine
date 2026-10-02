/* ========================================================================
   $File: r_immediate_rendering.cpp $
   $Date: April 29 2026 06:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <r_immediate_rendering.h>
#include <s_render_graph.h>

ENGINE_API void
immediate_put_data(RHI_vertex_buffer_t *buffer, immediate_vertex_t *buffer_data, u32 element_size, u32 element_count)
{
    immediate_vertex_t *vertex_pointer = (immediate_vertex_t*)buffer->vertex_data;
    Assert(buffer->buffer_data.buffer_element_size == element_size);

    memcpy(vertex_pointer, buffer_data, element_size * element_count);
    buffer->vertex_count += element_count;
}

/*
=============
QUADS
=============
*/

ENGINE_API void
immediate_quad_ex(immediate_vertex_t *vertex_buffer_data,
                  vec3_t              position,
                  vec2_t              render_size,
                  vec4_t              render_color,
                  vec2_t              uv_min,
                  vec2_t              uv_max,
                  vec2_t              padding,
                  vec2_t              sdf_info,
                  vec2_t              padding0)
{
    immediate_vertex_t *vertex_pointer = vertex_buffer_data;
    Expect(vertex_pointer, "The vertex buffer pointer is invalid...");

    immediate_vertex_t *bottom_right = vertex_pointer + 0;
    immediate_vertex_t *top_right    = vertex_pointer + 1;
    immediate_vertex_t *top_left     = vertex_pointer + 2;
    immediate_vertex_t *bottom_left  = vertex_pointer + 3;

    float32 top    = position.y + render_size.y;
    float32 bottom = position.y;
    float32 left   = position.x;
    float32 right  = position.x + render_size.x;

    bottom_left->vPosition  = vec4(left,  bottom, position.z, 1);
    bottom_right->vPosition = vec4(right, bottom, position.z, 1);
    top_left->vPosition     = vec4(left,  top,    position.z, 1);
    top_right->vPosition    = vec4(right, top,    position.z, 1);

    bottom_left->vColor  = render_color;
    bottom_right->vColor = render_color;
    top_left->vColor     = render_color;
    top_right->vColor    = render_color;

    bottom_left->vPadding  = padding;
    bottom_right->vPadding = padding;
    top_left->vPadding     = padding;
    top_right->vPadding    = padding;

    bottom_left->vPadding0  = padding0;
    bottom_right->vPadding0 = padding0;
    top_left->vPadding0     = padding0;
    top_right->vPadding0    = padding0;

    bottom_left->vSDFInfo  = sdf_info;
    bottom_right->vSDFInfo = sdf_info;
    top_left->vSDFInfo     = sdf_info;
    top_right->vSDFInfo    = sdf_info;

    float32 tbottom = uv_max.y;
    float32 ttop    = uv_min.y;
    float32 tleft   = uv_min.x;
    float32 tright  = uv_max.x;

    bottom_left->vTexCoord  = vec2(tleft,  tbottom);
    bottom_right->vTexCoord = vec2(tright, tbottom);
    top_left->vTexCoord     = vec2(tleft,  ttop);
    top_right->vTexCoord    = vec2(tright, ttop);
}

ENGINE_API void
immediate_rect_ex(immediate_vertex_t  *vertex_buffer_data,
                  vec3_t               position,
                  vec2_t               render_size,
                  vec4_t               render_color,
                  vec2_t               uv_min,
                  vec2_t               uv_max,
                  vec2_t               padding,
                  vec2_t               sdf_info,
                  vec2_t               padding0)
{
    immediate_quad_ex(vertex_buffer_data,
                      position, 
                      render_size, 
                      render_color,
                      uv_min,
                      uv_max,
                      padding,
                      sdf_info,
                      padding0);
}

ENGINE_API void
immediate_rect(immediate_vertex_t *vertex_buffer_data,
               vec3_t              position,
               vec2_t              render_size,
               vec4_t              render_color)
{
    vec2_t zero = vec2_zero();
    immediate_quad_ex(vertex_buffer_data,
                      position, 
                      render_size, 
                      render_color,
                      zero,
                      zero,
                      zero,
                      zero,
                      zero);
}

// NOTE(Sleepster): Returns the number of quads rendered 
ENGINE_API s32
immediate_text(render_group_t     *render_group, 
               immediate_vertex_t *vertex_buffer_data,
               asset_manager_t    *asset_manager,
               asset_handle_t     *font_handle,
               string_t            render_string, 
               vec3_t              position, 
               vec4_t              text_color,
               float32             settings,
               u32                 font_size)
{
    s32 result = 0;

    Assert(font_handle);
    Assert(font_handle->slot->type == AT_Font);
    if(*font_handle->is_valid)
    {
        dynamic_render_font_varient_t *varient = s_asset_font_acquire_font_at_size(asset_manager, 
                                                                                   font_handle, 
                                                                                   font_size);

        vec2_t render_position = position.xy;
        for(u32 character_index = 0;
            character_index < render_string.count;
            ++character_index)
        {
            u8 *character = render_string.data + character_index;
            if(*character != '0')
            {
                glyph_metric_t *metrics = s_asset_font_fetch_glyph(asset_manager, varient, character);
                if(metrics->is_valid)
                {
#if 0
                    s_render_group_bind_texture(render_group, metrics->synthetic_texture_handle);
#endif
                }
            }
        }

        for(u32 character_index = 0;
            character_index < render_string.count;
            ++character_index)
        {
            u8 *character = render_string.data + character_index;
            if(*character != '0')
            {
                glyph_metric_t *metrics = s_asset_font_fetch_glyph(asset_manager, varient, character);
                if(metrics->is_valid)
                {
#if 0
                    s32 texture_index = s_render_group_bind_texture(render_group, metrics->synthetic_texture_handle);
                    immediate_quad_ex(vertex_buffer_data + (result * 4),
                                      vec2_expand_vec3(vec2_subtract(render_position, vec2(0, metrics->offset_y)), position.z),
                                      vec2(metrics->width, metrics->height),
                                      text_color,
                                      metrics->atlas_offset,
                                      vec2_add(metrics->atlas_offset, metrics->atlas_size),
                                      vec2(settings, texture_index),
                                      vec2_zero(),
                                      vec2_zero());
#endif
                    render_position.x += metrics->advance;
                    ++result;
                }
                else
                {
                    log_info("Glyph data for character: '%c' is not valid yet...\n", *character);
                }
            }
        }
    }

    return(result);
}

/*
=============
LINES
=============
*/

ENGINE_API void
immediate_line(immediate_vertex_t *vertex_buffer_data,
               vec2_t              start,
               vec2_t              end,
               float32             depth,
               vec4_t              render_color)
{
    immediate_vertex_t *vertex_pointer = vertex_buffer_data;
    Expect(vertex_pointer, "The vertex buffer pointer is invalid...");

    immediate_vertex_t *first  = vertex_pointer;
    immediate_vertex_t *second = vertex_pointer + 1;

    first->vPosition = vec4(start.x, start.y, depth, 1.0f);
    first->vColor    = render_color;

    second->vPosition = vec4(end.x, end.y, depth, 1.0f);
    second->vColor    = render_color;
}
