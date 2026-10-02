/* ========================================================================
   $File: s_render_graph.cpp $
   $Date: September 30 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_render_graph.h>

render_command_t*
s_render_graph_get_command(render_graph_t *render_graph)
{
    render_command_t *result = null;
    result = render_graph->render_commands + render_graph->render_command_count++;

    return(result);
}

render_group_t*
s_render_group_begin(render_graph_t *render_graph, u32 renderpassID)
{
    render_group_t *result = null;
    result = render_graph->render_groups + render_graph->active_render_group_count++;
    ZeroStruct(*result);

    result->renderpassID = renderpassID;
    result->ID           = render_graph->active_render_group_count - 1;

    byte *data = c_arena_push_size(&render_graph->arena, MB(80));
    result->push_buffer.base = {
        data,
        MB(80)
    };
    result->push_buffer.used = 0;

    return(result);
}

internal_api byte*
push_vertices(render_group_push_buffer_t *buffer, u32 size)
{
    byte *result = null;
    Assert((s32)(buffer->used + size) <= buffer->base.count);

    result = buffer->base + buffer->used;
    buffer->used += size;

    return(result);
}

render_group_vertex_stream_t*
s_render_group_append_vertex_stream(render_group_t *render_group, u32 element_stride, s32 element_count, RHI_vertex_buffer_t *vertex_buffer)
{
    render_group_vertex_stream_t *result = render_group->vertex_streams + render_group->vertex_stream_count++;
    result->vertex_stride = element_stride;
    result->vertex_buffer = vertex_buffer;
    result->max_vertices  = element_count;
    result->vertices      = {
        push_vertices(&render_group->push_buffer, element_stride * element_count),
        (s32)(element_count * element_stride)
    };

    return(result);
}

s32
s_render_group_bind_texture(render_group_t *render_group, asset_handle_t texture)
{
    s32 result = 0;

    bool8 found = false;
    for(u32 texture_index = 0;
        texture_index < render_group->texture_count;
        ++texture_index)
    {
        asset_handle_t *texture_handle = render_group->textures + texture_index;
        if(texture_handle->texture == texture.texture)
        {
            found  = true;
            result = texture_index;

            break;
        }
    }

    if(!found)
    {       
        result = render_group->texture_count;
        render_group->textures[render_group->texture_count++] = texture;
    }

    return(result);
}

void
s_render_group_add_constant_buffer(render_group_t *render_group, RHI_uniform_constant_buffer_t *constant_buffer, void *data, u32 data_size)
{
    render_group_constant_buffer_info_t *info = render_group->constant_buffers + render_group->constant_buffer_count++;
    info->buffer = constant_buffer;
    info->buffer_data = data;
    info->data_size = data_size;
}

void
s_render_group_end(render_graph_t *render_graph, render_group_t *render_group)
{
    render_command_t *command = s_render_graph_get_command(render_graph);
    command->type = RENDER_GRAPH_COMMAND_TYPE_RENDER_GROUP;
    command->render_group = render_group;
    command->timestamp = SDL_GetPerformanceCounter(); 

    u64 pipeline_state_hash = 0;
    string_t pipeline_string = {
        .data  = (byte*)&render_group->pipeline_state,
        .count = sizeof(RHI_pipeline_state),
    };
    pipeline_state_hash = c_hash_table_hash_key(pipeline_string);

    u64 vertex_input_hash = (u64)render_group->index_buffer; 
    for(u32 stream_index = 0;
        stream_index < render_group->vertex_stream_count;
        ++stream_index)
    {
        render_group_vertex_stream_t *vertex_stream = render_group->vertex_streams + stream_index;
        string_t stream_string = {
            .data  = (byte*)vertex_stream,
            .count = sizeof(render_group_vertex_stream_t)
        };

        vertex_input_hash = c_hash_table_combine_hashes(vertex_input_hash, c_hash_table_hash_key(stream_string));
    }

    render_group->sort_key.renderpassID = render_group->renderpassID;
    render_group->sort_key.submission_index = render_group->ID;
    render_group->sort_key.pipeline_state_hash = pipeline_state_hash;
    render_group->sort_key.shader_hash = (u64)render_group->shader.slot;
    render_group->sort_key.vertex_input_hash = vertex_input_hash;
}

void
s_render_graph_command_blit_renderpasses(render_graph_t *render_graph, u32 renderpassA, u32 renderpassB)
{
    render_command_t *blit = s_render_graph_get_command(render_graph);
    blit->type = RENDER_GRAPH_COMMAND_TYPE_BLIT;
    blit->timestamp = SDL_GetPerformanceCounter();
    blit->blit.A = renderpassA;
    blit->blit.B = renderpassB;
}

void
s_render_graph_command_clear_renderpass(render_graph_t *render_graph, u32 renderpassID)
{
    render_command_t *clear = s_render_graph_get_command(render_graph);
    clear->type = RENDER_GRAPH_COMMAND_TYPE_CLEAR_RENDERPASS;
    clear->clear.A = renderpassID;
}

void
s_render_graph_output(render_state_t *render_state, RHI_image_t *present_image)
{
    render_graph_t *render_graph = &render_state->render_graph;
    RHI_command_list_t *command_list = RHI_get_command_list(render_state->RHI_context, RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

#if 0
    quicksort(render_graph->render_groups.items, render_graph->active_render_group_count, 
    [](const render_command_t &A, const render_command_t &B) -> int {
        if(B.sort_key.renderpassID        != A.sort_key.renderpassID)        return((B.sort_key.renderpassID        < A.sort_key.renderpassID)        - (B.sort_key.renderpassID        > A.sort_key.renderpassID));
        if(B.sort_key.pipeline_state_hash != A.sort_key.pipeline_state_hash) return((B.sort_key.pipeline_state_hash < A.sort_key.pipeline_state_hash) - (B.sort_key.pipeline_state_hash > A.sort_key.pipeline_state_hash));
        if(B.sort_key.shader_hash         != A.sort_key.shader_hash)         return((B.sort_key.shader_hash         < A.sort_key.shader_hash)         - (B.sort_key.shader_hash         > A.sort_key.shader_hash));
        if(B.sort_key.vertex_input_hash   != A.sort_key.vertex_input_hash)   return((B.sort_key.vertex_input_hash   < A.sort_key.vertex_input_hash)   - (B.sort_key.vertex_input_hash   > A.sort_key.vertex_input_hash));
        if(B.sort_key.descriptor_hash     != A.sort_key.descriptor_hash)     return((B.sort_key.descriptor_hash     < A.sort_key.descriptor_hash)     - (B.sort_key.descriptor_hash     > A.sort_key.descriptor_hash));
        return((B.sort_key.submission_index < A.sort_key.submission_index) - (B.sort_key.submission_index > A.sort_key.submission_index));
    });
#endif

    // NOTE(Sleepster): Recording Command Nodes 
    u32 active_renderpass = INVALID_ID;
    u64 active_pipeline_state = 0;
    u64 active_shader = 0;
    for(u32 command_index = 0;
        command_index < render_graph->render_command_count;
        ++command_index)
    {
        render_command_t *render_command = render_graph->render_commands + command_index;
        switch(render_command->type)
        {
            case RENDER_GRAPH_COMMAND_TYPE_RENDER_GROUP:
            {
                render_group_t *render_group = render_command->render_group;
                if(render_group->sort_key.renderpassID != active_renderpass)
                {
                    render_graph_node_t *new_node = render_graph->available_node_pool + render_graph->next_available_node++;

                    new_node->type = RENDER_GRAPH_NODE_TYPE_BEGIN_RENDERPASS;
                    new_node->begin_renderpass.renderpassID = render_group->renderpassID;

                    active_pipeline_state = 0;
                    active_shader = 0;
                    if(active_renderpass != INVALID_ID)
                    {
                        render_graph_node_t *end_renderpass = render_graph->available_node_pool + render_graph->next_available_node++;
                        end_renderpass->type = RENDER_GRAPH_NODE_TYPE_END_RENDERPASS;

                        active_renderpass = INVALID_ID;
                    }
                    else
                    {
                        active_renderpass = render_group->sort_key.renderpassID;
                    }
                }

                if(render_group->sort_key.pipeline_state_hash != active_pipeline_state)
                {
                    render_graph_node_t *set_pipeline_state = render_graph->available_node_pool + render_graph->next_available_node++;
                    set_pipeline_state->type = RENDER_GRAPH_NODE_TYPE_SET_PIPELINE_STATE;
                    set_pipeline_state->pipeline_state.state = render_group->pipeline_state;

                    active_pipeline_state = render_group->sort_key.pipeline_state_hash;
                }

                if((u64)render_group->shader.slot != active_shader)
                {
                    render_graph_node_t *bind_shader = render_graph->available_node_pool + render_graph->next_available_node++;
                    bind_shader->type = RENDER_GRAPH_NODE_TYPE_BIND_SHADER;
                    bind_shader->shader.handle = render_group->shader;

                    active_shader = (u64)render_group->shader.slot;
                }

                // TODO(Sleepster): Maybe separate these into bind textures + bind buffers because textures can be reused, but not buffers 
                render_graph_node_t *set_descriptors = render_graph->available_node_pool + render_graph->next_available_node++;
                {
                    set_descriptors->type = RENDER_GRAPH_NODE_TYPE_SET_DESCRIPTORS;
                    set_descriptors->descriptors.buffers      = render_group->constant_buffers.items;
                    set_descriptors->descriptors.buffer_count = render_group->constant_buffer_count;

                    set_descriptors->descriptors.textures      = render_group->textures.items;
                    set_descriptors->descriptors.texture_count = render_group->texture_count;
                }

                render_graph_node_t *draw_command = render_graph->available_node_pool + render_graph->next_available_node++;
                {
                    draw_command->type = RENDER_GRAPH_NODE_TYPE_DRAW;
                    draw_command->draw.element_count       = render_group->draw_element_count;
                    draw_command->draw.vertex_streams      = render_group->vertex_streams.items;
                    draw_command->draw.vertex_stream_count = render_group->vertex_stream_count;
                    draw_command->draw.index_buffer        = render_group->index_buffer;
                    draw_command->draw.line_width          = render_group->line_width;

                    draw_command->draw.viewport = {
                        render_group->viewport.offset,
                        render_group->viewport.extent,
                    };
                    draw_command->draw.scissor = {
                        render_group->scissor.offset,
                        render_group->scissor.extent,
                    };
                }
            }break;
            case RENDER_GRAPH_COMMAND_TYPE_BLIT:
            {
                if(active_renderpass != INVALID_ID)
                {
                    render_graph_node_t *end_renderpass = render_graph->available_node_pool + render_graph->next_available_node++;
                    end_renderpass->type = RENDER_GRAPH_NODE_TYPE_END_RENDERPASS;

                    active_renderpass = INVALID_ID;
                }

                render_graph_node_t *new_node = render_graph->available_node_pool + render_graph->next_available_node++;
                new_node->type = RENDER_GRAPH_NODE_TYPE_BLIT_RENDERPASS;
                new_node->usage_key = {};
                new_node->usage_key.read_renderpassID  = render_command->blit.A;
                new_node->usage_key.write_renderpassID = render_command->blit.B;

                new_node->blit.renderpassA = render_command->blit.A;
                new_node->blit.renderpassB = render_command->blit.B;
            }break;
            case RENDER_GRAPH_COMMAND_TYPE_CLEAR_RENDERPASS:
            {
                if(active_renderpass != render_command->clear.A)
                {
                    if(active_renderpass != INVALID_ID)
                    {
                        render_graph_node_t *end_renderpass = render_graph->available_node_pool + render_graph->next_available_node++;
                        end_renderpass->type = RENDER_GRAPH_NODE_TYPE_END_RENDERPASS;
                    }

                    render_graph_node_t *new_node = render_graph->available_node_pool + render_graph->next_available_node++;
                    new_node->type = RENDER_GRAPH_NODE_TYPE_BEGIN_RENDERPASS;
                    new_node->begin_renderpass.renderpassID = render_command->clear.A;

                    active_pipeline_state = 0;
                    active_shader         = 0;

                    active_renderpass = render_command->clear.A;
                }

                render_graph_node_t *clear_node = render_graph->available_node_pool + render_graph->next_available_node++;
                clear_node->type = RENDER_GRAPH_NODE_TYPE_CLEAR_RENDERPASS;
            }break;
        }
    }

    if(active_renderpass != INVALID_ID)
    {
        render_graph_node_t *end_renderpass = render_graph->available_node_pool + render_graph->next_available_node++;
        end_renderpass->type = RENDER_GRAPH_NODE_TYPE_END_RENDERPASS;
    }

    for(u32 node_index = 0;
        node_index < render_graph->next_available_node;
        ++node_index)
    {
        render_graph_node_t *current_node = render_graph->available_node_pool + node_index;
        switch(current_node->type)
        {
            case RENDER_GRAPH_NODE_TYPE_BEGIN_RENDERPASS:   {RHI_cmd_renderpass_begin(command_list, current_node->begin_renderpass.renderpassID);                  }break;
            case RENDER_GRAPH_NODE_TYPE_END_RENDERPASS:     {RHI_cmd_renderpass_end(command_list);                                                                 }break;
            case RENDER_GRAPH_NODE_TYPE_CLEAR_RENDERPASS:   {RHI_cmd_clear_renderpass_attachments(command_list, current_node->clear.renderpassID);                 }break;
            case RENDER_GRAPH_NODE_TYPE_BLIT_RENDERPASS:    {RHI_cmd_blit_renderpass(command_list, current_node->blit.renderpassA, current_node->blit.renderpassB);}break;
            case RENDER_GRAPH_NODE_TYPE_SET_PIPELINE_STATE: {RHI_cmd_set_render_state(command_list, &current_node->pipeline_state.state);                          }break;
            case RENDER_GRAPH_NODE_TYPE_BIND_SHADER:        {RHI_cmd_use_shader_program(command_list, current_node->shader.handle);                                }break;
            case RENDER_GRAPH_NODE_TYPE_SET_DESCRIPTORS:    
            {
                for(u32 buffer_index = 0;
                    buffer_index < current_node->descriptors.buffer_count;
                    ++buffer_index)
                {
                    render_group_constant_buffer_info_t *info = current_node->descriptors.buffers + buffer_index;
                    RHI_cmd_update_constant_buffer(command_list, info->buffer, info->buffer_data, info->data_size);
                }

                for(u32 texture_index = 0;
                    texture_index < current_node->descriptors.texture_count;
                    ++texture_index)
                {
                    asset_handle_t *handle  = current_node->descriptors.textures + texture_index;
                    texture2D_t    *texture = handle->texture;
                    if(handle->slot && handle->slot->subtexture_data)
                    {
                        texture = &handle->slot->subtexture_data->atlas->texture;
                    }

                    RHI_cmd_bind_texture_image(command_list, texture);
                }
            }break;
            case RENDER_GRAPH_NODE_TYPE_DRAW:               
            {
                RHI_vertex_buffer_t **vertex_buffers_alloc = c_arena_push_array(&gc->temp_arena, RHI_vertex_buffer_t*, current_node->draw.vertex_stream_count);
                array_view_t<RHI_vertex_buffer_t*> vertex_buffers = {
                    .items = vertex_buffers_alloc,
                    .count = (s32)current_node->draw.vertex_stream_count
                };

                s32 found_vertex_buffer_count = 0;
                for(u32 vertex_stream_index = 0;
                    vertex_stream_index < current_node->draw.vertex_stream_count;
                    ++vertex_stream_index)
                {
                    render_group_vertex_stream_t *stream = current_node->draw.vertex_streams + vertex_stream_index;

                    stream->vertex_buffer->vertex_data   = stream->vertices.items;
                    stream->vertex_buffer->vertex_count  = stream->vertex_count;
                    stream->vertex_buffer->vertex_offset = stream->vertex_offset;
                    RHI_cmd_update_buffer_contents(command_list, stream->vertex_buffer);
                    s32 index = c_array_add_if_unique(vertex_buffers, &stream->vertex_buffer, found_vertex_buffer_count);
                    if(index == -1)
                    {
                        ++found_vertex_buffer_count;
                    }
                }

                RHI_cmd_bind_vertex_buffers(command_list, vertex_buffers.items, found_vertex_buffer_count);
                if(current_node->draw.index_buffer)
                {
                    RHI_cmd_bind_index_buffer(command_list, current_node->draw.index_buffer);
                }

                RHI_cmd_set_viewport(command_list, current_node->draw.viewport.offset, current_node->draw.viewport.extent);
                RHI_cmd_set_scissor(command_list,  current_node->draw.scissor.offset,  current_node->draw.scissor.extent);
 
                if(current_node->draw.index_buffer)
                {
                    render_group_vertex_stream_t *stream = &current_node->draw.vertex_streams[0];
                    RHI_cmd_draw_indexed(command_list, ((stream->vertex_count / 4) * 6), 0, 0, 1, 0);
                }
                else
                {
                    render_group_vertex_stream_t *stream = &current_node->draw.vertex_streams[0];
                    RHI_cmd_set_line_width(command_list, current_node->draw.line_width);
                    RHI_cmd_draw(command_list, stream->vertex_count, 0, 1, 0);
                }
            }break;
        }
    }

    RHI_cmd_present(command_list, present_image);

    render_graph->render_command_count = 0;
    render_graph->active_render_group_count = 0;
    render_graph->next_available_node = 0;
    c_arena_reset(&render_graph->arena);
}
