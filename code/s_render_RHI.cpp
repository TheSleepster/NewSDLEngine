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

    renderer_state->command_lists    = c_arena_push_array(&renderer_state->renderer_arena, render_command_list_t, MAX_COMMAND_LISTS);
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

            renderpass->create_info.render_width = renderer_state->window_size.x;
            renderpass->create_info.render_width = renderer_state->window_size.y;
            renderpass->ID = s_renderer_build_renderpass(renderer_state, &renderpass->create_info);
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
s_renderer_vulkan_attachment_type
=============
*/

internal_api true_inline VkImageLayout
s_renderer_vulkan_attachment_type(renderpass_attachment_t *attachment)
{
    image_t *image = attachment->image;

    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(image->create_info.format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32_SRGB:
        case BMF_RGBA32_UNORM:
        case BMF_BGRA32_UNORM:
        {
            result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }break;
        case BMF_D32_SFLOAT_S8_UINT:
        case BMF_D24_SFLOAT_S8:
        case BMF_D32_SFLOAT:
        {
            // NOTE(Sleepster): D32 is still a depth stencil anyway 
            result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }break;
    }

    return(result);
}

/*
=============
s_renderer_vulkan_load_op
=============
*/

internal_api true_inline VkAttachmentLoadOp
s_renderer_vulkan_load_op(renderpass_attachment_load_operation_t load_op)
{
    Assert(load_op != RenderpassAttachmentLoadOperationInvalid);

    VkAttachmentLoadOp result = VK_ATTACHMENT_LOAD_OP_CLEAR;
    if(load_op == RenderpassAttachmentLoadOperationLoad)
    {
        result = VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    return(result);
}

/*
=============
s_renderer_vulkan_store_op
=============
*/

// NOTE(Sleepster): This is pointless right now, but we may want to expand the abilities here.
internal_api true_inline VkAttachmentStoreOp
s_renderer_vulkan_store_op(renderpass_attachment_store_operation_t store_op)
{
    VkAttachmentStoreOp result = VK_ATTACHMENT_STORE_OP_STORE;
    if(store_op == RenderpassAttachmentStoreOperationDontCare)
    {
        result = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    return(result);
}

/*
=============
is_depth_attachment_valid
=============
*/

internal_api true_inline bool8
is_depth_attachment_valid(renderpass_attachment_t *depth_attachment)
{
    bool8 result = true;
    if(depth_attachment->image == null)
    {
        result = false;
    }

    return(result);
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
    
    renderpass->render_width           = renderpass_desc->render_width;
    renderpass->render_height          = renderpass_desc->render_height;
    renderpass->color_attachment_count = renderpass_desc->color_attachment_count;
    renderpass->resize_with_window     = renderpass_desc->resize_with_window;
    renderpass->ID                     = renderer_state->renderpass_count++;

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

    vulkan_context_t *context = (vulkan_context_t *)renderer_state->render_context;

    // TODO(Sleepster): 
    // We obviously don't want to put raw vulkan code in here because that's stupid... 
    // however, we have no choice right now
    image_t             image_attachments[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkImageLayout       initial_layouts[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkImageLayout       final_layouts[MAX_RENDER_TARGET_ATTACHMENTS]     = {};
    VkAttachmentLoadOp  load_operations[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkAttachmentStoreOp store_operations[MAX_RENDER_TARGET_ATTACHMENTS]  = {};
    VkImageLayout       attachment_types[MAX_RENDER_TARGET_ATTACHMENTS]  = {};

    u32 attachment_count = renderpass_desc->color_attachment_count;
    for(u32 color_attachment_index = 0;
        color_attachment_index < renderpass_desc->color_attachment_count;
        ++color_attachment_index)
    {
        renderpass_attachment_t *color_attachment = renderpass_desc->color_attachments + color_attachment_index;
        Assert(color_attachment->load_operation  != RenderpassAttachmentLoadOperationInvalid);
        Assert(color_attachment->store_operation != RenderpassAttachmentStoreOperationInvalid);

        VkImageLayout initial_layout = color_attachment->image->vulkan_image.layout;
        VkImageLayout final_layout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        image_attachments[color_attachment_index] = *color_attachment->image;
        initial_layouts[color_attachment_index]   = initial_layout;
        final_layouts[color_attachment_index]     = final_layout;
        load_operations[color_attachment_index]   = s_renderer_vulkan_load_op(color_attachment->load_operation);
        store_operations[color_attachment_index]  = s_renderer_vulkan_store_op(color_attachment->store_operation);
        attachment_types[color_attachment_index]  = s_renderer_vulkan_attachment_type(color_attachment);

        renderpass->attachment_clear_values[color_attachment_index] = color_attachment->clear_value;
    }

   renderpass->has_depth_stencil_attachment = is_depth_attachment_valid(&renderpass_desc->depth_stencil_attachment);
    if(renderpass->has_depth_stencil_attachment)
    {
        renderpass_attachment_t *depth_stencil_attachment = &renderpass_desc->depth_stencil_attachment;
        u32 attachment_index = renderpass_desc->color_attachment_count;
        Assert(attachment_index < MAX_RENDER_TARGET_ATTACHMENTS);

        ++attachment_count;

        VkImageLayout initial_layout = depth_stencil_attachment->image->vulkan_image.layout;
        VkImageLayout final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        image_attachments[attachment_index] = *depth_stencil_attachment->image;
        initial_layouts[attachment_index]   = initial_layout;
        final_layouts[attachment_index]     = final_layout;
        load_operations[attachment_index]   = s_renderer_vulkan_load_op(depth_stencil_attachment->load_operation);
        store_operations[attachment_index]  = s_renderer_vulkan_store_op(depth_stencil_attachment->store_operation);
        attachment_types[attachment_index]  = s_renderer_vulkan_attachment_type(depth_stencil_attachment);

        renderpass->attachment_clear_values[attachment_index] = depth_stencil_attachment->clear_value;
    }

    renderpass->renderpass_handle = vk_backend_renderpass_create(context,
                                                                 image_attachments,
                                                                 attachment_count,
                                                                 initial_layouts,
                                                                 final_layouts,
                                                                 load_operations,
                                                                 store_operations,
                                                                 attachment_types);
    renderpass->framebuffer_handle = vk_backend_framebuffer_create(context,
                                                                   renderpass->renderpass_handle, 
                                                                   image_attachments,
                                                                   attachment_count,
                                                                   renderpass->render_width,
                                                                   renderpass->render_height);
    renderpass->total_attachment_count = attachment_count;

    return(result);
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
s_renderer_vertex_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, void *data, u32 size)
{
    render_buffer_t result = s_renderer_render_buffer_create(renderer_state, data, size, RenderBufferType_VertexBuffer, memory_type);
    return(result);
}

/*
=============
s_renderer_index_buffer_create
=============
*/

true_inline render_buffer_t
s_renderer_index_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, void *data, u32 size)
{
    render_buffer_t result = s_renderer_render_buffer_create(renderer_state, data, size, RenderBufferType_IndexBuffer, memory_type);
    return(result);
}

/*
=============
s_renderer_render_buffer_create
=============
*/

render_buffer_t
s_renderer_render_buffer_create(renderer_state_t           *renderer_state, 
                                void                       *data, 
                                u32                         size, 
                                render_buffer_type_t        buffer_type, 
                                render_buffer_memory_type_t allocation_type)
{
    render_buffer_t result = {};
    result.type            = buffer_type;
    result.size            = size;
    result.allocation_type = allocation_type;

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

    if(allocation_type == RenderBufferAllocationTypeMapped)
    {
        allocation_flags = VULKAN_MEMORY_USAGE_CPU_TO_GPU; 
    }
    else if(allocation_type == RenderBufferAllocationTypeGPUOnly)
    {
        allocation_flags = VULKAN_MEMORY_USAGE_GPU_ONLY;
    }

    result.buffer = vk_backend_buffer_create((vulkan_context_t *)renderer_state->render_context, 
                                             size,
                                             usage_flags,
                                             allocation_flags);
    if(data != null && size > 0)
    {
        vk_backend_buffer_copy_data((vulkan_context_t*)renderer_state->render_context, &result.buffer, data, size, 0);
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
s_renderer_command_list_init(render_command_list_t *list)
{
    if(list->is_initialized == false)
    {
        list->transient_arena = c_arena_create(MB(10));
        list->command_arena   = c_arena_create(MB(30));
    }
    else
    {
        c_arena_reset(&list->transient_arena);
        c_arena_reset(&list->command_arena);
    }

    list->commands            = c_arena_push_array(&list->command_arena, render_command_t, MAX_RENDER_COMMANDS);
    list->active_render_state = g_pipeline_default_state_key;
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

    s_renderer_command_list_init(result);
    Assert(result->is_initialized == true);
    Assert(result->transient_arena.is_initialized == true);
    Assert(result->command_arena.is_initialized   == true);

    return(result);
}

internal_api true_inline render_command_t*
s_renderer_get_next_command(render_command_list_t *command_list)
{
    render_command_t *result = null;
    result = command_list->commands + command_list->command_count++;

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
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_begin_renderpass_t *begin_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                               render_command_begin_renderpass_t);
    begin_renderpass->ID = renderpassID;

    command->header.command_type = RCT_BeginRenderpass;
    command->data                = begin_renderpass;
}


/*
=============
r_cmd_renderpass_end
=============
*/

void
r_cmd_renderpass_end(render_command_list_t *command_list)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    command->header.command_type = RCT_EndRenderpass;
}


/*
=============
r_cmd_draw_rectangle
=============
*/

void
r_cmd_draw_rectangle(render_command_list_t *command_list, 
                     vec2_t                 position, 
                     vec2_t                 size, 
                     vec4_t                 render_color, 
                     float32                rotation)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_draw_rectangle_t *draw_rect = c_arena_push_struct(&command_list->command_arena, 
                                                                      render_command_draw_rectangle_t);

    draw_rect->quad_data.position     = position;
    draw_rect->quad_data.size         = size;
    draw_rect->quad_data.render_color = render_color;
    draw_rect->quad_data.rotation     = rotation;

    command->header.command_type = RCT_DrawRectangle;
    command->data                = draw_rect;
}

/*
=============
r_cmd_draw_bitmap
=============
*/

void
r_cmd_draw_bitmap(render_command_list_t *command_list, 
                  vec2_t                 position, 
                  vec2_t                 size, 
                  vec4_t                 render_color, 
                  float32                rotation,
                  asset_handle_t         bitmap_handle)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_draw_bitmap_t *draw_bitmap = c_arena_push_struct(&command_list->command_arena, 
                                                                     render_command_draw_bitmap_t);
    
    draw_bitmap->quad_data.position     = position;
    draw_bitmap->quad_data.size         = size;
    draw_bitmap->quad_data.render_color = render_color;
    draw_bitmap->quad_data.rotation     = rotation;
    draw_bitmap->bitmap                 = bitmap_handle;

    command->data = draw_bitmap;
    command->header.command_type = RCT_DrawBitmap;
}

/*
=============
r_cmd_renderpass_end
=============
*/

void
r_cmd_use_shader_program(render_command_list_t *command_list, asset_handle_t program)
{
    render_command_t *command = s_renderer_get_next_command(command_list);
    render_command_bind_shader_t *bind_shader = c_arena_push_struct(&command_list->command_arena, 
                                                                     render_command_bind_shader_t);

    bind_shader->shader = program;

    command->header.command_type = RCT_BindShader;
    command->data = bind_shader;
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
r_cmd_update_push_constants
=============
*/

void
r_cmd_update_buffer_contents(render_command_list_t *command_list, uniform_constant_buffer_t *buffer, void *data, u32 data_size)
{
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_update_uniform_constant_buffer_t *update_buffer_contents = c_arena_push_struct(&command_list->command_arena, 
                                                                                                   render_command_update_uniform_constant_buffer_t);
    update_buffer_contents->constant_data_size = data_size;

    // TODO(Sleepster): Abstract this... 
    vulkan_context_t *vulkan_context = (vulkan_context_t*)command_list->renderer_state->render_context;
    update_buffer_contents->uniform_hash_index         = buffer->uniform_hash_index;
    update_buffer_contents->backend_uniform_buffer_ptr = vk_backend_append_uniform_constant_buffer_data(vulkan_context, 
                                                                                                        data, 
                                                                                                        data_size, 
                                                                                                       &buffer->offset);
    buffer->mapped_data = update_buffer_contents->backend_uniform_buffer_ptr;
    buffer->size        = data_size;

    command->header.command_type = RCT_UpdateUniformConstantBuffer;
    command->data                = update_buffer_contents;
}

/*
=============
r_cmd_bind_texture
=============
*/

void
r_cmd_bind_texture(render_command_list_t *command_list, asset_handle_t *asset_handle)
{
    Assert(asset_handle->type == AT_Bitmap);
    render_command_t *command  = s_renderer_get_next_command(command_list);
    render_command_bind_texture_t *bind_texture = c_arena_push_struct(&command_list->command_arena, 
                                                                       render_command_bind_texture_t);
    bind_texture->texture = &asset_handle->texture->gpu_data;

    command->header.command_type = RCT_BindTexture;
    command->data = bind_texture;
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
}

/*
=============
r_cmd_present
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
