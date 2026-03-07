/* ========================================================================
   $File: r_render_image.cpp $
   $Date: March 05 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <r_render_image.h>
#include <s_renderer.h>

image_t 
s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info)
{
    image_t result = {};
    result.create_jnfo = *image_create_info;

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    switch(image_create_info->format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32:
        {
            usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }break;
        case BMF_D32_SFLOAT:
        {
            usage_flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }break;
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
    if(usage_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
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


