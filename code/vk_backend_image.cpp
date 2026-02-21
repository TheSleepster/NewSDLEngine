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
    info.samples       = (VkSampleCountFlagBits)image_info->sample_count;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    vkAssert(vkCreateImage(vulkan_context->device, &info, vulkan_context->cpu_allocation_callbacks, &result.handle));

    // NOTE(Sleepster): Allocation 
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vulkan_context->device, result.handle, &memory_requirements);
    result.allocation = vk_allocator_allocate(&vulkan_context->vulkan_allocator, 
                                              &memory_requirements, 
                                              VULKAN_MEMORY_USAGE_GPU_ONLY, 
                                              false, 
                                              false);
    VkResult code = vkBindImageMemory(vulkan_context->device, result.handle, result.allocation.parent_block->memory, result.allocation.offset);
    if(!vk_backend_result_is_success(code))
    {
        Expect(false, "Failed to bind the memory for this image...\n");
    }

    if(image_info->data.data != null)
    {
        VkCommandBuffer scratch_buffer = vk_backend_get_and_begin_scratch_command_buffer(vulkan_context, true);
        vk_backend_image_change_layout(vulkan_context, &result, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, scratch_buffer);

        // TODO(Sleepster): This is wastful since the allocation from the allocator is too big and gives us the WHOLE block
        // even when we just want to bump allocate from it...
        if(vulkan_context->scratch_buffer.size < result.allocation.allocation_size)
        {
            //vk_backend_buffer_resize(vulkan_context, &vulkan_context->scratch_buffer, scratch_buffer, result.allocation.allocation_size);
        }
        // TODO(Sleepster): For now we assume 32bit colors... bad...
        u32 allocation_size = (image_info->width * image_info->height) * 4;
        vulkan_buffer_t copy_buffer = vk_backend_buffer_create(vulkan_context, 
                                                               allocation_size, 
                                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                                               VULKAN_MEMORY_USAGE_CPU_TO_GPU, 
                                                               false, 
                                                               false);

        vk_backend_buffer_copy_data(&vulkan_context->scratch_buffer, image_info->data.data, image_info->data.count, 0);
        vk_backend_image_update_from_buffer(vulkan_context, &result, &copy_buffer, scratch_buffer);

        // TODO(Sleepster): might wanna handle this more gracefully. Should we just let the image info tell us what the FINAL format should be?
        vk_backend_image_change_layout(vulkan_context, &result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scratch_buffer);
        vk_backend_submit_and_release_scratch_command_buffer(vulkan_context, &scratch_buffer);

        vk_backend_buffer_destroy(vulkan_context, &copy_buffer);
    }
#if 0
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    alloc_create_info.priority = 1.0f;

    vmaCreateImage(vulkan_context->vulkan_allocator, &info, &alloc_create_info, &result.handle, &result.gpu_memory, null);
#endif

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
    Assert(image->allocation.parent_block);
#if 1
    if(image->allocation.parent_block->is_unique)
    {
        vk_allocator_free_block(&vulkan_context->vulkan_allocator, image->allocation.parent_block);
    }
    vkDestroyImage(vulkan_context->device, image->handle, vulkan_context->cpu_allocation_callbacks);
#else
    vmaDestroyImage(vulkan_context->vulkan_allocator, image->handle, image->gpu_memory);
#endif
}

/*
=============
vk_backend_image_change_layout
=============
*/

// TODO(Sleepster): Compute shaders will need to be able too see this stuff too
void
vk_backend_image_change_layout(vulkan_context_t *vulkan_context, 
                               vulkan_image_t   *image, 
                               VkImageLayout     new_layout, 
                               VkCommandBuffer   command_buffer)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = (VkImageLayout)image->info.initial_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image->handle;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkPipelineStageFlags source_stage = 0;
    VkPipelineStageFlags dest_stage   = 0;
    if(image->info.initial_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // NOTE(Sleepster): Essentially "don't care where it is, just copy it" 
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage   = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(image->info.initial_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // NOTE(Sleepster): Copy from the stage, to the fragment shader 
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dest_stage   = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }
    else
    {
        Expect(false, "Unsupported image transition at the moment...\n");
    }

    vkCmdPipelineBarrier(command_buffer, 
                         source_stage, 
                         dest_stage, 
                         0, 0, 0, 0, 0, 1, 
                        &barrier);

    image->info.initial_layout = new_layout;
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
