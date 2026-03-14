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
#include <r_render_image.h>
#include <s_renderer.h>

void
s_renderer_state_init(renderer_state_t *renderer_state, void *render_context)
{
    ZeroStruct(*renderer_state);
    renderer_state->renderer_arena  = c_arena_create(MB(100));
    renderer_state->transient_arena = c_arena_create(MB(100));

    renderer_state->command_lists    = c_arena_push_array(&renderer_state->renderer_arena, render_command_list_t, MAX_COMMAND_LISTS);
    renderer_state->constant_buffers = c_arena_push_array(&renderer_state->renderer_arena, constant_buffer_t,     MAX_CONSTANT_BUFFERS);

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
#if 0
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
                image_create_info_t info = attachment->attachment->create_info;
                info.width  = window_size.x;
                info.height = window_size.y;

                s_renderer_image_destroy(renderer_state, attachment->attachment);

                *attachment->attachment = s_renderer_image_create(renderer_state, &info);
            }

            s_renderer_render_target_destroy(renderer_state, render_target);
            s_renderer_render_target_create(renderer_state, &render_target->create_info);
        }
    }
#endif

    renderer_state->last_window_size_generation += 1;
}

render_target_t*
s_renderer_render_target_create(renderer_state_t *renderer_state, render_target_create_info_t *create_info)
{
    render_target_t *result = null;
#if 0
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
#endif
    return(result);
}

void
s_renderer_render_target_destroy(renderer_state_t *renderer_state, render_target_t *render_target)
{
#if 0
    Assert(renderer_state->render_target_count > 0);

    vulkan_context_t *vulkan_context = (vulkan_context_t *)renderer_state->render_context;
    vk_backend_framebuffer_destroy(vulkan_context, render_target->framebuffer);
    vk_backend_renderpass_destroy(vulkan_context,  render_target->renderpass);

    render_target->ID = INVALID_ID;
    renderer_state->render_target_count--;
#endif
}

/////////////////////////
// FRAME GRAPH 
/////////////////////////

// NOTE(Sleepster):
// Realistically, we don't need these first two functions, we just have them in case you want
// to be able to do more when it comes to initialization like using DynamicArrays or memory
// arenas.

void
s_renderer_frame_graph_desc_init(render_frame_graph_desc_t *desc)
{
    ZeroStruct(*desc);
}

void
s_renderer_renderpass_desc_init(renderpass_desc_t *desc)
{
    ZeroStruct(*desc);
}

void
s_renderer_renderpass_attach_image(renderpass_desc_t           *renderpass, 
                                   image_t                     *image, 
                                   renderpass_attachment_type_t type, 
                                   clear_value_t                clear_value)
{
    renderpass_attachment_t *attachment = renderpass->attachments + renderpass->attachment_count++;
    attachment->image       = image;
    attachment->type        = type;
    attachment->clear_value = clear_value;
}

u32
s_renderer_frame_graph_attach_renderpass(render_frame_graph_desc_t *frame_graph, renderpass_desc_t *renderpass_desc)
{
    u32 ID = frame_graph->renderpass_count;
    renderpass_desc_t *new_desc = frame_graph->renderpass_descs + ID;

    frame_graph->renderpass_count++;
    memcpy(new_desc, renderpass_desc, sizeof(renderpass_desc_t));

    return(ID);
}

