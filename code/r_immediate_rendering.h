#if !defined(R_IMMEDIATE_RENDERING_H)
/* ========================================================================
   $File: r_immediate_rendering.h $
   $Date: April 29 2026 06:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_IMMEDIATE_RENDERING_H
#include <c_math.h>
#include <c_string.h>

#include <r_render_image.h>
#include <s_render_RHI.h>
#include <s_asset_manager.h>

struct alignas(16) immediate_vertex_t
{
    vec4_t vPosition;
    vec4_t vColor;
    vec2_t vTexCoord;
    vec2_t vPadding;
};

void immediate_quad_ex(render_command_list_t *command_list, vertex_buffer_t *buffer, vec3_t position, vec2_t render_size, vec4_t render_color, vec2_t uv_min, vec2_t uv_max, vec2_t padding, texture2D_t *texture);
void immediate_rect(render_command_list_t *command_list, vertex_buffer_t *buffer, vec3_t position, vec2_t render_size, vec4_t render_color, vec2_t padding);
void immediate_text(render_command_list_t *command_list, vertex_buffer_t *vertex_buffer, asset_manager_t *asset_manager, asset_handle_t *asset_handle, string_t render_string, vec3_t position, vec4_t text_color, vec2_t padding, u32 font_size);

#endif // R_IMMEDIATE_RENDERING_H

