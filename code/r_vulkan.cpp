/* ========================================================================
   $File: r_vulkan.cpp $
   $Date: December 14 2025 04:50 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_globals.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <r_vulkan.h>

// TODO(Sleepster): Every structure that requires an AllocArray should just have a memory arena.

VKAPI_ATTR VkBool32 VKAPI_CALL
Vk_debug_log_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                      VkDebugUtilsMessageTypeFlagsEXT             message_type,
                      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                      void*                                       user_data)
{
    switch(message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            log_fatal(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            log_warning(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            log_info(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            log_trace(callback_data->pMessage);
        }break;
    }
return VK_FALSE;
}

////////////////////////////
// VULKAN IMAGE 
////////////////////////////

VkImageView
r_vulkan_image_view_create(vulkan_render_context_t *render_context,
                           VkFormat                 format, 
                           vulkan_image_data_t     *image_data,
                           u32                      view_aspect_flags)
{
    VkImageView result = {};
    VkImageViewCreateInfo view_create_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = image_data->handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
        .subresourceRange = {
            .aspectMask     = view_aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        },
    };
    VkAssert(vkCreateImageView(render_context->rendering_device.logical_device, &view_create_info, render_context->allocators, &result));

    return(result);
}

void
r_vulkan_image_destroy(vulkan_render_context_t *render_context,
                       vulkan_image_data_t     *image_data)
{
    if(image_data->view)
    {
        vkDestroyImageView(render_context->rendering_device.logical_device, 
                           image_data->view, 
                           render_context->allocators);
        image_data->view = 0;
    }
    if(image_data->memory)
    {
        vkFreeMemory(render_context->rendering_device.logical_device, 
                     image_data->memory, 
                     render_context->allocators);
        image_data->memory = 0;
    }
    if(image_data->handle)
    {
        vkDestroyImage(render_context->rendering_device.logical_device, 
                       image_data->handle, 
                       render_context->allocators);
        image_data->handle = 0;
    }
}

vulkan_image_data_t
r_vulkan_image_create(vulkan_render_context_t *render_context,
                      VkImageType              image_type,
                      u32                      width,
                      u32                      height,
                      VkFormat                 format,
                      VkImageTiling            tiling,
                      VkImageUsageFlags        usage_flags,
                      VkMemoryPropertyFlags    memory_flags,
                      VkImageAspectFlags       view_aspect_flags,
                      u32                      mip_levels,
                      bool32                   create_view)
{
    vulkan_image_data_t result = {};
    result.width  = width;
    result.height = height;

    VkImageCreateInfo image_create_info = {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = {
            .width  = width,
            .height = height,
            .depth  = 1
        },
        .mipLevels     = mip_levels,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage_flags,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE
    };

    VkAssert(vkCreateImage(render_context->rendering_device.logical_device, 
                          &image_create_info, 
                           render_context->allocators, 
                          &result.handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(render_context->rendering_device.logical_device, result.handle, &memory_requirements);
    s32 memory_index = r_vulkan_find_memory_index(render_context, memory_requirements.memoryTypeBits, memory_flags);
    if(memory_index == -1)
    {
        log_error("Requested memory type could not be found... Image is invalid...\n");
    }

    VkMemoryAllocateInfo memory_allocation_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = (u32)memory_index
    };
    VkAssert(vkAllocateMemory(render_context->rendering_device.logical_device, &memory_allocation_info, render_context->allocators, &result.memory));
    VkAssert(vkBindImageMemory(render_context->rendering_device.logical_device, result.handle, result.memory, 0)); // final parameter is an offset
    if(create_view)       
    {
        result.view = r_vulkan_image_view_create(render_context, format, &result, view_aspect_flags);
    }

    return(result);
}

////////////////////////////
// VULKAN RENDERPASS 
////////////////////////////

vulkan_renderpass_data_t
r_vulkan_renderpass_create(vulkan_render_context_t *render_context, 
                           vec2_t                   offset, 
                           vec2_t                   size, 
                           vec4_t                   clear_color, 
                           float32                  clear_depth, 
                           u32                      stencil_value)
{
    vulkan_renderpass_data_t result = {};

    VkSubpassDescription primary_subpass = {};
    primary_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // NOTE(Sleepster): Why is this a fucking clang extension????? 
    u32 attachment_description_count = render_context->swapchain.has_depth_attachment ? 2 : 1;
    VkAttachmentDescription attachment_descriptions[attachment_description_count];

    // NOTE(Sleepster): index 0, color attachment
    attachment_descriptions[0] = {
        .format         = render_context->swapchain.image_format.format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    primary_subpass.colorAttachmentCount = 1;
    primary_subpass.pColorAttachments    = &color_attachment_reference;

    // NOTE(Sleepster): index 1, depth attachment
    VkAttachmentReference depth_attachment_reference;
    if(render_context->swapchain.has_depth_attachment)
    {
        attachment_descriptions[1] = {
            .format = render_context->rendering_device.physical_device.device_depth_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        depth_attachment_reference = {
            .attachment = 1,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL 
        };

        primary_subpass.pDepthStencilAttachment = &depth_attachment_reference;
    }

    // NOTE(Sleepster): Input from a shader... 
    primary_subpass.inputAttachmentCount = 0;
    primary_subpass.pInputAttachments    = null;

    // NOTE(Sleepster): Used for multisampling...
    primary_subpass.pResolveAttachments = 0;

    // NOTE(Sleepster): Not needed this pass, but should be preserved for a future pass...
    primary_subpass.preserveAttachmentCount = 0;
    primary_subpass.pPreserveAttachments    = null;

    // NOTE(Sleepster): We only have 1 subpass, so it's VK_SUBPASS_EXTERNAL here. 
    VkSubpassDependency subpass_dependencies = {
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = null,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask   = 0,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachment_description_count,
        .pAttachments    = attachment_descriptions,
        .subpassCount    = 1,
        .pSubpasses      = &primary_subpass,
        .dependencyCount = 1,
        .pDependencies   = &subpass_dependencies,
        .pNext           = null,
        .flags           = 0
    };
    VkAssert(vkCreateRenderPass(render_context->rendering_device.logical_device, 
                               &renderpass_create_info, 
                                render_context->allocators, 
                               &result.handle));
    log_info("Vulkan renderpass created successfully...\n");

    return(result);
}

void
r_vulkan_renderpass_destroy(vulkan_render_context_t *render_context, vulkan_renderpass_data_t *renderpass)
{
    if(renderpass && renderpass->handle)
    {
        vkDestroyRenderPass(render_context->rendering_device.logical_device, renderpass->handle, render_context->allocators);
        renderpass->handle = 0;
    }
    else
    {
        log_warning("Tried to destroy a renderpass, but it's data isn't valid...\n");
    }
}

void
r_vulkan_renderpass_begin(vulkan_render_context_t      *render_context,
                          vulkan_renderpass_data_t     *renderpass,
                          vulkan_command_buffer_data_t *command_buffer,
                          VkFramebuffer                 framebuffer)
{
    VkRenderPassBeginInfo begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = renderpass->handle,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset.x      = (s32)renderpass->offset.x,
            .offset.y      = (s32)renderpass->offset.y,
            .extent.width  = (u32)renderpass->size.x,
            .extent.height = (u32)renderpass->size.y
        }
    };

    VkClearValue clear_values[2] = {};
    clear_values[0].color.float32[0] = renderpass->clear_color.x;
    clear_values[0].color.float32[1] = renderpass->clear_color.y;
    clear_values[0].color.float32[2] = renderpass->clear_color.z;
    clear_values[0].color.float32[3] = renderpass->clear_color.w;

    clear_values[1].depthStencil.depth   = renderpass->depth_clear;
    clear_values[1].depthStencil.stencil = renderpass->stencil_clear;

    begin_info.clearValueCount = 2;
    begin_info.pClearValues    = clear_values;
    
    vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VKCBS_WITHIN_RENDERPASS;
}

void
r_vulkan_renderpass_end(vulkan_render_context_t      *render_context,
                        vulkan_command_buffer_data_t *command_buffer,
                        vulkan_renderpass_data_t     *renderpass)
{
    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = VKCBS_RECORDING;
}


////////////////////////////
// VULKAN SWAPCHAIN 
////////////////////////////

vulkan_swapchain_data_t
r_vulkan_swapchain_create(vulkan_render_context_t *render_context, 
                          u32                      image_width, 
                          u32                      image_height,
                          bool8                    wants_depth_buffer,
                          vulkan_swapchain_data_t *old_swapchain)
{
    vulkan_swapchain_data_t result = {};
    VkExtent2D swapchain_extent = {image_width, image_height};
        
    // NOTE(Sleepster): Triple buffering. 
    result.max_frames_in_flight = 2;

    vulkan_rendering_device_t *device_data = &render_context->rendering_device;
    
    bool8 default_surface_format_found = false;
    for(u32 format_index = 0;
        format_index < device_data->physical_device.swapchain_support_info.valid_surface_format_count;
        ++format_index)
    {
        VkSurfaceFormatKHR format = device_data->physical_device.swapchain_support_info.valid_surface_formats[format_index];
        if(format.format     == VK_FORMAT_R8G8B8A8_UNORM &&
           format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {    
            result.image_format = format;
            default_surface_format_found = true;

            break;
        }
    }

    if(!default_surface_format_found)
    {
        log_error("You really don't have this default image format?...\n");
        result.image_format = device_data->physical_device.swapchain_support_info.valid_surface_formats[0];
    }

    // NOTE(Sleepster): Vulkan spec says by default this one must exist:
    // https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkPresentModeKHR.html
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 present_mode_index = 0;
        present_mode_index < device_data->physical_device.swapchain_support_info.valid_present_mode_count;
        ++present_mode_index)
    {
        VkPresentModeKHR mode = device_data->physical_device.swapchain_support_info.valid_present_modes[present_mode_index];
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_present_mode = mode;
            break;
        }
    }
    result.present_mode = swapchain_present_mode; 

    r_vulkan_physical_device_get_swapchain_support_info(render_context, 
                                                        device_data->physical_device.handle,
                                                       &device_data->physical_device.swapchain_support_info);
    if(device_data->physical_device.swapchain_support_info.surface_capabilities.currentExtent.width != MAX_U32)
    {
        swapchain_extent = device_data->physical_device.swapchain_support_info.surface_capabilities.currentExtent;
    }

    VkExtent2D min_extent = device_data->physical_device.swapchain_support_info.surface_capabilities.minImageExtent;
    VkExtent2D max_extent = device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageExtent;

    swapchain_extent.width  = Clamp(swapchain_extent.width,  min_extent.width,  max_extent.width); 
    swapchain_extent.height = Clamp(swapchain_extent.height, min_extent.height, max_extent.height); 

    u32 swapchain_image_count = device_data->physical_device.swapchain_support_info.surface_capabilities.minImageCount + 1;
    if(device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount > 0 && 
       swapchain_image_count > device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount)
    {
        swapchain_image_count = device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = render_context->render_surface,
        .minImageCount    = swapchain_image_count,
        .imageFormat      = result.image_format.format,
        .imageColorSpace  = result.image_format.colorSpace,
        .imageExtent      = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    if(device_data->graphics_queue_family_index != device_data->present_queue_family_index)
    {
        u32 queue_family_indices[] = {
            device_data->graphics_queue_family_index,
            device_data->present_queue_family_index
        };

        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices   = 0;
    }

    // NOTE(Sleepster): COMPOSITE_ALPHA here is for the windowing system. NOT FOR THE IMAGES. 
    swapchain_create_info.preTransform   = device_data->physical_device.swapchain_support_info.surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode    = swapchain_present_mode;
    swapchain_create_info.clipped        = true;
    swapchain_create_info.oldSwapchain   = old_swapchain != null ? old_swapchain->handle : null;
    VkAssert(vkCreateSwapchainKHR(device_data->logical_device, &swapchain_create_info, render_context->allocators, &result.handle));

    render_context->current_frame_index = 0;
    render_context->current_image_index = 0;

    result.image_count = 0;
    VkAssert(vkGetSwapchainImagesKHR(device_data->logical_device, result.handle, &result.image_count, null));
    result.images = AllocArray(VkImage,     result.image_count);
    result.views  = AllocArray(VkImageView, result.image_count);
    VkAssert(vkGetSwapchainImagesKHR(device_data->logical_device, result.handle, &result.image_count, result.images));

    for(u32 view_index = 0;
        view_index < result.image_count;
        ++view_index)
    {
        VkImageViewCreateInfo image_view_create_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = result.images[view_index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = result.image_format.format,
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            },
        };

        VkAssert(vkCreateImageView(device_data->logical_device, &image_view_create_info, render_context->allocators, &result.views[view_index]));
    }

    if(wants_depth_buffer)
    {
        bool8 could_get = r_vulkan_physical_device_detect_depth_format(&device_data->physical_device);
        if(!could_get)
        {
            device_data->physical_device.device_depth_format = VK_FORMAT_UNDEFINED;
            log_fatal("Failure to find a supported depth format...\n");
        }

        result.depth_attachment = r_vulkan_image_create(render_context,
                                                        VK_IMAGE_TYPE_2D, 
                                                        swapchain_extent.width,
                                                        swapchain_extent.height,
                                                        device_data->physical_device.device_depth_format,
                                                        VK_IMAGE_TILING_OPTIMAL,
                                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                        VK_IMAGE_ASPECT_DEPTH_BIT,
                                                        1,
                                                        true);
        result.has_depth_attachment = true;
    }
    log_info("Swapchain Created...\n");

    return(result);
}

void
r_vulkan_swapchain_destroy(vulkan_render_context_t *render_context,
                           vulkan_swapchain_data_t *swapchain)
{
    if(swapchain->depth_attachment.handle)
    {
        r_vulkan_image_destroy(render_context, &swapchain->depth_attachment);
    }

    for(u32 view_index = 0;
        view_index < swapchain->image_count;
        ++view_index)
    {
        vkDestroyImageView(render_context->rendering_device.logical_device, 
                           swapchain->views[view_index], 
                           render_context->allocators);
    }

    vkDestroySwapchainKHR(render_context->rendering_device.logical_device, swapchain->handle, render_context->allocators);
    log_info("Swapchain destroyed...\n");
}

vulkan_swapchain_data_t
r_vulkan_swapchain_recreate(vulkan_render_context_t *render_context, 
                            u32                      image_width, 
                            u32                      image_height)
{
    vulkan_swapchain_data_t result;
    result = r_vulkan_swapchain_create(render_context, 
                                       image_width, 
                                       image_height, 
                                       true, 
                                      &render_context->swapchain);

    log_info("Swapchain Recreated...\n");
    return(result);
}

u32 
r_vulkan_swapchain_get_next_image_index(vulkan_render_context_t *render_context,
                                        vulkan_swapchain_data_t *swapchain,
                                        u64                      timeout_ns,
                                        VkSemaphore              image_semaphore,
                                        VkFence                  fence)
{
    u32 result = 0;
    VkResult code = vkAcquireNextImageKHR(render_context->rendering_device.logical_device, 
                                          render_context->swapchain.handle,
                                          timeout_ns, 
                                          image_semaphore,
                                          fence,
                                         &result);
    if(code == VK_ERROR_OUT_OF_DATE_KHR)
    {
        render_context->swapchain = r_vulkan_swapchain_recreate(render_context, 
                                                                render_context->framebuffer_width, 
                                                                render_context->framebuffer_height);
    }
    else if(code != VK_SUCCESS && code != VK_SUBOPTIMAL_KHR)
    {
        log_fatal("Failure to get the swapchain image...\n");
        Assert(false);

        result = INVALID_SWAPCHAIN_IMAGE_INDEX;
    }

    return(result);
}

void
r_vulkan_swapchain_present(vulkan_render_context_t *render_context,
                           vulkan_swapchain_data_t *swapchain,
                           VkQueue                  graphics_queue,
                           VkQueue                  present_queue,
                           VkSemaphore              render_semaphore,
                           u32                      image_index)
{
    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &render_semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain->handle,
        .pImageIndices      = &image_index,
    };
    
    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        render_context->swapchain = r_vulkan_swapchain_recreate(render_context, 
                                                                render_context->framebuffer_width, 
                                                                render_context->framebuffer_height);
    }
    else if(result != VK_SUCCESS)
    {
        log_fatal("Failure to present the swapchain image...\n");
        Assert(false);
    }
}

////////////////////////////
// VULKAN PHYSICAL DEVICE 
////////////////////////////

bool8
r_vulkan_physical_device_detect_depth_format(vulkan_physical_device_t *device)
{
    bool8 result = false;

    VkFormat valid_canidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u32 attachment_flag = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for(u32 canidate_index = 0;
        canidate_index < ArrayCount(valid_canidates);
        ++canidate_index)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->handle, valid_canidates[canidate_index], &properties);
        if((properties.linearTilingFeatures & attachment_flag) == attachment_flag)
        {
            device->device_depth_format = valid_canidates[canidate_index];
            result = true;

            break;
        }
        else if((properties.optimalTilingFeatures & attachment_flag) == attachment_flag)
        {
            device->device_depth_format = valid_canidates[canidate_index];
            result = true;

            break;
        }
    }

    return(result);
}

void 
r_vulkan_physical_device_get_swapchain_support_info(vulkan_render_context_t                         *context, 
                                                    VkPhysicalDevice                                 device,
                                                    vulkan_physical_device_swapchain_support_info_t *swapchain_info)
{
    VkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, 
                                                       context->render_surface, 
                                                       &swapchain_info->surface_capabilities));
    VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->render_surface, &swapchain_info->valid_surface_format_count, 0));
    if(swapchain_info->valid_surface_format_count > 0)
    {
        if(!swapchain_info->valid_surface_formats)
        {
            swapchain_info->valid_surface_formats = AllocArray(VkSurfaceFormatKHR, 
                                                               swapchain_info->valid_surface_format_count);
        }
        VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(device, 
                                                      context->render_surface, 
                                                      &swapchain_info->valid_surface_format_count, 
                                                      swapchain_info->valid_surface_formats));
    }

    VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->render_surface, &swapchain_info->valid_present_mode_count, 0));
    if(swapchain_info->valid_present_mode_count > 0)
    {
        if(!swapchain_info->valid_present_modes)
        {
            swapchain_info->valid_present_modes = AllocArray(VkPresentModeKHR, 
                                                             swapchain_info->valid_present_mode_count);
        }
    }

    VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->render_surface, 
                                                       &swapchain_info->valid_present_mode_count, 
                                                       swapchain_info->valid_present_modes));
}

bool32 
r_vulkan_physical_device_is_supported(vulkan_render_context_t                         *context_data, 
                                      VkPhysicalDevice                                 device,
                                      VkPhysicalDeviceProperties                      *device_properties,
                                      VkPhysicalDeviceFeatures                        *device_features,
                                      vulkan_physical_device_swapchain_support_info_t *device_swapchain_support_info,
                                      vulkan_physical_device_queue_info_t             *device_queue_info,
                                      vulkan_physical_device_requirements_t           *requirements)
{
    device_queue_info->graphics_queue_family_index = (u32)-1;
    device_queue_info->present_queue_family_index  = (u32)-1;
    device_queue_info->transfer_queue_family_index = (u32)-1;
    device_queue_info->compute_queue_family_index  = (u32)-1;

    u32 queue_family_counter;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_counter, 0);
    VkQueueFamilyProperties *queue_family_data = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queue_family_counter);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_counter, queue_family_data);

    log_info("GRAPHICS | PRESENT | COMPUTE | TRANSFER | DEVICE NAME\n");

    u8 minimum_transfer_score = 255;
    for(u32 queue_family_index = 0;
        queue_family_index < queue_family_counter;
        ++queue_family_index)
    {
        u8 current_transfer_score = 0;

        VkQueueFamilyProperties properties = queue_family_data[queue_family_index];
        if(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            device_queue_info->graphics_queue_family_index = queue_family_index;
            ++current_transfer_score;
        }

        if(properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            device_queue_info->compute_queue_family_index = queue_family_index;
            ++current_transfer_score;
        }

        if(properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if(current_transfer_score <= minimum_transfer_score)
            {
                minimum_transfer_score = current_transfer_score;
                device_queue_info->transfer_queue_family_index = queue_family_index;
            }
        }

        VkBool32 supports_presenting = VK_FALSE;
        VkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_index, context_data->render_surface, &supports_presenting));
        if(supports_presenting)
        {
            device_queue_info->present_queue_family_index = queue_family_index;
        }
    }

    log_info("   %d     |    %d    |    %d    |     %d    | %s\n",
             device_queue_info->graphics_queue_family_index, device_queue_info->present_queue_family_index,
             device_queue_info->compute_queue_family_index,  device_queue_info->transfer_queue_family_index,
             device_properties->deviceName);
    if((!requirements->has_graphics_queue || (requirements->has_graphics_queue && device_queue_info->graphics_queue_family_index != (u32)-1)) &&
       (!requirements->has_present_queue  || (requirements->has_present_queue  && device_queue_info->present_queue_family_index  != (u32)-1)) &&
       (!requirements->has_compute_queue  || (requirements->has_compute_queue  && device_queue_info->compute_queue_family_index  != (u32)-1)) &&
       (!requirements->has_transfer_queue || (requirements->has_transfer_queue && device_queue_info->transfer_queue_family_index != (u32)-1)))
    {
        log_info("Device meets queue requirements...\n");
        log_trace("Graphics Queue Family Index: %d\n", device_queue_info->graphics_queue_family_index);
        log_trace("Present  Queue Family Index: %d\n", device_queue_info->present_queue_family_index);
        log_trace("Transfer Queue Family Index: %d\n", device_queue_info->transfer_queue_family_index);
        log_trace("Compute  Queue Family Index: %d\n", device_queue_info->compute_queue_family_index);

        r_vulkan_physical_device_get_swapchain_support_info(context_data, device, device_swapchain_support_info);
        

        if(device_swapchain_support_info->valid_surface_format_count < 1 || device_swapchain_support_info->valid_present_mode_count < 1)
        {
            log_info("Swapchain requirements not met, skipping this device...\n");
            return(false);
        }

        if(requirements->required_extensions)
        {
            u32 device_avaliable_extension_counter = 0;
            VkExtensionProperties *device_avaliable_extensions = null;

            VkAssert(vkEnumerateDeviceExtensionProperties(device, 0, &device_avaliable_extension_counter, 0));
            if(device_avaliable_extension_counter != 0)
            {
                device_avaliable_extensions = AllocArray(VkExtensionProperties, device_avaliable_extension_counter);
            }

            for(u32 extension_index = 0;
                extension_index < ArrayCount(requirements->required_extensions);
                ++extension_index)
            {
                bool8 found = false;
                const char *extension = requirements->required_extensions[extension_index];

                for(u32 device_extension_index = 0;
                    device_extension_index < device_avaliable_extension_counter;
                    ++device_extension_index)
                {
                    const char *device_extension = device_avaliable_extensions[device_extension_index].extensionName;
                    if(strcmp(extension, device_extension) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    log_info("Vulkan Extension '%s' was no found... skipping device...\n", extension);
                    return(false);
                }
            }
        }

        return(true);
    }

    return(false);
}

////////////////////////////
// VULKAN RENDERER CORE 
////////////////////////////

s32 
r_vulkan_find_memory_index(vulkan_render_context_t *render_context,
                           u32                      type_filter, 
                           u32                      property_flags)
{
    s32 result = -1;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(render_context->rendering_device.physical_device.handle, &memory_properties);

    for(u32 memory_index = 0;
        memory_index < memory_properties.memoryTypeCount;
        ++memory_index)
    
    {
        if((type_filter & (1 << memory_index)) && ((memory_properties.memoryTypes[memory_index].propertyFlags & property_flags) == property_flags))
        {
            result = memory_index;
            break;
        }
    }

    if(result == -1)
    {
        log_error("Failure to find a valid memory type...\n");
    }

    return(result);
}

void
r_renderer_init(vulkan_render_context_t *render_context)
{
	VkApplicationInfo app_info = {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName   = null;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName        = null;
	app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion         = VK_API_VERSION_1_3;

    u32 platform_extension_count = 0;
    const char * const *SDL_extensions = SDL_Vulkan_GetInstanceExtensions(&platform_extension_count);
    if(SDL_extensions == null)
    {
        log_error("We have failed to get the SDL_Vulkan instance extensions... Error: '%s'\n", SDL_GetError());
        SDL_Quit();
    }
    platform_extension_count += 1;
    DynArray(char*) extensions = c_dynarray_create(char*);
    extensions = c_dynarray_reserve(extensions, platform_extension_count);

    // NOTE(Sleepster): DEBUG LAYERS 
    extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    memcpy((byte*)extensions + sizeof(char *), SDL_extensions, (platform_extension_count - 1) * sizeof(char *));

    const char *layer = "VK_LAYER_KHRONOS_validation\0";
    DynArray(char*) validation_layers = c_dynarray_create(char*);
    c_dynarray_push(validation_layers, layer);

    u32 total_validation_layers = 0;
    VkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, 0));

    DynArray(VkLayerProperties) found_validation_layers = c_dynarray_create(VkLayerProperties);
    found_validation_layers = c_dynarray_reserve(found_validation_layers, total_validation_layers);

    VkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, found_validation_layers));
    c_dynarray_for(validation_layers, layer_index)
    {
        const char *layer_to_find = c_dynarray_get_at_index(validation_layers, layer_index);
        log_info("Searching for Vulkan validation layer: '%s'\n", layer_to_find);

        bool8 found = false;
        for(u32 property_index = 0;
            property_index < total_validation_layers;
            ++property_index)
        {
            char *layer_name = (found_validation_layers + property_index)->layerName;
            if(strcmp(layer_to_find, layer_name) == 0)
            {
                found = true;
                log_info("Layer found...\n");

                break;
            }
        }

        if(!found)
        {
            log_error("Failure to find Vulkan validation layer: '%s'\n", layer_to_find);
        }
    }

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount   = platform_extension_count;
    instance_info.ppEnabledLayerNames     = validation_layers;
    instance_info.enabledLayerCount       = 1;

    VkAssert(vkCreateInstance(&instance_info, render_context->allocators, &render_context->instance));
    log_info("Vulkan Instance Created..\n");

    // NOTE(Sleepster): DEBUG LAYERS 
    {
        u32 debug_log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

        u32 debug_message_types = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT vulkan_debug_info = {};
        vulkan_debug_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        vulkan_debug_info.messageSeverity = debug_log_severity;
        vulkan_debug_info.messageType     = debug_message_types;
        vulkan_debug_info.pfnUserCallback = Vk_debug_log_callback;

        PFN_vkCreateDebugUtilsMessengerEXT vk_debug_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(render_context->instance, 
                                                                                                                     "vkCreateDebugUtilsMessengerEXT");

        VkAssert(vk_debug_func(render_context->instance, &vulkan_debug_info, null, &render_context->debug_callback));
    }

    c_dynarray_destroy(extensions);
    c_dynarray_destroy(validation_layers);
    c_dynarray_destroy(found_validation_layers);

    if(!SDL_Vulkan_CreateSurface(render_context->window, render_context->instance, render_context->allocators, &render_context->render_surface))
    {
        log_error("Failure to create the SDL Vulkan Surface... Error: '%s\n'", SDL_GetError());
    }

    log_info("Vulkan surface created...\n");

    // NOTE(Sleepster): GET VULKAN PHYSICAL DEVICE
    {
        u32 physical_device_counter = 0;
        VkAssert(vkEnumeratePhysicalDevices(render_context->instance, &physical_device_counter, 0));
        VkPhysicalDevice *physical_devices = AllocArray(VkPhysicalDevice, physical_device_counter);
        VkAssert(vkEnumeratePhysicalDevices(render_context->instance, &physical_device_counter, physical_devices));

        log_info("Physical Device(s) found!\n");
        for(u32 device_index = 0;
            device_index < physical_device_counter;
            ++device_index)
        {
            VkPhysicalDevice current_device = physical_devices[device_index];

            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(current_device, &device_properties);

            VkPhysicalDeviceFeatures device_features;
            vkGetPhysicalDeviceFeatures(current_device, &device_features);

            VkPhysicalDeviceMemoryProperties device_memory_properties;
            vkGetPhysicalDeviceMemoryProperties(current_device, &device_memory_properties);

            vulkan_physical_device_requirements_t requirements;
            requirements.has_graphics_queue = true;
            requirements.has_present_queue  = true;
            requirements.has_transfer_queue = true;
            requirements.has_compute_queue  = true;

            requirements.required_extensions = AllocArray(const char *, 10);
            requirements.required_extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

            vulkan_physical_device_swapchain_support_info_t device_swapchain_support_info = {};
            vulkan_physical_device_queue_info_t             device_queue_information = {};
            if(r_vulkan_physical_device_is_supported(render_context,
                                                     current_device,
                                                     &device_properties,
                                                     &device_features,
                                                     &device_swapchain_support_info,
                                                     &device_queue_information,
                                                     &requirements))
            {
                log_info("Device: '%s' selected...\n", device_properties.deviceName);
                switch(device_properties.deviceType)
                {
                    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    {
                        log_info("Device Type is unknown...\n");
                    }break;
                    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    {
                        log_info("Device Type is 'Discrete GPU'\n");
                    }break;
                    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    {
                        log_info("Device Type is 'Integrated GPU'\n");
                    }break;
                    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    {
                        log_info("Device Type is 'Virtual GPU'\n");
                    }break;
                    case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    {
                        log_info("Device Type is 'CPU'\n");
                    }break;
                }

                log_info("GPU DRIVER VERSION: %d.%d.%d\n",
                         VK_VERSION_MAJOR(device_properties.driverVersion),
                         VK_VERSION_MINOR(device_properties.driverVersion),
                         VK_VERSION_PATCH(device_properties.driverVersion));
                log_info("Vulkan API Version: %d.%d.%d\n",
                         VK_VERSION_MAJOR(device_properties.apiVersion),
                         VK_VERSION_MINOR(device_properties.apiVersion),
                         VK_VERSION_PATCH(device_properties.apiVersion));
                for(u32 heap_index = 0;
                    heap_index < device_memory_properties.memoryHeapCount;
                    ++heap_index)
                {
                    float32 memory_sizeGB = (float32)(device_memory_properties.memoryHeaps[heap_index].size) / 1024.0f / 1024.0f / 1024.0f;
                    if(device_memory_properties.memoryHeaps[heap_index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    {
                        log_info("Local GPU Memory: '%.2fGB'...\n", memory_sizeGB);
                    }
                    else
                    {
                        log_info("Shared GPU Memory: '%.2fGB'...\n", memory_sizeGB);
                    }
                }

                vulkan_rendering_device_t *device = &render_context->rendering_device;

                device->physical_device.handle = current_device;
                Assert(device->physical_device.handle);

                device->graphics_queue_family_index = device_queue_information.graphics_queue_family_index;
                device->present_queue_family_index  = device_queue_information.present_queue_family_index;
                device->transfer_queue_family_index = device_queue_information.transfer_queue_family_index;
                device->compute_queue_family_index  = device_queue_information.compute_queue_family_index;

                device->physical_device.properties             = device_properties;
                device->physical_device.features               = device_features;
                device->physical_device.memory_properties      = device_memory_properties;
                device->physical_device.swapchain_support_info = device_swapchain_support_info;

                break;
            }
        }

        if(!render_context->rendering_device.physical_device.handle)
        {
            log_fatal("No compatible physical GPU device was found...\n");
        }

        log_info("Physical device created successfully...\n");
    }

    // NOTE(Sleepster): LOGICAL DEVICE
    {
        bool8 present_queue_shares_graphics_queue  = render_context->rendering_device.graphics_queue_family_index  == render_context->rendering_device.present_queue_family_index;
        bool8 transfer_queue_shares_graphics_queue = render_context->rendering_device.graphics_queue_family_index  == render_context->rendering_device.transfer_queue_family_index;
        bool8 compute_queue_shares_any             = (render_context->rendering_device.graphics_queue_family_index == render_context->rendering_device.compute_queue_family_index) ||
                                                     (render_context->rendering_device.present_queue_family_index  == render_context->rendering_device.compute_queue_family_index)  ||
                                                     (render_context->rendering_device.transfer_queue_family_index == render_context->rendering_device.compute_queue_family_index);

        u32 index_count = 1;
        if(!present_queue_shares_graphics_queue)  index_count++;
        if(!transfer_queue_shares_graphics_queue) index_count++;
        if(!compute_queue_shares_any) index_count++;

        u32 indices[4] = {};
        u32 index = 0;

        indices[index++] = render_context->rendering_device.graphics_queue_family_index;
        if(!present_queue_shares_graphics_queue)
        {
            indices[index++] = render_context->rendering_device.present_queue_family_index;
        }
        if(!transfer_queue_shares_graphics_queue)
        {
            indices[index++] = render_context->rendering_device.transfer_queue_family_index;
        }
        if(!compute_queue_shares_any)
        {
            indices[index++] = render_context->rendering_device.compute_queue_family_index;
        }

        VkDeviceQueueCreateInfo queue_create_infos[4] = {};
        for(u32 queue_index = 0;
            queue_index < index_count;
            ++queue_index)
        {
            VkDeviceQueueCreateInfo *info = queue_create_infos + queue_index;

            info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info->queueFamilyIndex = indices[queue_index];
            info->queueCount       = 1;
            if(indices[queue_index] == render_context->rendering_device.graphics_queue_family_index)
            {
                info->queueCount = 2;
            }

            info->flags = 0;
            info->pNext = 0;

            float32 queue_priority = 1.0f;
            info->pQueuePriorities = &queue_priority;
        }

        VkDeviceCreateInfo device_create_info = {};
        device_create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount  = index_count;
        device_create_info.pQueueCreateInfos     = queue_create_infos;
        device_create_info.pEnabledFeatures      = 0;
        device_create_info.enabledExtensionCount = 1;

        const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        device_create_info.ppEnabledExtensionNames = &extension;

        VkAssert(vkCreateDevice(render_context->rendering_device.physical_device.handle,
                               &device_create_info,
                                render_context->allocators,
                               &render_context->rendering_device.logical_device));
        log_info("Logical device created...\n");

#if 0
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.graphics_queue_family_index, 0, &render_context->rendering_device.graphics_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.present_queue_family_index,  1, &render_context->rendering_device.present_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.transfer_queue_family_index, 0, &render_context->rendering_device.transfer_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.compute_queue_family_index,  0, &render_context->rendering_device.compute_queue);

        log_info("Device Queues gathered...\n");
#endif
    }
    render_context->swapchain       = r_vulkan_swapchain_create(render_context, render_context->window_width, render_context->window_height, true, null);
    render_context->main_renderpass = r_vulkan_renderpass_create(render_context, 
                                                                 {0, 0}, 
                                                                 {(float32)render_context->window_height, (float32)render_context->window_height}, 
                                                                 {0.3, 0.2, 0.4, 1.0}, 
                                                                 0.0f, 
                                                                 0);

    log_info("Vulkan context initialized...\n");
}
