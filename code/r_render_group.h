#if !defined(R_RENDER_GROUP_H)
/* ========================================================================
   $File: r_render_group.h $
   $Date: December 11 2025 03:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_GROUP_H
#include <c_types.h>
#include <c_base.h>
#include <c_log.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <s_renderer.h>

enum render_group_flags_t 
{
    RGF_None  = (1 << 0),
    RGF_Valid = (1 << 1),
};

struct render_group_geometry_buffer_t
{
    u32              geometry_type;


    render_quad_t   *quad_buffer;
    u32              quad_count;

    render_line_t   *line_buffer;
    u32              line_count;

    u32              main_vertex_buffer_offset;
    vertex_t        *vertex_buffer;
    u32              vertex_count;

    render_camera_t *render_camera;

    render_group_geometry_buffer_t *next_buffer;
};

struct render_group_render_desc_t
{
    asset_handle_t           texture_handle;
    render_pipeline_data_t  *pipeline;
};

struct render_group_t
{
    u32                            ID;
    u32                            flags;
    render_group_render_desc_t     desc;
    render_group_geometry_buffer_t first_buffer;
};

render_group_geometry_buffer_t* r_render_group_get_buffer(render_state_t *render_state, render_group_t *render_group);
void                            r_renderpass_begin(render_state_t *render_state);
void                            r_renderpass_end(render_state_t *render_state);
void                            r_render_group_to_output(render_state_t *render_state, render_group_t *render_group);
void                            r_renderpass_update_render_buffers(render_state_t *render_state);

#endif // R_RENDER_GROUP_H

