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

    renderer_state->render_context  = render_context;
}

image_t 
s_renderer_create_texture(renderer_state_t *render_state, image_create_info_t *image_create_info)
{
    image_t result = {};

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if(image_create_info->color_attachment)
    {
        usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    else if(image_create_info->depth_stencil_attachment)
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
    info.format         = bitmap_format_to_vulkan_format(image_create_info->image_type);
    info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.mip_count      = 1;
    info.sample_count   = 1;
    info.usage          = usage_flags;
    if(image_create_info->use_device_depth_format)
    {
        info.format = vulkan_context->depth_format;
    }

    result.vulkan_image = vk_backend_image_create(vulkan_context, &info);
    return(result);
}


void
s_renderer_update_image_data(void *backend_context, image_t *image)
{
    vk_backend_image_update_data((vulkan_context_t*)backend_context, &image->vulkan_image);
}

render_target_t
s_renderer_render_target_create(renderer_state_t *renderer_state, render_target_create_info_t *create_info)
{
    render_target_t result = {};

    image_t       *attachments      = create_info->attachments;
    VkImageLayout *initial_layouts  = create_info->attachment_initial_format;
    VkImageLayout *final_layouts    = create_info->attachment_final_format;
    u32           *load_operations  = create_info->attachment_load_operations;
    u32           *store_operations = create_info->attachment_store_operations;
    VkImageLayout *attachment_types = create_info->attachment_type;

    vulkan_context_t *context = (vulkan_context_t *)renderer_state->render_context;
    result.renderpass  = vk_backend_renderpass_create(context, 
                                                      attachments, 
                                                      create_info->attachment_count, 
                                                      initial_layouts,
                                                      final_layouts,
                                 (VkAttachmentLoadOp*)load_operations,
                                (VkAttachmentStoreOp*)store_operations,
                                                      attachment_types);

    result.framebuffer = vk_backend_framebuffer_create(context, 
                                                       result.renderpass, 
                                                       attachments, 
                                                       create_info->attachment_count, 
                                                       create_info->width, 
                                                       create_info->height);
    for(u32 value_index = 0;
        value_index < create_info->attachment_count;
        ++value_index)
    {
        result.clear_values[value_index] = create_info->attachment_clear_values[value_index];
    }

    return(result);
}

render_command_list_t*
s_renderer_get_command_list(renderer_state_t *renderer_state)
{
    render_command_list_t *result = null;
    result = renderer_state->command_lists + renderer_state->command_list_count++;
    Assert(renderer_state->command_list_count < MAX_COMMAND_LISTS);

    return(result);
}

internal_api true_inline void
r_cmd_add_to_list(render_command_list_t *command_list, render_instance_t *instance, constant_buffer_t *buffer, render_command_type_t type)
{
    render_command_t *command = command_list->commands + command_list->command_count;
    command->command_type =  type;
    command->buffer       = *buffer;
    command->instance     = *instance;
}
