/* ========================================================================
   $File: s_renderer.cpp $
   $Date: February 22 2026 05:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_global_context.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <c_log.h>

#include <vk_backend_core.h>
#include <s_asset_manager.h>
#include <r_render_image.h>
#include <s_render_RHI.h>

/*
=============
s_renderer_state_init
=============
*/

void
s_renderer_state_init(renderer_state_t *renderer_state, void *render_context)
{
    renderer_state->renderer_arena  = c_arena_create(MB(100));
    renderer_state->transient_arena = c_arena_create(MB(100));

    renderer_state->command_lists   = c_arena_push_array(&renderer_state->renderer_arena, render_command_list_t, MAX_COMMAND_LISTS);
    c_hash_table_init(&renderer_state->constant_buffer_hash, MAX_CONSTANT_BUFFERS);

    renderer_state->render_context  = render_context;
}

/*
=============
s_renderer_handle_window_resize
=============
*/

void
s_renderer_handle_window_resize(renderer_state_t *renderer_state, vec2_t window_size)
{
    vulkan_context_t *vulkan_context = (vulkan_context_t *)renderer_state->render_context;
    vk_backend_handle_window_resize(vulkan_context, window_size);

    renderer_state->window_size.x = window_size.x;
    renderer_state->window_size.y = window_size.y;

    renderer_state->last_window_size_generation     = renderer_state->current_window_size_generation;
    renderer_state->current_window_size_generation += 1;

    for(u32 renderpass_index = 0;
        renderpass_index < renderer_state->renderpass_count;
        ++renderpass_index)
    {
        renderpass_t *renderpass = renderer_state->renderpasses + renderpass_index;
        if(renderpass->resize_with_window)
        {
            for(u32 color_attachment_index = 0;
                color_attachment_index < renderpass->color_attachment_count;
                ++color_attachment_index)
            {
                renderpass_attachment_t *attachment = renderpass->color_attachments + color_attachment_index;
                image_create_info_t *info = &attachment->image->create_info;
                info->width  = renderer_state->window_size.x;
                info->height = renderer_state->window_size.y;

                s_renderer_image_destroy(renderer_state, attachment->image);
                *attachment->image = s_renderer_image_create(renderer_state, info);
            }

            if(renderpass->has_depth_stencil_attachment)
            {
                image_create_info_t *info = &renderpass->depth_stencil_attachment.image->create_info;
                info->width  = renderer_state->window_size.x;
                info->height = renderer_state->window_size.y;

                s_renderer_image_destroy(renderer_state, renderpass->depth_stencil_attachment.image);
                *renderpass->depth_stencil_attachment.image = s_renderer_image_create(renderer_state, info);
            }

            renderpass->create_info.render_width  = renderer_state->window_size.x;
            renderpass->create_info.render_height = renderer_state->window_size.y;
            if(renderpass->create_info.color_attachment_count > 0)
            {
                s_renderer_resize_renderpass(renderer_state, renderpass);
                log_info("renderpass with ID: '%u' rebuilt with the current window size...\n", renderpass->ID);
            }
        }
    }
}

void
s_renderer_execute_backend_commands(renderer_state_t *renderer_state)
{
    vk_backend_render_frame((vulkan_context_t*)renderer_state->render_context, renderer_state);
}

/*
=============
s_renderer_build_renderpass
=============
*/

u32 
s_renderer_build_renderpass(renderer_state_t *renderer_state, renderpass_desc_t *renderpass_desc)
{
    u32 result = INVALID_ID;
    renderpass_t *renderpass = renderer_state->renderpasses + renderer_state->renderpass_count;
    Assert(renderpass);
    
    renderpass->create_info            = *renderpass_desc;
    renderpass->render_width           =  renderpass_desc->render_width;
    renderpass->render_height          =  renderpass_desc->render_height;
    renderpass->color_attachment_count =  renderpass_desc->color_attachment_count;
    renderpass->resize_with_window     =  renderpass_desc->resize_with_window;
    renderpass->ID                     =  renderer_state->renderpass_count++;

    result = renderpass->ID;

    Assert(renderer_state->renderpass_count <= 100);
    Assert(renderpass_desc->color_attachment_count > 0);
    Assert(renderpass_desc->color_attachment_count <= MAX_RENDER_TARGET_ATTACHMENTS);
    
    // NOTE(Sleepster): Copy color attachments 
    memcpy(renderpass->color_attachments, 
           renderpass_desc->color_attachments, 
           renderpass_desc->color_attachment_count * sizeof(renderpass_attachment_t));

    // NOTE(Sleepster): Copy depth_stencil attachment 
    memcpy(&renderpass->depth_stencil_attachment, 
           &renderpass_desc->depth_stencil_attachment, 
            sizeof(renderpass_attachment_t));

    u32 attachment_count = vk_backend_initialize_RHI_renderpass(renderer_state, renderpass_desc, renderpass);
    renderpass->total_attachment_count = attachment_count;

    return(result);
}

