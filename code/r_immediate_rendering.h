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

// NOTE(Sleepster): 
// We hated this idea, so why is it here? well, unfortunately because it has to be. The idea is simple, we need
// to group vertices and their data for rendering in an optimal manner. The characteristics that must be met are:
// - Same renderpass
// - Uniforms must be identical
// - Same shader
//
// This is so that in instances like the widget rendering, we can optimally (or once again, in the widget's case at
// all) render the items as needed. There are instances where we just want to be able to render things but cannot 
// enforce specific behavior... like here:
#if 0 
internal_api void
render_widget_hierarchy(ui_state_t *ui_state, render_command_list_t *command_list, widget_t *first_widget)
{
    widget_t *current_widget = first_widget;
    do {
        if(current_widget->widget_flags & UI_WIDGET_FLAG_HAS_TEXT)
        {
            immediate_text(command_list, 
                          &ui_state->vertex_buffer, 
                           ui_state->asset_manager,
                          &ui_state->default_font,
                           current_widget->widget_text,
                           current_widget->state->position, 
                           current_widget->state->render_color,
                           ui_state->default_font_size);
        }
        else
        {
            immediate_rect(command_list,
                           &ui_state->vertex_buffer,
                           current_widget->state->position, 
                           current_widget->state->render_size,
                           current_widget->state->render_color);
        }

        if(current_widget->first_child)
        {
            render_widget_hierarchy(ui_state, command_list, current_widget->first_child);
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != first_widget);
}
#endif
// God forbid anyone want a custom shader, or a custom texture applied to it. This is the crux of the problem that
// the render_group needs to solve. This is here because it might be useful outside of the widget code and is simply
// a wrapper around the vertex buffer data. A core characteristic is the "marker" code, where you can just say something
// like:
//
// "Vertices 0-100 use the same data, sort them as a unit"
//
// Which will then allow us to sort ALL 100 VERTICES in the buffer at once.

struct render_group_marker_t
{
    u32 first_vertex;
    u32 last_vertex;
};

struct render_group_t
{
    u32 renderpass_ID;
    u64 shader_ID;
};

struct alignas(16) immediate_vertex_t
{
    vec4_t vPosition;
    vec4_t vColor;
    vec2_t vTexCoord;
    vec2_t vPadding;
};


void immediate_quad_ex(render_command_list_t *command_list, vertex_buffer_t *buffer, vec3_t position, vec2_t render_size, vec4_t render_color, vec2_t uv_min, vec2_t uv_max, texture2D_t *texture);
void immediate_rect(render_command_list_t *command_list, vertex_buffer_t *buffer, vec3_t position, vec2_t render_size, vec4_t render_color);
void immediate_text(render_command_list_t *command_list, vertex_buffer_t *vertex_buffer, asset_manager_t *asset_manager, asset_handle_t *asset_handle, string_t render_string, vec3_t position, vec4_t text_color, u32 font_size);

#endif // R_IMMEDIATE_RENDERING_H

