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
#include <shaders/test.glsl.h>

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
    u32 pipeline_id = pipeline->pipeline.id;
    u32 texture_id  = 0;
    if(render_state->draw_frame.active_texture.is_valid)
    {
        texture_id = render_state->draw_frame.active_texture.texture->view.id;
    }

    u64 hash_index = ((u64)pipeline_id << 32) | texture_id;

    hash_index ^= hash_index >> 33;
    hash_index *= 0xff51afd7ed558ccdULL;
    hash_index ^= hash_index >> 33;
    hash_index %= MAX_RENDER_GROUPS;

    render_group_t *render_group = render_state->render_groups + hash_index;
    if(!(render_group->flags & RGF_Valid))
    {
        render_group->ID                  = hash_index;
        render_group->flags               = RGF_Valid;
        render_group->desc.texture_handle = render_state->draw_frame.active_texture;
        render_group->desc.pipeline       = pipeline;

        render_state->groups_created++;
        r_render_group_init_geometry_buffer(render_state, &render_group->first_buffer);
    }

    u32 index = render_state->draw_frame.used_render_group_count;
    render_state->draw_frame.used_render_groups[index] = render_group->ID;
    render_state->draw_frame.used_render_group_count += 1;

    Assert(render_group);

    render_state->draw_frame.active_render_group = render_group;
}

void
r_renderpass_end(render_state_t *render_state)
{
    render_state->draw_frame.active_render_group = null;
    r_reset_pipeline_render_state(render_state);
}

void
r_renderpass_update_render_buffers(render_state_t *render_state)
{
    draw_frame_t *draw_frame = &render_state->draw_frame;

    for(u32 render_group_index = 0;
        render_group_index < draw_frame->used_render_group_count;
        ++render_group_index)
    {
        u32 ID = draw_frame->used_render_groups[render_group_index];

        render_group_t *render_group = render_state->render_groups + ID;
        for(render_group_geometry_buffer_t *buffer = &render_group->first_buffer;
            buffer;
            buffer = buffer->next_buffer)
        {
            Assert(buffer->quad_count > 0);

            buffer->main_vertex_buffer_offset = draw_frame->total_vertex_count;
            for(u32 quad_index = 0;
                quad_index < buffer->quad_count;
                ++quad_index)
            {
                render_quad_t *render_quad = buffer->quad_buffer + quad_index;
                vertex_t      *buffer_ptr  = buffer->vertex_buffer + buffer->vertex_count;

                memcpy(buffer_ptr, render_quad->vertices, sizeof(vertex_t) * 4);
                buffer->vertex_count += 4;
            }
            // TODO(Sleepster): If this is multithreaded... we will die. 
            memcpy(draw_frame->main_vertex_buffer + draw_frame->total_vertex_count,
                   buffer->vertex_buffer,
                   buffer->vertex_count * sizeof(vertex_t));

            draw_frame->total_vertex_count += buffer->vertex_count;
        }
    }
}

void
r_render_group_to_output(render_state_t *render_state, render_group_t *render_group)
{
    for(render_group_geometry_buffer_t *buffer = &render_group->first_buffer;
        buffer;
        buffer = buffer->next_buffer)
    {
        if(render_group->desc.pipeline->pipeline.id)
        {
            sg_apply_pipeline(render_group->desc.pipeline->pipeline);

            bool8 nearest_sampling = render_state->default_texture.texture->filter_type == TAAFT_NEAREST;
            if(render_group->desc.texture_handle.texture)
            {
                nearest_sampling = render_group->desc.texture_handle.texture->filter_type == TAAFT_NEAREST;

                render_state->bindings.views[0]    = render_group->desc.texture_handle.texture->view;
                render_state->bindings.samplers[0] = nearest_sampling ? render_state->nearest_filter_sampler : render_state->linear_filter_sampler; 
            }
            else
            {
                render_state->bindings.views[0]    = render_state->default_texture.texture->view;
                render_state->bindings.samplers[0] = nearest_sampling ? render_state->nearest_filter_sampler : render_state->linear_filter_sampler; 
            }
            sg_apply_bindings(render_state->bindings);

            VSParams_t matrices = {};
            memcpy(matrices.uProjectionMatrix, buffer->render_camera->projection_matrix.values, sizeof(float32) * 16);
            memcpy(matrices.uViewMatrix,       buffer->render_camera->view_matrix.values,       sizeof(float32) * 16);

            sg_range matrix_range = {
                .ptr  = &matrices,
                .size = sizeof(VSParams_t)
            };
            sg_apply_uniforms(UB_VSParams, matrix_range);

            u32 index_offset = (buffer->main_vertex_buffer_offset / 4) * 6;
            sg_draw(index_offset, 6 * buffer->quad_count, 1);

            buffer->quad_count   = 0;
            buffer->vertex_count = 0;
        }
    }
}