true_inline void
s_renderer_resize_renderpass(renderer_state_t *renderer_state, renderpass_t *renderpass)
{
    vk_backend_initialize_RHI_renderpass(renderer_state, &renderpass->create_info, renderpass);
    renderpass->render_width  = renderpass->create_info.render_width;
    renderpass->render_height = renderpass->create_info.render_height;
}

/////////////////////////
// VERTEX AND INDEX BUFFERS 
/////////////////////////

/*
=============
s_renderer_vertex_buffer_create
=============
*/

true_inline render_buffer_t
s_renderer_vertex_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, u32 element_size, render_buffer_advance_rate_t rate, void *data, u32 size)
{
    render_buffer_desc_t buffer_desc = {
        .type = RenderBufferType_VertexBuffer,
        .allocation_type = memory_type,
        .buffer_capacity = size,
        .element_size    = element_size,
        .initial_data    = data
    };
    render_buffer_t result = s_renderer_render_buffer_create(renderer_state, &buffer_desc);
    return(result);
}

/*
=============
s_renderer_index_buffer_create
=============
*/

true_inline render_buffer_t
s_renderer_index_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, u32 element_size, void *data, u32 size)
{
    render_buffer_desc_t buffer_desc = {
        .type = RenderBufferType_IndexBuffer,
        .allocation_type = memory_type,
        .buffer_capacity = size,
        .element_size    = element_size,
        .initial_data    = data
    };
    render_buffer_t result = s_renderer_render_buffer_create(renderer_state, &buffer_desc);
    return(result);
}

/*
=============
s_renderer_render_buffer_create
=============
*/

