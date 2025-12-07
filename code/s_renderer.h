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
#include <c_math.h>
#include <c_log.h>
#include <c_globals.h>

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


struct render_group_t
{
};

struct render_state_t
{
    sg_pipeline    pipeline;
    sg_bindings    bindings;
    sg_pass_action pass_action;
};

void s_init_renderer(render_state_t *render_state, game_state_t *state);
void s_renderer_draw_frame(game_state_t *state, render_state_t *render_state);
#endif // s_RENDERER_H

