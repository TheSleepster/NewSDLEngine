/* ========================================================================
   $File: r_render_group.cpp $
   $Date: December 11 2025 03:47 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_log.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_dynarray.h>

#include <r_render_group.h>

/*
 *  - [ ] Rendergroups must be pulled from the hash so that their data persists
 *  - [ ] Support different cameras per geometry buffer
 *  - [ ] Create different pipelines per different render_group 
 */

void
r_render_group_init_geometry_buffer(render_state_t *render_state, render_group_geometry_buffer_t *buffer)
{
    ZeroStruct(*buffer);

    buffer->quad_buffer   = c_arena_push_array(&render_state->renderer_arena, render_quad_t, MAX_QUADS);
    buffer->vertex_buffer = c_arena_push_array(&render_state->renderer_arena, vertex_t,      MAX_VERTICES);
    buffer->render_camera = render_state->draw_frame.active_camera;
}

render_group_geometry_buffer_t*
r_render_group_create_new_geometry_buffer(render_state_t *render_state, render_group_t *render_group)
{
    render_group_geometry_buffer_t *new_buffer = c_arena_push_struct(&render_state->renderer_arena, render_group_geometry_buffer_t); 
    r_render_group_init_geometry_buffer(render_state, new_buffer);

    render_group_geometry_buffer_t *old_buffer = render_group->first_buffer.next_buffer;
    Assert(old_buffer);

    render_group->first_buffer.next_buffer = new_buffer;
    new_buffer->next_buffer = old_buffer;

    return(new_buffer);
}

render_group_geometry_buffer_t*
r_render_group_get_buffer(render_state_t *render_state, render_group_t *render_group)
{
    render_group_geometry_buffer_t *result = null;
    for(render_group_geometry_buffer_t *buffer = &render_group->first_buffer;
        buffer;
        buffer = buffer->next_buffer)
    {
        if(buffer->render_camera == render_state->draw_frame.active_camera)
        {
            result = buffer;
            break;
        }
    }
    
    if(!result)
    {
        result = r_render_group_create_new_geometry_buffer(render_state, render_group);
    }

    Expect(result, "We have failed to create a new render_group_geometry_buffer...\n");
    return(result);
}

void
r_renderpass_begin(render_state_t *render_state)
{
    if(!render_state->render_groups)
    {
        render_state->render_groups = c_arena_push_array(&render_state->renderer_arena, render_group_t, MAX_RENDER_GROUPS);
    }
    Expect(render_state->render_groups, "Somehow the render_groups array is invalid...\n");

    render_pipeline_data_t *pipeline = r_get_or_create_pipeline(render_state, 
                                                                render_state->draw_frame.active_shader, 
                                                                render_state->draw_frame.pipeline_state);
    u32 pipeline_id = pipeline->ID;
    u32 texture_id  = render_state->draw_frame.active_texture.texture->viewID;

    u64 hash_index = ((u64)((u64)pipeline_id >> 32) | ((u64)(texture_id) >> 0));
    hash_index    %= MAX_RENDER_GROUPS;

    render_group_t *render_group = render_state->render_groups + hash_index;
    if(!(render_group->flags & RGF_Valid))
    {
        render_group->ID                  = render_state->groups_created++;
        render_group->flags               = RGF_Valid;
        render_group->desc.texture_handle = render_state->draw_frame.active_texture;
        render_group->desc.pipeline       = pipeline;

        r_render_group_init_geometry_buffer(render_state, &render_group->first_buffer);
    }

    render_state->draw_frame.active_render_group = render_group;
}

void
r_renderpass_end(render_state_t *render_state)
{
    render_state->draw_frame.active_render_group = null;
    r_reset_pipeline_render_state(render_state);
}
