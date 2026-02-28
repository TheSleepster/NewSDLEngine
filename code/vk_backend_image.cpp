/* ========================================================================
   $File: vk_backend_image.cpp $
   $Date: February 14 2026 01:46 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>
#include <vk_backend_image.h>

/*
=============
vk_backend_image_update_from_buffer
=============
*/

void
vk_backend_image_update_from_buffer(vulkan_context_t *vulkan_context, 
                                    vulkan_image_t   *image, 
                                    vulkan_buffer_t  *buffer, 
                                    VkCommandBuffer   command_buffer)
{
    VkBufferImageCopy copy_data = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .layerCount     = 1,
            .baseArrayLayer = 0,
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
        },
        .imageExtent = {
            .depth  = 1,
            .width  = image->width,
            .height = image->height
        }
    };

    // TODO(Sleepster): Should be configurable 
    vkCmdCopyBufferToImage(command_buffer, 
                           buffer->handle, 
                           image->handle, 
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           1, 
                           &copy_data);
}

/*
=============
vk_backend_image_update_data
=============
*/
void
vk_backend_image_update_data(vulkan_context_t *vulkan_context, vulkan_image_t *image)
{
    vulkan_image_info_t *image_info = &image->info;

    VkImageSubresourceRange src_range = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel   = 0,
        .layerCount     = 1,
        .levelCount     = 1,
    };
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context->device, image->handle, &memory_requirements);

    VkCommandBuffer scratch_buffer = vk_backend_get_and_begin_scratch_command_buffer(vulkan_context, true);
    vk_backend_image_change_layout(vulkan_context, 
                                   scratch_buffer,
                                   image->handle, 
                                   image->layout,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   src_range);

    vulkan_buffer_t copy_buffer = vk_backend_buffer_create(vulkan_context, 
                                                           memory_requirements.size,
                                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                                           VULKAN_MEMORY_USAGE_CPU_TO_GPU); 

    vk_backend_buffer_copy_data(&copy_buffer, image_info->data.data, image_info->data.count, 0);
    vk_backend_image_update_from_buffer(vulkan_context, image, &copy_buffer, scratch_buffer);

    vk_backend_image_change_layout(vulkan_context, 
                                   scratch_buffer,
                                   image->handle, 
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT,
                                   src_range);
    vk_backend_submit_and_release_scratch_command_buffer(vulkan_context, &scratch_buffer);

    vk_backend_buffer_destroy(vulkan_context, &copy_buffer);
}

internal_api bool8
is_image_format_depth_format(vulkan_image_t *image)
{
    bool8 result = false;

    return(result);
}

internal_api bool8
is_image_format_stencil_format(vulkan_image_t *image)
{
    bool8 result = false;

    return(result);
}

/*
=============
vk_backend_image_create_view
=============
*/

void
vk_backend_image_create_view(vulkan_context_t *vulkan_context, vulkan_image_t *image)
{
    VkImageAspectFlags aspect_mask = {};
    bool8 is_depth_format   = is_image_format_depth_format(image);
    bool8 is_stencil_format = is_image_format_stencil_format(image);
    if(aspect_mask == 0) 
    {
        if(is_depth_format && is_stencil_format) aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else if(is_depth_format)                 aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if(is_stencil_format)               aspect_mask = VK_IMAGE_ASPECT_STENCIL_BIT;
        else                                     aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewCreateInfo view_create_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = image->handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = image->internal_format,
        .subresourceRange = {
            .aspectMask     = aspect_mask,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        },
    };
    vkAssert(vkCreateImageView(vulkan_context->device, 
                              &view_create_info, 
                               vulkan_context->cpu_allocation_callbacks, 
                              &image->view));
}


/*
=============
vk_backend_image_destroy_view
=============
*/