internal_api VkImageLayout
get_vulkan_layout_from_attachment(renderpass_attachment_t *attachment)
{
    image_t *image = attachment->image;

    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(image->create_info.format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32:
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

internal_api true_inline VkAttachmentLoadOp
get_load_operation(renderpass_attachment_type_t type)
{
    VkAttachmentLoadOp result = VK_ATTACHMENT_LOAD_OP_CLEAR;
    if(type == RenderpassAttachmentRead || type == RenderpassAttachmentReadWrite)
    {
        result = VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    return(result);
}


// NOTE(Sleepster): This is pointless right now, but we may want to expand the abilities here.
internal_api true_inline VkAttachmentStoreOp
get_store_operation(renderpass_attachment_type_t type)
{
    VkAttachmentStoreOp result = VK_ATTACHMENT_STORE_OP_STORE;
    return(result);
}

internal_api true_inline VkImageLayout
get_attachment_type(renderpass_attachment_t *attachment)
{
    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(attachment->image->create_info.format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32:
        {
            result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }break;
        case BMF_D32_SFLOAT_S8_UINT:
        case BMF_D24_SFLOAT_S8:
        case BMF_D32_SFLOAT:
        {
            result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }break;
    }

    return(result);
}

internal_api void
construct_frame_graph_renderpass(vulkan_context_t *vulkan_context, frame_graph_renderpass_t *renderpass, renderpass_desc_t *desc, u32 ID)
{
    renderpass->ID               = ID;
    renderpass->width            = desc->attachments->image->create_info.width;
    renderpass->height           = desc->attachments->image->create_info.height;
    renderpass->attachment_count = desc->attachment_count;

    image_t             image_attachments[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkImageLayout       initial_layouts[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkImageLayout       final_layouts[MAX_RENDER_TARGET_ATTACHMENTS]     = {};
    VkAttachmentLoadOp  load_operations[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkAttachmentStoreOp store_operations[MAX_RENDER_TARGET_ATTACHMENTS]  = {};
    VkImageLayout       attachment_types[MAX_RENDER_TARGET_ATTACHMENTS]  = {};

    for(u32 attachment_index = 0;
        attachment_index < desc->attachment_count;
        ++attachment_index)
    {
        renderpass_attachment_t *attachment_data = desc->attachments + attachment_index;
        image_t                 *image           = image_attachments + attachment_index;

        initial_layouts[attachment_index]  = VK_IMAGE_LAYOUT_UNDEFINED;
        final_layouts[attachment_index]    = get_vulkan_layout_from_attachment(attachment_data);
        load_operations[attachment_index]  = get_load_operation(attachment_data->type);
        store_operations[attachment_index] = get_store_operation(attachment_data->type);
        attachment_types[attachment_index] = get_attachment_type(attachment_data);

        renderpass->attachment_clear_values[attachment_index] = attachment_data->clear_value;
        memcpy(image, attachment_data->image, sizeof(image_t));
    }

    renderpass->renderpass_handle = vk_backend_renderpass_create(vulkan_context,
                                                                 image_attachments,
                                                                 desc->attachment_count,
                                                                 initial_layouts,
                                                                 final_layouts,
                                                                 load_operations,
                                                                 store_operations,
                                                                 attachment_types);

    renderpass->framebuffer_handle = vk_backend_framebuffer_create(vulkan_context,
                                                                   renderpass->renderpass_handle,
                                                                   image_attachments,
                                                                   desc->attachment_count,
                                                                   image_attachments[0].create_info.width,
                                                                   image_attachments[0].create_info.height);
}

// TODO(Sleepster): 
// Realistically when we do this for real, we'll want to construct a dependancy tree for each of the
// attachments to know when and where the attachments are actually being used in ways that require their
// store / load operations to be differ.
render_frame_graph_t
s_renderer_frame_graph_construct(renderer_state_t *renderer_state, render_frame_graph_desc_t *frame_graph_desc)
{
    render_frame_graph_t result = {};

    vulkan_context_t *vulkan_context = (vulkan_context_t*)renderer_state->render_context;
    for(u32 renderpass_desc_index = 0;
        renderpass_desc_index < frame_graph_desc->renderpass_count;
        ++renderpass_desc_index)
    {
        renderpass_desc_t        *renderpass_desc = frame_graph_desc->renderpass_descs + renderpass_desc_index;
        frame_graph_renderpass_t *renderpass      = result.renderpasses                + renderpass_desc_index;

        construct_frame_graph_renderpass(vulkan_context, renderpass, renderpass_desc, renderpass_desc_index);
    }

    return(result);
}

/////////////////////////
// COMMAND LISTS
/////////////////////////

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

#if 0
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
    cmd->header.command_type =  RCT_BlitRenderTarget;
    cmd->info                = *blit_info;
}
#endif

void
r_cmd_renderpass_begin(render_command_list_t *command_list, render_frame_graph_t *frame_graph, u32 renderpassID)
{
    render_command_begin_renderpass_t *cmd = (render_command_begin_renderpass_t*)(command_list->commands + command_list->command_count++);
    cmd->header.command_type = RCT_BeginRenderpass;
    cmd->frame_graph         = frame_graph;
    cmd->ID                  = renderpassID;
}

void
r_cmd_renderpass_end(render_command_list_t *command_list)
{
    render_command_end_renderpass_t *cmd = (render_command_end_renderpass_t*)(command_list->commands + command_list->command_count++);
    cmd->header.command_type = RCT_EndRenderpass;
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