render_buffer_t
s_renderer_render_buffer_create(renderer_state_t           *renderer_state, 
                                render_buffer_desc_t       *buffer_desc)
{
    render_buffer_t result = {};
    result.buffer_ID           = c_fnv_hash_value((byte*)buffer_desc, sizeof(render_buffer_desc_t));
    result.type                = buffer_desc->type;
    result.buffer_capacity     = buffer_desc->buffer_capacity;
    result.buffer_element_size = buffer_desc->element_size;
    result.allocation_type     = buffer_desc->allocation_type;

    render_buffer_type_t buffer_type = buffer_desc->type;

    VkBufferUsageFlags             usage_flags      = (VkBufferUsageFlagBits)0;
    vulkan_allocation_usage_type_t allocation_flags = (vulkan_allocation_usage_type_t)0;
    Assert(buffer_type != RenderBufferType_Invalid);

    usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if(buffer_type == RenderBufferType_IndexBuffer)
    {
        usage_flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    else if (buffer_type == RenderBufferType_VertexBuffer)
    {
        usage_flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if(buffer_desc->allocation_type == RenderBufferAllocationTypeMapped)
    {
        allocation_flags = VULKAN_MEMORY_USAGE_CPU_TO_GPU; 
    }
    else if(buffer_desc->allocation_type == RenderBufferAllocationTypeGPUOnly)
    {
        allocation_flags = VULKAN_MEMORY_USAGE_GPU_ONLY;
    }

    result.buffer = vk_backend_buffer_create((vulkan_context_t *)renderer_state->render_context, 
                                             buffer_desc->buffer_capacity,
                                             usage_flags,
                                             allocation_flags);
    if(buffer_desc->initial_data != null && buffer_desc->buffer_capacity > 0)
    {
        vk_backend_buffer_copy_data((vulkan_context_t*)renderer_state->render_context, &result.buffer, buffer_desc->initial_data, buffer_desc->buffer_capacity, 0);
    }

    return(result);
}

/*
=============
s_renderer_render_buffer_copy_data
=============
*/

true_inline void
s_renderer_render_buffer_copy_data(renderer_state_t *renderer_state, render_buffer_t *buffer, void *data, u32 size, u32 offset)
{
    vk_backend_buffer_copy_data((vulkan_context_t*)renderer_state->render_context, &buffer->buffer, data, size, offset);
}

/////////////////////////
// UNIFORM BUFFERS 
/////////////////////////

// NOTE(Sleepster): 
// This currently can't fail... even if you mess up the uniform buffer name... 
// perhaps that's a bad thing in all honesty...
//
// We could perhaps do what we do for assets and shader parameters where we preload
// the name of the item into the hash table so that if you ask for a constant buffer that literally
// doesn't exist and can't exist, it's a reasonable error.
uniform_constant_buffer_t*
s_renderer_get_constant_buffer(renderer_state_t *renderer_state, string_t uniform_name)
{
    uniform_constant_buffer_t *result = null;
    result = c_hash_table_get_value_ptr(&renderer_state->constant_buffer_hash, uniform_name);
    if(result)
    {
        result->uniform_hash_index  = c_fnv_hash_value(uniform_name.data, uniform_name.count);
        result->uniform_hash_index %= MAX_CONSTANT_BUFFERS;
    }
    else
    {
        log_fatal("Idk how, but there's no place for this constant_buffer...\n");
        InvalidCodePath;
    }

    return(result);
}

/////////////////////////
// COMMAND LISTS
/////////////////////////

/*
=============
s_renderer_command_list_init
=============
*/

internal_api void
s_renderer_command_list_init(renderer_state_t *renderer_state, render_command_list_t *list)
{
    if(list->is_initialized == false)
    {
        list->transient_arena = c_arena_create(MB(10));
        list->command_arena   = c_arena_create(MB(30));

        list->active_vertex_buffers = c_dynarray_create(render_buffer_t *);
    }
    else
    {
        c_arena_reset(&list->transient_arena);
        c_arena_reset(&list->command_arena);

        c_dynarray_clear(list->active_vertex_buffers);
        memset(list->image_ids_to_bind,   0, sizeof(u32)      * MAX_SHADER_IMAGE_PARAMS);
        memset(list->image_shader_params, 0, sizeof(image_t*) * MAX_SHADER_IMAGE_PARAMS);
    }

    Assert(global_context);
    Assert(global_context->renderer_state);
    asset_catalog_t *texture_catalog = global_context->asset_manager->texture_catalog;

    list->commands               = c_arena_push_array(&list->command_arena, render_command_t, MAX_RENDER_COMMANDS);
    list->active_render_state    = g_pipeline_default_state_key;
    list->image_shader_params[0] = &texture_catalog->default_asset.texture->gpu_data;

    list->is_initialized      = true;

}

/*
=============
s_renderer_get_command_list
=============
*/

render_command_list_t*
s_renderer_get_command_list(renderer_state_t *renderer_state)
{
    render_command_list_t *result = null;
    result = renderer_state->command_lists + renderer_state->command_list_count++;
    Assert(renderer_state->command_list_count < MAX_COMMAND_LISTS);
    result->renderer_state = renderer_state;

    s_renderer_command_list_init(renderer_state, result);
    Assert(result->is_initialized == true);
    Assert(result->transient_arena.is_initialized == true);
    Assert(result->command_arena.is_initialized   == true);

    return(result);
}

/*
=============
s_renderer_get_next_command
=============
*/

internal_api true_inline render_command_t*
s_renderer_get_next_command(render_command_list_t *command_list)
{
    render_command_t *result = null;
    result = command_list->commands + command_list->command_count++;

    return(result);
}

/*
=============
s_renderer_is_texture_bound
=============
*/

bool8
s_renderer_is_texture_bound(render_command_list_t *command_list, texture2D_t *texture)
{
    bool8 result = false;
    for(u32 bound_textures_index = 0;
        bound_textures_index < command_list->bound_image_count;
        ++bound_textures_index)
    {
        u32 image_id = command_list->image_ids_to_bind[bound_textures_index];
        if(image_id == texture->gpu_data.ID)
        {
            result = true;
            break;
        }
    }

    return(result);
}

/*
=============
r_cmd_renderpass_begin
=============
*/

void
r_cmd_renderpass_begin(render_command_list_t *command_list, u32 renderpassID)
{
    Assert(command_list->active_renderpass == null);

    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_begin_renderpass_t *begin_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                               render_command_begin_renderpass_t);
    begin_renderpass->ID = renderpassID;

    command->header.command_type = RCT_BeginRenderpass;
    command->data                = begin_renderpass;

    command_list->active_renderpass = command_list->renderer_state->renderpasses + renderpassID;
}


/*
=============
r_cmd_renderpass_end
=============
*/

// TODO(Sleepster): 
// This is probably a problem. Not really sure why we're actually operating on command list here when it could 
// be accessed from multiple threads
void
r_cmd_renderpass_end(render_command_list_t *command_list)
{
    Assert(command_list->active_renderpass != null);

    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_end_renderpass_t *end_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                           render_command_end_renderpass_t);
    end_renderpass->ID = command_list->active_renderpass->ID;

    command->header.command_type = RCT_EndRenderpass;
    command->data                = end_renderpass;

    command_list->active_renderpass = null;
}

