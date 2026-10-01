/* ========================================================================
   $File: s_render_graph.cpp $
   $Date: September 30 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_render_graph.h>

true_inline render_graph_command_t*
s_render_graph_get_next_command(render_graph_t *render_graph)
{
    render_graph_command_t *result;
    result = render_graph->command_nodes + render_graph->command_node_count++;

    return(result);
}

render_graph_section_t*
s_render_graph_acquire_render_section(render_graph_t *render_graph, s32 sectionID)
{
    render_graph_section_t *result = null;

    render_graph_command_t *command = null;
    for(u32 command_index = 0;
        command_index < render_graph->command_node_count;
        ++command_index)
    {
        render_graph_command_t *found = render_graph->command_nodes + command_index;
        if(found->type == RENDER_GRAPH_COMMAND_SET_TYPE_RENDER_SECTION)
        {
            if(found->render_section->sectionID == sectionID)
            {
                command = found;
                break;
            }
        }
    }

    if(!command)
    {
        command = s_render_graph_get_next_command(render_graph);
        command->type           = RENDER_GRAPH_COMMAND_SET_TYPE_RENDER_SECTION;
        command->render_section = render_graph->render_sections + sectionID;
        command->render_section->sectionID   = sectionID;
    }

    result = command->render_section;
    return(result);
}

render_group_t*
s_render_group_begin(render_graph_section_t *section, u32 renderpassID)
{
    render_group_t *result = null;
    result = section->render_groups + section->used_render_group_count++;
    if(result->push_buffer.is_initialized == false) 
    {
        result->push_buffer = c_arena_create(MB(50), ALLOCATOR_TAG_RHI);
    }
    else
    {
        c_arena_reset(&result->push_buffer);

        // NOTE(Sleepster): Zero all but the memory arena  
        memset((byte*)result + sizeof(memory_arena_t), 0, sizeof(render_group_t) - sizeof(memory_arena_t));
        result->pipeline_state = {};
    }

    result->ID           = section->used_render_group_count - 1;
    result->renderpassID = renderpassID;
    result->draw_element_count = 0;

    return(result);
}

void
s_render_group_end(render_group_t *render_group)
{
    string_t pipeline_state_key = {
        .data = (byte *)&render_group->pipeline_state,
        .count = sizeof(render_group->pipeline_state)
    };
    u64 pipeline_state_hash = c_hash_table_hash_key(pipeline_state_key);

    u64 vertex_stream_hash = 2166136261u;
    for(u32 vertex_stream_index = 0;
        vertex_stream_index < render_group->vertex_stream_count;
        ++vertex_stream_index)
    {
        render_group_vertex_stream_t *vertex_stream = render_group->vertex_streams + vertex_stream_index;
        string_t vertex_stream_key = {
            .data = (byte*)vertex_stream,
            .count = sizeof(render_group_vertex_stream_t)
        };
        vertex_stream_hash = c_hash_table_combine_hashes(vertex_stream_hash, c_hash_table_hash_key(vertex_stream_key));
    }

    if(render_group->index_buffer)
    {
        string_t index_buffer_key = {
            .data = (byte*)render_group->index_buffer,
            .count = sizeof(render_group->index_buffer)
        };
        vertex_stream_hash = c_hash_table_combine_hashes(vertex_stream_hash, c_hash_table_hash_key(index_buffer_key));
    }

    u64 shaderID = (u64)(render_group->shader.slot);
    render_group->sort_key.renderpassID        =      render_group->renderpassID;
    render_group->sort_key.shader_hash         = (u32)shaderID;
    render_group->sort_key.submission_index    =      render_group->ID;
    render_group->sort_key.pipeline_state_hash = (u32)pipeline_state_hash;
    render_group->sort_key.vertex_input_hash   = (u32)vertex_stream_hash;
}

s32
s_render_group_append_vertex_stream(render_group_t *render_group, u32 vertex_stride, RHI_vertex_buffer_t *vertex_buffer)
{
    s32 result = 0;
    result = render_group->vertex_stream_count;

    render_group_vertex_stream_t *vertex_stream = render_group->vertex_streams + render_group->vertex_stream_count++;
    vertex_stream->vertex_buffer = vertex_buffer;
    vertex_stream->vertex_stride = vertex_stride;

    return(result);
}

void
s_render_group_push_vertex_stream_data(render_group_t *render_group, s32 vertex_stream_index, void *buffer, s32 vertex_count)
{
    render_group_vertex_stream_t *stream = render_group->vertex_streams + vertex_stream_index;
    stream->vertices     = buffer;
    stream->vertex_count = vertex_count;
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
s_render_group_add_constant_buffer(render_group_t *render_group, RHI_uniform_constant_buffer_t *buffer, void *buffer_data, u32 data_size)
{
    render_group_constant_buffer_info_t *buffer_info = render_group->constant_buffers + render_group->constant_buffer_count++;
    byte *copied_data = c_arena_push_size(&gc->temp_arena, data_size);
    memcpy(copied_data, buffer_data, data_size);

    buffer_info->buffer      = buffer;
    buffer_info->buffer_data = copied_data;
    buffer_info->data_size   = data_size;
}

void
s_render_graph_add_section_barrier(render_graph_command_t *command, render_graph_section_t *dependancy)
{
    (void)command;
    (void)dependancy;
}

void
s_render_graph_cmd_blit_renderpasses(render_graph_command_t *command, u32 A, u32 B)
{
    command->type = RENDER_GRAPH_COMMAND_SET_TYPE_BLIT_SECTION;
    command->blit_command.renderpassA = A;
    command->blit_command.renderpassB = B;
}

void
s_render_graph_cmd_clear_renderpass_attachments(render_graph_t *render_graph, u32 renderpassID)
{
    render_graph_command_t *command = s_render_graph_get_next_command(render_graph);
    command->type = RENDER_GRAPH_COMMAND_SET_TYPE_CLEAR_RENDERPASS_ATTACHMENTS;
    command->clear_renderpass.renderpassID = renderpassID;
}


void
render_groups_to_output(render_state_t *render_state)
{
    render_graph_t *render_graph = &render_state->render_graph;

    RHI_command_list_t *command_list = RHI_get_command_list(render_state->RHI_context, RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
    for(u32 render_graph_command_index = 0;
        render_graph_command_index < render_graph->command_node_count;
        ++render_graph_command_index)
    {
        render_graph_command_t *command = render_graph->command_nodes + render_graph_command_index;
        switch(command->type)
        {
            case RENDER_GRAPH_COMMAND_SET_TYPE_RENDER_SECTION:
            {
                render_graph_section_t *section = command->render_section;
                quicksort(section->render_groups.items, section->used_render_group_count, [](render_group_t &A, render_group_t &B) -> int {
                      if(A.sort_key.renderpassID        != B.sort_key.renderpassID)        return((A.sort_key.renderpassID        < B.sort_key.renderpassID) - (A.sort_key.renderpassID        > B.sort_key.renderpassID));
                      if(A.sort_key.pipeline_state_hash != B.sort_key.pipeline_state_hash) return((A.sort_key.pipeline_state_hash < B.sort_key.pipeline_state_hash) - (A.sort_key.pipeline_state_hash > B.sort_key.pipeline_state_hash));
                      if(A.sort_key.shader_hash         != B.sort_key.shader_hash)         return((A.sort_key.shader_hash         < B.sort_key.shader_hash) - (A.sort_key.shader_hash         > B.sort_key.shader_hash));
                      if(A.sort_key.vertex_input_hash   != B.sort_key.vertex_input_hash)   return((A.sort_key.vertex_input_hash   < B.sort_key.vertex_input_hash) - (A.sort_key.vertex_input_hash   > B.sort_key.vertex_input_hash));
                      if(A.sort_key.descriptor_hash     != B.sort_key.descriptor_hash)     return((A.sort_key.descriptor_hash     < B.sort_key.descriptor_hash) - (A.sort_key.descriptor_hash     > B.sort_key.descriptor_hash));
                      return((A.sort_key.submission_index < B.sort_key.submission_index) - (A.sort_key.submission_index > B.sort_key.submission_index));
                });


                render_group_sort_key_t cached_key = { .renderpassID = INVALID_ID };
                for(u32 render_group_index = 0;
                    render_group_index < section->used_render_group_count;
                    ++render_group_index)
                {
                    render_group_t *render_group = section->render_groups + render_group_index;
                    if(render_group->sort_key.renderpassID != cached_key.renderpassID)        
                    {
                        if(cached_key.renderpassID != INVALID_ID) RHI_cmd_renderpass_end(command_list);
                        RHI_cmd_renderpass_begin(command_list, render_group->renderpassID);
                    }

                    if(render_group->clear_attachments == true)
                    {
                        // TODO(Sleepster): Not happy about this 
                        RHI_cmd_clear_renderpass_attachments(command_list, render_group->renderpassID);
                    }

                    if(render_group->sort_key.pipeline_state_hash != cached_key.pipeline_state_hash) RHI_cmd_set_render_state(command_list,  &render_group->pipeline_state);
                    if(render_group->sort_key.shader_hash         != cached_key.shader_hash)         RHI_cmd_use_shader_program(command_list, render_group->shader);
                    //if(render_group->sort_key.descriptor_hash     != last_key.descriptor_hash) 
                    {
                        for(u32 texture_index = 0;
                            texture_index < render_group->texture_count;
                            ++texture_index)
                        {
                            asset_handle_t *handle  = render_group->textures + texture_index;
                            texture2D_t    *texture = handle->texture;
                            if(handle->slot && handle->slot->subtexture_data)
                            {
                                texture = &handle->slot->subtexture_data->atlas->texture;
                            }

                            RHI_cmd_bind_texture_image(command_list, texture);
                        }

                        for(u32 constant_buffer_index = 0;
                            constant_buffer_index < render_group->constant_buffer_count;
                            ++constant_buffer_index)
                        {
                            render_group_constant_buffer_info_t *info = render_group->constant_buffers + constant_buffer_index;
                            RHI_cmd_update_constant_buffer(command_list, info->buffer, info->buffer_data, info->data_size);
                        }
                    }
                    //if(render_group->sort_key.vertex_input_hash   != cached_key.vertex_input_hash)
                    {
                        RHI_vertex_buffer_t **vertex_buffers_alloc = c_arena_push_array(&gc->temp_arena, RHI_vertex_buffer_t*, render_group->vertex_stream_count);
                        array_view_t<RHI_vertex_buffer_t*> vertex_buffers = {
                            .items = vertex_buffers_alloc,
                            .count = (s32)render_group->vertex_stream_count
                        };

                        s32 found_vertex_buffer_count = 0;
                        for(u32 vertex_stream_index = 0;
                            vertex_stream_index < render_group->vertex_stream_count;
                            ++vertex_stream_index)
                        {
                            render_group_vertex_stream_t *stream = render_group->vertex_streams + vertex_stream_index;
                            RHI_vertex_buffer_reset_count(stream->vertex_buffer);

                            stream->vertex_buffer->vertex_data   = (byte*)stream->vertices;
                            stream->vertex_buffer->vertex_count  = stream->vertex_count;
                            stream->vertex_buffer->vertex_offset = stream->vertex_offset;
                            RHI_cmd_update_buffer_contents(command_list, stream->vertex_buffer);
                            s32 index = c_array_add_if_unique(vertex_buffers, &stream->vertex_buffer, found_vertex_buffer_count);
                            if(index == -1)
                            {
                                ++found_vertex_buffer_count;
                            }
                        }

                        RHI_cmd_bind_vertex_buffers(command_list, vertex_buffers.items[0], found_vertex_buffer_count);
                        if(render_group->index_buffer)
                        {
                            RHI_cmd_bind_index_buffer(command_list, render_group->index_buffer);
                        }
                    }

                    RHI_cmd_set_viewport(command_list, render_group->viewport.offset, render_group->viewport.extent);
                    RHI_cmd_set_scissor(command_list,  render_group->scissor.offset,  render_group->scissor.extent);

                    // TODO(Sleepster): Stupid. For now we assume you want to draw lines if you don't have an index buffer. Dumb.
                    // Bad. Awful.
                    if(render_group->index_buffer)
                    {
                        RHI_cmd_draw_indexed(command_list, (render_group->draw_element_count * 6), 0, 0, 1, 0);
                    }
                    else
                    {
                        RHI_cmd_set_line_width(command_list, render_group->line_width);
                        RHI_cmd_draw(command_list, render_group->draw_element_count * 2, 0, 1, 0);
                    }

                    cached_key = render_group->sort_key;
                }

                RHI_cmd_renderpass_end(command_list);
                section->used_render_group_count = 0;
            }break;
            case RENDER_GRAPH_COMMAND_SET_TYPE_BLIT_SECTION:
            {
                RHI_cmd_blit_renderpass(command_list, command->blit_command.renderpassA, command->blit_command.renderpassB);
            }break;
            case RENDER_GRAPH_COMMAND_SET_TYPE_CLEAR_RENDERPASS_ATTACHMENTS:
            {
                RHI_cmd_clear_renderpass_attachments(command_list, command->clear_renderpass.renderpassID);
            }break;
        }
    }

    RHI_cmd_present(command_list, &render_state->fullscreen_color_buffer);

    render_graph->command_node_count   = 0;
    render_graph->render_section_count = 0;
}