void
vk_backend_image_destroy_view(vulkan_context_t *vulkan_context, vulkan_image_t *image)
{
    vkDestroyImageView(vulkan_context->device, image->view, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_image_create
=============
*/

vulkan_image_t
vk_backend_image_create(vulkan_context_t *vulkan_context, vulkan_image_info_t *image_info)
{
    Assert(image_info);

    vulkan_image_t result = {};
    result.info   = *image_info;
    result.width  =  image_info->width;
    result.height =  image_info->height;

    VkImageCreateInfo info = {};
    info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType     = (VkImageType)image_info->type;
    info.extent.width  = result.width;
    info.extent.height = result.height;
    info.extent.depth  = 1;
    info.mipLevels     = image_info->mip_count;
    info.arrayLayers   = 1;
    info.format        = (VkFormat)image_info->format;
    info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = (VkImageLayout)image_info->initial_layout;
    info.usage         = image_info->usage;
    info.samples       = (VkSampleCountFlagBits)VK_SAMPLE_COUNT_1_BIT;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    result.internal_format = (VkFormat)image_info->format;
    result.layout          = (VkImageLayout)image_info->initial_layout;

    vkAssert(vkCreateImage(vulkan_context->device, &info, vulkan_context->cpu_allocation_callbacks, &result.handle));

    // NOTE(Sleepster): Allocation 
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context->device, result.handle, &memory_requirements);
    result.allocation = vk_allocator_allocate(&vulkan_context->vulkan_allocator, 
                                              &memory_requirements, 
                                              VULKAN_MEMORY_USAGE_GPU_ONLY);
    VkResult code = vkBindImageMemory(vulkan_context->device, result.handle, result.allocation.memory, result.allocation.offset);
    if(!vk_backend_result_is_success(code))
    {
        Expect(false, "Failed to bind the memory for this image...\n");
    }

    if(image_info->data.data != null)
    {
        vk_backend_image_update_data(vulkan_context, &result);
    }

    vk_backend_image_create_view(vulkan_context, &result);
    return(result);
}

/*
=============
vk_backend_image_destroy
=============
*/

// NOTE(Sleepster): If we used a unique allocation block for this image, then 
//                  it should be moved to the allocator's free list 
void
vk_backend_image_destroy(vulkan_context_t *vulkan_context, vulkan_image_t *image)
{
    Assert(image);

    vk_backend_image_destroy_view(vulkan_context, image);
    vk_allocator_free(&vulkan_context->vulkan_allocator, &image->allocation);
    vkDestroyImage(vulkan_context->device, image->handle, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_image_change_layout
=============
*/

// TODO(Sleepster): Compute shaders will need to be able too see this stuff too
void
vk_backend_image_change_layout(vulkan_context_t       *vulkan_context, 
                               VkCommandBuffer         command_buffer,
                               VkImage                 image, 
                               VkImageLayout           current_layout,
                               VkImageLayout           target_layout, 
                               VkPipelineStageFlags    src_stage_flag,
                               VkPipelineStageFlags    dst_stage_flag,
                               VkAccessFlags           src_access_flags,
                               VkAccessFlags           dst_access_flags,
                               VkImageSubresourceRange range)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout            = current_layout;
    barrier.newLayout            = target_layout;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                = image;
	barrier.srcAccessMask        = src_access_flags;
	barrier.dstAccessMask        = dst_access_flags;
    barrier.subresourceRange     = range;

    vkCmdPipelineBarrier(command_buffer, 
                         src_stage_flag, 
                         dst_stage_flag, 
                         0, 0, 0, 0, 0, 1, 
                        &barrier);
}

/*
=============
vk_backend_sampler_create
=============
*/

VkSampler
vk_backend_sampler_create(vulkan_context_t *vulkan_context, vulkan_sampler_info_t *info)
{
    Assert(info);
    VkSampler result = {};

    VkSamplerCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.minFilter               = (VkFilter)info->min_filter;
    create_info.magFilter               = (VkFilter)info->mag_filter;
    create_info.addressModeU            = (VkSamplerAddressMode)info->wrapu;
    create_info.addressModeV            = (VkSamplerAddressMode)info->wrapv;
    create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.anisotropyEnable        = (VkBool32)info->anisotropy_enabled;
    create_info.maxAnisotropy           = info->max_anisotropy;
    create_info.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    create_info.compareEnable           = info->compare_enabled;
    create_info.compareOp               = (VkCompareOp)info->compare_operation;
    create_info.unnormalizedCoordinates = !info->use_normalized_coordinates;
    create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkAssert(vkCreateSampler(vulkan_context->device, &create_info, vulkan_context->cpu_allocation_callbacks, &result));

    return(result);
}

/*
=============
vk_backend_sampler_destroy
=============
*/

void
vk_backend_sampler_destroy(vulkan_context_t *vulkan_context, VkSampler sampler)
{
    vkDestroySampler(vulkan_context->device, sampler, vulkan_context->cpu_allocation_callbacks);
}


/*
=============
vk_backend_image_blit
=============
*/

void
vk_backend_image_blit(vulkan_context_t       *vulkan_context, 
                      VkImage                 source_image, 
                      VkImage                 destination_image, 
                      VkImageSubresourceRange source_range, 
                      VkImageSubresourceRange destination_range)
{
    
}
