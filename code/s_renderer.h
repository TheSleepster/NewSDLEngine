#if !defined(S_RENDERER_H)
/* ========================================================================
   $File: s_renderer.h $
   $Date: December 07 2025 01:10 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_RENDERER_H
#include <SDL3/SDL.h>

#include <c_types.h>
#include <c_base.h>
#include <c_log.h>
#include <c_globals.h>
#include <c_math.h>

#include <g_game_state.h>

constexpr u32 MAX_QUADS    = 2500;
constexpr u32 MAX_VERTICES = MAX_QUADS * 4;
constexpr u32 MAX_INDICES  = MAX_QUADS * 6;

#pragma pack(push, 1)
struct vertex_t 
{
    vec4_t  v_position;
    vec4_t  v_color;
};
#pragma pack(pop)

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

struct render_group_t
{
};

struct render_state_t
{
    sg_pipeline    pipeline;
    sg_bindings    bindings;
    sg_pass_action pass_action;

    sg_sampler     linear_sampler;
    sg_sampler     nearest_sampler;

    mat4_t         projection_matrix;
    mat4_t         view_matrix;

    vertex_t      *vertex_buffer;
    u32            vertex_count;

    render_quad_t *quad_buffer;
    u32            quad_count;
};

void s_init_renderer(render_state_t *render_state, game_state_t *state);
void s_renderer_draw_frame(game_state_t *state, render_state_t *render_state);
#endif // s_RENDERER_H

