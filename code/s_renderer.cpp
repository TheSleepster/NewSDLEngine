/* ========================================================================
   $File: s_renderer.cpp $
   $Date: February 22 2026 05:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <c_log.h>

#include <vk_backend_core.h>
#include <s_asset_manager.h>
#include <s_renderer.h>

internal_api VkFormat
bitmap_format_to_vulkan_format(u32 bitmap_format)
{
    VkFormat result = VK_FORMAT_R8G8B8_SRGB;
    switch(bitmap_format)
    {
        case BMF_R8:         {result = VK_FORMAT_R8_SRGB;      }break;
        case BMF_G8:         {result = VK_FORMAT_R8_SRGB;      }break;
        case BMF_B8:         {result = VK_FORMAT_R8_SRGB;      }break;
        case BMF_RGB24:      {result = VK_FORMAT_R8G8B8_SRGB;  }break;
        case BMF_RGBA32:     {result = VK_FORMAT_R8G8B8A8_SRGB;}break;
        case BMF_D32_SFLOAT: {result = VK_FORMAT_D32_SFLOAT;   }break;
    }

    return(result);
}

void
s_renderer_state_init(renderer_state_t *renderer_state, void *render_context)
{
    ZeroStruct(*renderer_state);
    renderer_state->renderer_arena  = c_arena_create(MB(100));
    renderer_state->transient_arena = c_arena_create(MB(100));

    renderer_state->command_lists    = c_arena_push_array(&renderer_state->renderer_arena, render_command_list_t, MAX_COMMAND_LISTS);
    renderer_state->constant_buffers = c_arena_push_array(&renderer_state->renderer_arena, constant_buffer_t,     MAX_CONSTANT_BUFFERS);

    for(u32 render_target_index = 0;
        render_target_index < MAX_RENDER_TARGETS;
        ++render_target_index)
    {
        render_target_t *render_target = renderer_state->render_targets + render_target_index;
        render_target->ID = INVALID_ID;
    }

    renderer_state->render_context  = render_context;
}

void
s_renderer_handle_window_resize(renderer_state_t *renderer_state, vec2_t window_size)
{
    vulkan_context_t *vulkan_context = (vulkan_context_t *)renderer_state->render_context;
    vk_backend_handle_window_resize(vulkan_context, window_size);

    renderer_state->window_size                     = window_size;
    renderer_state->last_window_size_generation     = renderer_state->current_window_size_generation;
    renderer_state->current_window_size_generation += 1;
}

void
s_renderer_resize_render_targets(renderer_state_t *renderer_state, vec2_t window_size)
{
    for(u32 render_target_index = 0;
        render_target_index < renderer_state->render_target_count;
        ++render_target_index)
    {
        render_target_t *render_target = renderer_state->render_targets + render_target_index;
        if(render_target->resize_with_window)
        {
            render_target->create_info.width  = window_size.x;
            render_target->create_info.height = window_size.y;
            for(u32 attachment_index = 0;
                attachment_index < render_target->attachment_count;
                ++attachment_index)
            {
                render_target_attachment_info_t *attachment = render_target->attachment_info + attachment_index;
                image_create_info_t info = attachment->attachment->create_jnfo;
                info.width  = window_size.x;
                info.height = window_size.y;

                s_renderer_image_destroy(renderer_state, attachment->attachment);

                *attachment->attachment = s_renderer_image_create(renderer_state, &info);
            }

            s_renderer_render_target_destroy(renderer_state, render_target);
            s_renderer_render_target_create(renderer_state, &render_target->create_info);
        }
    }

    renderer_state->last_window_size_generation += 1;
}

image_t 
s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info)
{
    image_t result = {};
    result.create_jnfo = *image_create_info;

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if(image_create_info->image_type == IMAGE_TYPE_ColorAttachment)
    {
        usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    else if(image_create_info->image_type == IMAGE_TYPE_DepthStencilAttachment)
    {
        usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    vulkan_context_t *vulkan_context = (vulkan_context_t*)render_state->render_context;
    
    // NOTE(Sleepster): Only 2D images
    vulkan_image_info_t info = {};
    info.type           = VK_IMAGE_TYPE_2D;
    info.width          = image_create_info->width;
    info.height         = image_create_info->height;
    info.data           = image_create_info->data;
    info.format         = vulkan_context->swapchain_format.format;
    info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.mip_count      = 1;
    info.sample_count   = 1;
    info.usage          = usage_flags;

    // NOTE(Sleepster): Override the depth format... 
    if(image_create_info->image_type == IMAGE_TYPE_DepthStencilAttachment)
    {
        info.format = vulkan_context->depth_format;
    }

    result.vulkan_image = vk_backend_image_create(vulkan_context, &info);
    return(result);
}

void
s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image)
{
    vk_backend_image_destroy((vulkan_context_t *)renderer_state->render_context, &image->vulkan_image);
}

void
s_renderer_image_update_data(void *backend_context, image_t *image)
{
    vk_backend_image_update_data((vulkan_context_t*)backend_context, &image->vulkan_image);
}

render_target_t*
s_renderer_render_target_create(renderer_state_t *renderer_state, render_target_create_info_t *create_info)
{
    render_target_t *result = null;
    for(u32 render_target_index = 0;
        render_target_index < MAX_RENDER_TARGETS;
        ++render_target_index)
    {
        render_target_t *target = renderer_state->render_targets + render_target_index;
        if(target->ID == INVALID_ID)
        {
            result     = target;
            result->ID = render_target_index;

            Assert(renderer_state->render_target_count + 1 <= MAX_RENDER_TARGETS);
            renderer_state->render_target_count++;

            break;
        }
    }
    Assert(result);

    result->attachment_count   =  create_info->attachment_count;
    result->create_info        = *create_info;
    result->resize_with_window =  create_info->resize_with_window;

    image_t             image_attachments[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkImageLayout       initial_layouts[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkImageLayout       final_layouts[MAX_RENDER_TARGET_ATTACHMENTS]     = {};
    VkAttachmentLoadOp  load_operations[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkAttachmentStoreOp store_operations[MAX_RENDER_TARGET_ATTACHMENTS]  = {};
    VkImageLayout       attachment_types[MAX_RENDER_TARGET_ATTACHMENTS]  = {};

    u32 current_attachment_index = 0;
    bool8 color_buffer_found = false;
    for(u32 attachment_index = 0;
        attachment_index < create_info->attachment_count;
        ++attachment_index)
    {
        render_target_attachment_info_t *info = (create_info->attachments + attachment_index);
        result->attachment_info[attachment_index] = *info;

        vec4_t *clear_color = &info->clear_value.clear_color.float_color;

        Assert(info->attachment->vulkan_image.width  == create_info->width);
        Assert(info->attachment->vulkan_image.height == create_info->height);

        image_attachments[attachment_index]   = *info->attachment;
        initial_layouts[attachment_index]     = (VkImageLayout)info->initial_layout;
        final_layouts[attachment_index]       = (VkImageLayout)info->final_layout;
        load_operations[attachment_index]     = (VkAttachmentLoadOp)info->load_operation;
        store_operations[attachment_index]    = (VkAttachmentStoreOp)info->store_operation;
        attachment_types[attachment_index]    = (VkImageLayout)info->attachment_type;
        if(info->attachment_type == IMAGE_TYPE_ColorAttachment && color_buffer_found == false)
        {
            result->primary_color_buffer = result->attachment_info[attachment_index].attachment;
            color_buffer_found = true;
        }
        else if(info->attachment_type == IMAGE_TYPE_DepthStencilAttachment || 
                info->attachment_type == IMAGE_TYPE_DepthStencilReadOnly)
        {
            result->depth_buffer = result->attachment_info[attachment_index].attachment;
        }

        // TODO(Sleepster): This is a union. Fix this and make it so that the owner of the clear color is the attachment. 
        VkClearValue *clear_value = &result->clear_values[current_attachment_index];
        if(info->attachment_type == IMAGE_TYPE_ColorAttachment)
        {
            clear_value->color.float32[0]     = clear_color->x;
            clear_value->color.float32[1]     = clear_color->y;
            clear_value->color.float32[2]     = clear_color->z;
            clear_value->color.float32[3]     = clear_color->w;

            ++current_attachment_index;
        }
        else if(info->attachment_type == IMAGE_TYPE_DepthStencilAttachment ||
                info->attachment_type == IMAGE_TYPE_DepthStencilReadOnly)
        {
            clear_value->depthStencil.depth   = info->clear_value.clear_depth;
            ++current_attachment_index;

            VkClearValue *stencil_value = &result->clear_values[current_attachment_index];
            stencil_value->depthStencil.stencil = info->clear_value.clear_stencil;
            ++current_attachment_index;
        }

        Assert(result->clear_values[attachment_index].color.float32[0] <= 1.0f);
        Assert(result->clear_values[attachment_index].color.float32[1] <= 1.0f);
        Assert(result->clear_values[attachment_index].color.float32[2] <= 1.0f);
        Assert(result->clear_values[attachment_index].color.float32[3] <= 1.0f);
    }

    vulkan_context_t *context = (vulkan_context_t *)renderer_state->render_context;
    result->renderpass  = vk_backend_renderpass_create(context, 
                                                      image_attachments, 
                                                      create_info->attachment_count, 
                                                      initial_layouts,
                                                      final_layouts,
                                                      load_operations,
                                                      store_operations,
                                                      attachment_types);

    result->framebuffer = vk_backend_framebuffer_create(context, 
                                                       result->renderpass, 
                                                       image_attachments, 
                                                       create_info->attachment_count, 
                                                       create_info->width, 
                                                       create_info->height);
    return(result);
}

void
s_renderer_render_target_destroy(renderer_state_t *renderer_state, render_target_t *render_target)
{
    Assert(renderer_state->render_target_count > 0);

    vulkan_context_t *vulkan_context = (vulkan_context_t *)renderer_state->render_context;
    vk_backend_framebuffer_destroy(vulkan_context, render_target->framebuffer);
    vk_backend_renderpass_destroy(vulkan_context,  render_target->renderpass);

    render_target->ID = INVALID_ID;
    renderer_state->render_target_count--;
}

internal_api void
s_renderer_command_list_init(render_command_list_t *list)
{
    if(list->is_initialized == false)
    {
        list->transient_arena = c_arena_create(MB(10));
        list->command_arena   = c_arena_create((sizeof(render_command_t) * MAX_RENDER_COMMANDS) * 2);
        list->commands        = c_arena_push_array(&list->command_arena, render_command_t, MAX_RENDER_COMMANDS);
    }
    else
    {
        c_arena_reset(&list->transient_arena);
    }
    list->is_initialized = true;
}

render_command_list_t*
s_renderer_get_command_list(renderer_state_t *renderer_state)
{
    render_command_list_t *result = null;
    result = renderer_state->command_lists + renderer_state->command_list_count++;
    Assert(renderer_state->command_list_count < MAX_COMMAND_LISTS);

    s_renderer_command_list_init(result);
    Assert(result->is_initialized == true);
    Assert(result->transient_arena.is_initialized == true);

    return(result);
}

void
r_cmd_bind_render_target(render_command_list_t *command_list, render_target_t *render_target)
{
    Assert(!command_list->presenting);

    render_command_bind_render_target_t *bind_target = (render_command_bind_render_target_t*)(command_list->commands + command_list->command_count++);
    bind_target->render_target         = render_target;
    bind_target->header.command_type   = RCT_BindRenderTarget;

    command_list->active_render_target = render_target;
}

// NOTE(Sleepster): You don't need to call this if the render target's load operation is a clear.
void
r_cmd_clear_render_target(render_command_list_t *command_list, render_target_t *render_target)
{
    render_command_clear_render_target_t *clear_target = (render_command_clear_render_target_t*)(command_list->commands + command_list->command_count++);
    clear_target->render_target       = render_target;
    clear_target->header.command_type = RCT_ClearRenderTarget;
}

void
r_cmd_begin_render_group(render_command_list_t *command_list)
{
    render_command_begin_render_group_t *begin_rendergroup = (render_command_begin_render_group_t*)(command_list->commands + command_list->command_count++);
    begin_rendergroup->header.command_type = RCT_BeginRenderGroup;
}

void
r_cmd_end_render_group(render_command_list_t *command_list)
{
    render_command_end_render_group_t *end_rendergroup = (render_command_end_render_group_t*)(command_list->commands + command_list->command_count++);
    end_rendergroup->header.command_type = RCT_EndRenderGroup;
}

void
r_cmd_blit_render_target(render_command_list_t *command_list, render_command_blit_info_t *blit_info)
{
    render_command_blit_render_target_t *cmd = (render_command_blit_render_target_t*)(command_list->commands + command_list->command_count++);
    cmd->header.command_type =  RCT_BlitToRenderTarget;
    cmd->info                = *blit_info;
}

void
r_cmd_draw_rectangle(render_command_list_t *command_list, 
                     vec2_t                 position, 
                     vec2_t                 size, 
                     vec4_t                 render_color, 
                     float32                rotation)
{
    render_command_draw_rectangle_t *draw_rect = (render_command_draw_rectangle_t*)(command_list->commands + command_list->command_count++);
    draw_rect->header.command_type    = RCT_DrawRectangle;
    draw_rect->quad_data.position     = position;
    draw_rect->quad_data.size         = size;
    draw_rect->quad_data.render_color = render_color;
    draw_rect->quad_data.rotation     = rotation;
}

void
r_cmd_draw_bitmap(render_command_list_t *command_list, 
                  vec2_t                 position, 
                  vec2_t                 size, 
                  vec4_t                 render_color, 
                  float32                rotation,
                  asset_handle_t         bitmap_handle)
{
    render_command_draw_bitmap_t *draw_bitmap = (render_command_draw_bitmap_t*)(command_list->commands + command_list->command_count++);
    draw_bitmap->header.command_type    = RCT_DrawBitmap;
    draw_bitmap->quad_data.position     = position;
    draw_bitmap->quad_data.size         = size;
    draw_bitmap->quad_data.render_color = render_color;
    draw_bitmap->quad_data.rotation     = rotation;
    draw_bitmap->bitmap                 = bitmap_handle;
}

// NOTE(Sleepster): The cake is a lie. This does not actually present, but rather blits to the swapchain image
void
r_cmd_present(render_command_list_t *command_list)
{
    render_command_present_frame_t *present = (render_command_present_frame_t*)(command_list->commands + command_list->command_count++);
    present->header.command_type = RCT_PresentFrame;
    present->presentation_target  = command_list->active_render_target;

    command_list->active_render_target = null;
    command_list->presenting = true;
}