/*
=============
r_cmd_use_shader_program
=============
*/

void
r_cmd_use_shader_program(render_command_list_t *command_list, asset_handle_t asset_handle)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_bind_shader_t *bind_shader = c_arena_push_struct(&command_list->command_arena, 
                                                                     render_command_bind_shader_t);

    asset_slot_load_status_t load_status = asset_handle.slot->slot_state; 
    if(load_status != ASLS_Loaded)
    {
        Assert(asset_handle.catalog);
        if(load_status == ASLS_Unloaded)
        {
            // signal for load
        }
        // apply a default texture
        bind_shader->shader = asset_handle.catalog->default_asset;
    }
    else if(load_status == ASLS_Loaded)
    {
        bind_shader->shader = asset_handle;
    }

    command->header.command_type = RCT_BindShader;
    command->data = bind_shader;
}

/*
=============
r_cmd_bind_texture_from_handle
=============
*/

void
r_cmd_bind_texture_from_handle(render_command_list_t *command_list, asset_handle_t *asset_handle)
{
    Assert(asset_handle->type == AT_Bitmap);

    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_bind_texture_t *bind_texture = c_arena_push_struct(&command_list->command_arena, 
                                                                       render_command_bind_texture_t);
    asset_slot_load_status_t load_status = asset_handle->slot->slot_state; 
    if(load_status != ASLS_Loaded)
    {
        if(load_status == ASLS_Unloaded)
        {
            // signal for load
        }
        // apply a default texture
        bind_texture->texture = &asset_handle->catalog->default_asset.texture->gpu_data;
        Assert(bind_texture->texture->ID != 0);
    }
    else if(load_status == ASLS_Loaded)
    {
        bind_texture->texture = &asset_handle->texture->gpu_data;
    }

    command->header.command_type = RCT_BindTexture;
    command->data = bind_texture;

    command_list->image_ids_to_bind[command_list->bound_image_count++] = bind_texture->texture->ID;
}

/*
=============
r_cmd_bind_texture_image
=============
*/

void
r_cmd_bind_texture_image(render_command_list_t *command_list, texture2D_t *texture)
{
    Assert(texture);

    if(texture->gpu_data.ID > 0)
    {
        render_command_t *command  = s_renderer_get_next_command(command_list);
        render_command_bind_texture_t *bind_texture = c_arena_push_struct(&command_list->command_arena, 
                                                                           render_command_bind_texture_t);
        bind_texture->texture = &texture->gpu_data;

        command->header.command_type = RCT_BindTexture;
        command->data = bind_texture;

        command_list->image_ids_to_bind[command_list->bound_image_count++] = bind_texture->texture->ID;
    }
    else
    {
        log_warning("Attempting to bind a texture where it's ID == 0. This may be intentional...\n");
    }
}

/*
=============
r_cmd_bind_vertex_buffer
=============
*/

void
r_cmd_bind_vertex_buffer(render_command_list_t *command_list, render_buffer_t *buffer)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_bind_vertex_buffer_t *bind_vertex_buffer = c_arena_push_struct(&command_list->command_arena, 
                                                                                   render_command_bind_vertex_buffer_t);
    Assert(buffer->type == RenderBufferType_VertexBuffer);
    bind_vertex_buffer->buffer = buffer; 

    command->header.command_type = RCT_BindVertexBuffer;
    command->data = bind_vertex_buffer;
}

/*
=============
r_cmd_bind_index_buffer
=============
*/

