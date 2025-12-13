/* ========================================================================
   $File: r_render_interface.cpp $
   $Date: December 11 2025 11:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <r_render_interface.h>
#include <s_asset_manager.h>
#include <s_renderer.h>
#include <r_render_group.h>

internal_api render_quad_t
r_create_draw_quad(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color, asset_handle_t handle)
{
    render_quad_t result = {};

    result.position = position;
    result.size     = size;
    result.color    = color;

    float32 top    = position.y + size.y;
    float32 left   = position.x;
    float32 bottom = position.y;
    float32 right  = position.x + size.x;

    Assert(render_state->draw_frame.active_render_layer < MAX_RENDER_LAYERS);

    float32 near_value = -1;
    float32 far_value  =  1;

    float32 depth_step        = (far_value - near_value) / MAX_RENDER_LAYERS;
    float32 layer_depth_value = near_value + (render_state->draw_frame.active_render_layer * depth_step);

    result.bottom_left.v_position  = vec4(left, bottom,  layer_depth_value, 1.0f);
    result.top_left.v_position     = vec4(left, top,     layer_depth_value, 1.0f);
    result.top_right.v_position    = vec4(right, top,    layer_depth_value, 1.0f);
    result.bottom_right.v_position = vec4(right, bottom, layer_depth_value, 1.0f);
    
    if(!handle.texture->is_valid)
    {
        handle = render_state->default_texture;
    }
    top    = handle.texture->uv_max->y;
    left   = handle.texture->uv_min->x;
    bottom = handle.texture->uv_min->y;
    right  = handle.texture->uv_max->x;

    result.bottom_left.v_texcoords  = vec2(left,  bottom);
    result.top_left.v_texcoords     = vec2(left,  top);
    result.top_right.v_texcoords    = vec2(right, top);
    result.bottom_right.v_texcoords = vec2(right, bottom);

    for(u32 index = 0;
        index < 4;
        ++index)
    {
        result.vertices[index].v_color = color;
    }

    return(result);
}

render_quad_t*
r_draw_rect(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color)
{
    Expect(render_state->draw_frame.active_render_group, "No valid render_group set as active group...\n");
    render_quad_t *result = null;

    render_group_geometry_buffer_t *render_buffer = r_render_group_get_buffer(render_state, render_state->draw_frame.active_render_group);
    Assert(render_buffer);

    render_quad_t quad = r_create_draw_quad(render_state, position, size, color, render_state->draw_frame.active_texture);
    render_buffer->quad_buffer[render_buffer->quad_count] = quad;
    
    result = render_buffer->quad_buffer + render_buffer->quad_count;
    render_buffer->quad_count += 1;

    return(result);
}

