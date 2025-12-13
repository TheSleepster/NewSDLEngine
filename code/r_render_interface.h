#if !defined(R_RENDER_INTERFACE_H)
/* ========================================================================
   $File: r_render_interface.h $
   $Date: December 11 2025 11:51 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_INTERFACE_H
#include <c_types.h>
#include <c_math.h>

struct render_state_t;

constexpr u32 MAX_QUADS    = 2500;
constexpr u32 MAX_VERTICES = MAX_QUADS * 4;
constexpr u32 MAX_INDICES  = MAX_QUADS * 6;

#define MAX_RENDER_LAYERS (32)

#pragma pack(push, 1)
struct vertex_t 
{
    vec4_t  v_position;
    vec4_t  v_color;
    vec2_t  v_texcoords;
};
#pragma pack(pop)

struct render_camera_t
{
    mat4_t view_matrix;
    mat4_t projection_matrix;

    mat4_t view_projection_matrix;
};

enum render_group_geometry_type_t
{
    RGGT_Invalid,
    RGGT_Quads,
    RGGT_Lines,
    RGGT_Count
};

struct render_quad_t
{
    union 
    {
        vertex_t vertices[4];
        struct 
        {
            vertex_t bottom_left;
            vertex_t top_left;
            vertex_t top_right;
            vertex_t bottom_right;
        };
    };

    vec2_t position;
    vec2_t size;
    vec4_t color;
};

struct render_line_t
{
    union 
    {
        vertex_t points[2];
        struct 
        {
            vertex_t start;
            vertex_t end;
        };
    };
};

render_quad_t* r_draw_rect(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color);

#endif // R_RENDER_INTERFACE_H

