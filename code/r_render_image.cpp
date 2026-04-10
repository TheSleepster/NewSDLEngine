/* ========================================================================
   $File: r_render_image.cpp $
   $Date: March 05 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <r_render_image.h>
#include <s_render_RHI.h>

// TODO(Sleepster): 
// This whole file is dumb. This structure is meant to be an abstraction above vulkan images for multiple backend support... 
// Instead, it's just an extension of the already Vulkan vulkan_image... This is bad. Fix this.

VkFormat
s_renderer_bitmap_format_to_vulkan_format(u32 bitmap_format)
{
    VkFormat result = VK_FORMAT_R8G8B8_SRGB;
    switch(bitmap_format)
    {
        case BMF_R8:                 {result = VK_FORMAT_R8_SRGB;           }break;
        case BMF_G8:                 {result = VK_FORMAT_R8_SRGB;           }break;
        case BMF_B8:                 {result = VK_FORMAT_R8_SRGB;           }break;
        case BMF_RGB24_SRGB:         {result = VK_FORMAT_R8G8B8_SRGB;       }break;
        case BMF_RGB24_UNORM:        {result = VK_FORMAT_R8G8B8_UNORM;      }break;
        case BMF_RGBA32_SRGB:        {result = VK_FORMAT_R8G8B8A8_SRGB;     }break;
        case BMF_RGBA32_UNORM:       {result = VK_FORMAT_R8G8B8A8_UNORM;    }break;
        case BMF_BGRA32_UNORM:       {result = VK_FORMAT_B8G8R8A8_UNORM;    }break;
        case BMF_D32_SFLOAT:         {result = VK_FORMAT_D32_SFLOAT;        }break;
        case BMF_D32_SFLOAT_S8_UINT: {result = VK_FORMAT_D32_SFLOAT_S8_UINT;}break;
    }

    return(result);
}

VkImageUsageFlags
s_renderer_image_usage_flags_from_image_format(image_create_info_t *image_create_info)
{
    VkImageUsageFlags result = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    switch(image_create_info->format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24_SRGB:
        case BMF_RGB24_UNORM:
        case BMF_BGRA32_UNORM:
        case BMF_RGBA32_UNORM:
        case BMF_RGBA32_SRGB:
        {
            result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }break;
        case BMF_D32_SFLOAT_S8_UINT:
        case BMF_D24_SFLOAT_S8:
        case BMF_D32_SFLOAT:
        {
            result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }break;
    }
    
    return(result);
}

internal_api bool8 
sampler_info_is_valid(image_create_info_t *create_info)
{
    bool8 result = true;
    if(create_info->sampler_info.filtering == ImageFilterType_Invalid ||
       create_info->sampler_info.wrapu     == ImageWrapping_Invalid   || 
       create_info->sampler_info.wrapv     == ImageWrapping_Invalid)
    {
        result = false;
    }

    return(result);
}

internal_api VkFilter
sampler_filter_type_to_vk_filter(render_image_filter_type_t filter)
{
    VkFilter result;
    switch(filter)
    {
        case ImageFilterType_Nearest:
        {
            result = VK_FILTER_NEAREST;
        }break;
        case ImageFilterType_Linear:
        {
            result = VK_FILTER_LINEAR;
        }break;
        default:
        {
            result = VK_FILTER_NEAREST;
            InvalidCodePath;
        }break;
    }

    return(result);
}

image_t 
s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info)
{
    image_t result = {};
    result.create_info = *image_create_info;

    VkImageUsageFlags usage_flags = s_renderer_image_usage_flags_from_image_format(image_create_info);
    vulkan_context_t *vulkan_context = (vulkan_context_t*)render_state->render_context;
    
    // NOTE(Sleepster): Only 2D images
    vulkan_image_info_t info = {};
    info.type           = VK_IMAGE_TYPE_2D;
    info.width          = image_create_info->width;
    info.height         = image_create_info->height;
    info.data           = image_create_info->data;
    info.format         = s_renderer_bitmap_format_to_vulkan_format(image_create_info->format);
    info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.mip_count      = 1;
    info.sample_count   = 1;
    info.usage          = usage_flags;

    vulkan_sampler_info_t sampler_info;
    if(sampler_info_is_valid(image_create_info))
    {
        // TODO(Sleepster): For now compare operations are disabled. I don't want to deal with them right now... 
        sampler_create_info_t *image_sampler = &image_create_info->sampler_info;
        VkFilter filter = sampler_filter_type_to_vk_filter(image_sampler->filtering);

        sampler_info = {
            .anisotropy_enabled = image_sampler->anisotropy_enabled,
            .max_anisotropy     = image_sampler->max_anisotropy,
            .compare_enabled    = false,
            .compare_operation  = 0,
            .min_filter         = filter,
            .mag_filter         = filter,
            .wrapu              = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .wrapv              = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,

            .use_normalized_coordinates = image_sampler->use_normalized_coordinates
        };

        info.sampler_info = &sampler_info;
    }

    // TODO(Sleepster): Maybe we need this???
#if 0
    // NOTE(Sleepster): Override the depth format... 
    if(usage_flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
       info.format = vulkan_context->depth_format;
    }
#endif

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


