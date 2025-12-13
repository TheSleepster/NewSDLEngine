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
#include <c_dynarray.h>
#include <c_hash_table.h>

#include <s_asset_manager.h>
#include <r_asset_texture.h>
#include <g_game_state.h>

#include <r_render_interface.h>

#define MAX_RENDER_GROUPS (1031)

struct render_group_t;
struct render_group_desc_t;
struct render_group_geometry_buffer_t;

struct render_shader_t 
{
          sg_shader       ID;
    const sg_shader_desc *desc;
};

struct render_pipeline_state_t 
{
    bool8           depth_test;
    bool8           depth_writing;
    u32             depth_func;

    bool8           blending;
    u32             src_rgb_factor;
    u32             src_alpha_factor;
    u32             dst_rgb_factor;
    u32             dst_alpha_factor;

    u32             color_blend_op;
    u32             alpha_blend_op;

    render_shader_t *active_shader;
    sg_view         *active_texture;
};

struct render_pipeline_data_t
{
    render_pipeline_state_t p_state;
    sg_pipeline_desc        desc;
    sg_pipeline             pipeline;
};

struct draw_frame_t
{
    render_pipeline_state_t pipeline_state;
    render_camera_t        *active_camera;
    render_shader_t        *active_shader;
    asset_handle_t          active_texture;
    u32                     active_render_layer;

    render_group_t         *active_render_group;
    u32                     used_render_groups[64];
    u32                     used_render_group_count;

    vertex_t               *main_vertex_buffer;
    u32                     total_vertex_count;
};

struct render_state_t
{
    memory_arena_t        renderer_arena;
    memory_arena_t        draw_frame_arena;

    sg_pipeline           pipeline;
    sg_bindings           bindings;
    sg_pass_action        pass_action;

    sg_sampler            nearest_filter_sampler;
    sg_sampler            linear_filter_sampler;

    draw_frame_t          draw_frame;

    render_camera_t       default_camera;
    render_shader_t       default_shader;
    asset_handle_t        default_texture;

    mat4_t                projection_matrix;
    mat4_t                view_matrix;

    render_group_t       *render_groups;
    u32                   groups_created;

    vertex_t             *vertex_buffer;
    u32                   vertex_count;

    render_quad_t        *quad_buffer;
    u32                   quad_count;

    DynArray(render_pipeline_data_t) pipelines;
    u32                              created_pipeline_count;
};

void s_init_renderer(render_state_t *render_state, asset_manager_t *asset_manager, game_state_t *state);
void s_renderer_draw_frame(game_state_t *state, asset_manager_t *asset_manager, render_state_t *render_state);
void r_texture_upload(texture2D_t *texture, bool8 is_mutable, bool8 has_AA, filter_type_t filter_type);

render_pipeline_data_t* r_get_or_create_pipeline(render_state_t *render_state,  render_shader_t *shader, render_pipeline_state_t pipeline_state);
void                    r_reset_pipeline_render_state(render_state_t *render_state);

#endif // s_RENDERER_H