void
r_cmd_bind_index_buffer(render_command_list_t *command_list, render_buffer_t *buffer)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_bind_index_buffer_t *bind_index_buffer = c_arena_push_struct(&command_list->command_arena, 
                                                                                  render_command_bind_index_buffer_t);

    Assert(buffer->type == RenderBufferType_IndexBuffer);
    bind_index_buffer->buffer = buffer; 

    command->header.command_type = RCT_BindIndexBuffer;
    command->data = bind_index_buffer;
}

/*
=============
r_cmd_set_viewport
=============
*/

void
r_cmd_set_viewport(render_command_list_t *command_list, vec2_t offset, vec2_t size)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_set_viewport_t *set_viewport = c_arena_push_struct(&command_list->command_arena, 
                                                                       render_command_set_viewport_t);
    set_viewport->size   = vec2(size.x, size.y);
    set_viewport->offset = vec2(offset.x, offset.y);

    command->header.command_type = RCT_SetViewport;
    command->data = set_viewport;
}

/*
=============
r_cmd_set_scissor
=============
*/

void
r_cmd_set_scissor(render_command_list_t *command_list, vec2_t offset, vec2_t size)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_set_scissor_t *set_scissor = c_arena_push_struct(&command_list->command_arena, 
                                                                     render_command_set_scissor_t);
    set_scissor->size   = size;
    set_scissor->offset = offset;

    command->header.command_type = RCT_SetScissor;
    command->data = set_scissor;
}

/*
=============
r_cmd_update_push_constants
=============
*/

void
r_cmd_update_push_constants(render_command_list_t *command_list, u32 offset, u32 size, void *data) 
{
    Assert(size <= 128);

    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_update_push_constant_t *update_constant = c_arena_push_struct(&command_list->command_arena, 
                                                                                  render_command_update_push_constant_t);
    update_constant->data   = data;
    update_constant->size   = size;
    update_constant->offset = offset;

    command->header.command_type = RCT_UpdatePushConstants;
    command->data = update_constant;
}

/*
=============
r_cmd_update_constant_buffer
=============
*/

void
r_cmd_update_constant_buffer(render_command_list_t *command_list, uniform_constant_buffer_t *buffer, void *data, u32 data_size)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_update_uniform_constant_buffer_t *update_buffer_contents = c_arena_push_struct(&command_list->command_arena, 
                                                                                                   render_command_update_uniform_constant_buffer_t);
    update_buffer_contents->constant_data_size = data_size;

    // TODO(Sleepster): Abstract this... 
    vulkan_context_t *vulkan_context = (vulkan_context_t*)command_list->renderer_state->render_context;
    update_buffer_contents->buffer                     = buffer;
    update_buffer_contents->uniform_hash_index         = buffer->uniform_hash_index;
    update_buffer_contents->backend_uniform_buffer_ptr = vk_backend_append_uniform_constant_buffer_data(vulkan_context, 
                                                                                                        data, 
                                                                                                        data_size, 
                                                                                                       &buffer->offset);
    update_buffer_contents->constant_buffer_offset = buffer->offset;

    buffer->mapped_data = update_buffer_contents->backend_uniform_buffer_ptr;
    buffer->size        = data_size;

    command->header.command_type = RCT_UpdateUniformConstantBuffer;
    command->data                = update_buffer_contents;
}

/*
=============
r_cmd_update_buffer_contents
=============
*/

void
r_cmd_update_buffer_contents(render_command_list_t *command_list, render_buffer_t *buffer, void *data, u32 data_size)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_update_render_buffer_contents_t *update_buffer_contents = c_arena_push_struct(&command_list->command_arena, 
                                                                                                  render_command_update_render_buffer_contents_t);
    update_buffer_contents->data_size = data_size;
    update_buffer_contents->offset    = buffer->buffer_elements_used;
    update_buffer_contents->buffer    = buffer;

    buffer->buffer_elements_used += (data_size / buffer->buffer_element_size);

    vulkan_context_t *vulkan_context = (vulkan_context_t*)command_list->renderer_state->render_context;
    vk_backend_buffer_append_data(vulkan_context, &buffer->buffer, data, data_size);

    command->data = update_buffer_contents; 
    command->header.command_type = RCT_UpdateBufferContents;
}

/*
=============
r_cmd_set_render_state
=============
*/

void
r_cmd_set_render_state(render_command_list_t *command_list, render_pipeline_state_t *render_pipeline_state)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_set_pipeline_state_t *set_render_state = c_arena_push_struct(&command_list->command_arena, 
                                                                                render_command_set_pipeline_state_t);
    set_render_state->pipeline_state = *render_pipeline_state;

    command->header.command_type = RCT_SetRenderState;
    command->data = set_render_state;
}


/*
=============
r_cmd_reset_render_state
=============
*/

void
r_cmd_reset_render_state(render_command_list_t *command_list, render_pipeline_state_t *render_pipeline_state)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    command->header.command_type = RCT_ResetRenderState;
}

/*
=============
r_cmd_draw
=============
*/

void
r_cmd_draw(render_command_list_t *command_list, 
           u32                    vertex_count, 
           u32                    vertex_offset, 
           u32                    instance_count, 
           u32                    first_instance)
{
    render_command_t *command   = s_renderer_get_next_command(command_list);
    render_command_draw_t *draw = c_arena_push_struct(&command_list->command_arena, 
                                                       render_command_draw_t);
    draw->vertices_to_draw = vertex_count;
    draw->vertex_offset    = vertex_offset;
    draw->instance_count   = instance_count;
    draw->first_instance   = first_instance;

    command->header.command_type = RCT_Draw;
    command->data = draw;

    command_list->bound_image_count = 0;
}

/*
=============
r_cmd_draw_indexed
=============
*/

void
r_cmd_draw_indexed(render_command_list_t *command_list, 
                   u32                    index_count, 
                   u32                    index_offset, 
                   u32                    instance_count, 
                   u32                    first_instance)
{
    render_command_t *command   = s_renderer_get_next_command(command_list);
    render_command_draw_t *draw = c_arena_push_struct(&command_list->command_arena, 
                                                       render_command_draw_t);
    draw->indices_to_draw  = index_count;
    draw->index_offset     = index_offset;
    draw->instance_count   = instance_count;
    draw->first_instance   = first_instance;

    command->header.command_type = RCT_DrawIndexed;
    command->data = draw;

    command_list->bound_image_count = 0;
}

/*
=============
r_cmd_blit_image
=============
*/

void
r_cmd_blit_image(render_command_list_t *command_list, 
                 image_t               *source_image, 
                 image_t               *dest_image, 
                 vec2_t                 source_offset, 
                 vec2_t                 source_blit_size, 
                 vec2_t                 dest_offset, 
                 vec2_t                 dest_blit_size)
{
    render_command_t *command   = s_renderer_get_next_command(command_list);
    render_command_blit_image_t *blit_image = c_arena_push_struct(&command_list->command_arena, 
                                                                  render_command_blit_image_t);
    blit_image->source_image  = source_image;
    blit_image->dest_image    = dest_image;
    blit_image->source_offset = source_offset;
    blit_image->source_size   = source_blit_size;
    blit_image->dest_offset   = dest_offset;
    blit_image->dest_size     = dest_blit_size;

    command->header.command_type = RCT_BlitImage;
    command->data = blit_image;
}

/*
=============
r_cmd_clear_image
=============
*/

// TODO(Sleepster): This function
void
r_cmd_clear_image()
{
    Expect(false, "This function is not implemented...\n");
}

/*
=============
r_cmd_dispatch_compute
=============
*/

void
r_cmd_dispatch_compute(render_command_list_t *command_list, u32 invoke_x, u32 invoke_y, u32 invoke_z)
{
    render_command_t *command   = s_renderer_get_next_command(command_list);
    render_command_dispatch_compute_t *dispath_compute = c_arena_push_struct(&command_list->command_arena, 
                                                                             render_command_dispatch_compute_t);
    dispath_compute->invoke_x = invoke_x;
    dispath_compute->invoke_y = invoke_y;
    dispath_compute->invoke_z = invoke_z;

    command->header.command_type = RCT_DispatchCompute;
    command->data                = dispath_compute;
}

/*
=============
r_cmd_present
=============
*/

// NOTE(Sleepster): 
// The cake is a lie. This does not actually present, but rather blits to the swapchain image,
// which is later presented when appropriate
//
// Also, when you call this command any commands after this is called, are an error.
void
r_cmd_present(render_command_list_t *command_list, image_t *presentation_source)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_present_frame_t *present = c_arena_push_struct(&command_list->command_arena, 
                                                                   render_command_present_frame_t);
    present->presentation_source = presentation_source;

    command->header.command_type = RCT_PresentFrame;
    command->data = present;
}
