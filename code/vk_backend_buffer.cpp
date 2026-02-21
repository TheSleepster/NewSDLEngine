/* ========================================================================
   $File: vk_backend_buffer.cpp $
   $Date: February 14 2026 04:09 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_buffer.h>
#include <vk_backend_core.h>

/*
=============
vk_backend_buffer_create
=============
*/

vulkan_buffer_t
vk_backend_buffer_create(vulkan_context_t              *vulkan_context, 
                         u64                            buffer_size, 
                         VkBufferUsageFlags             usage_flags, 
                         vulkan_allocation_usage_type_t usage_type,
                         bool8                          transient_allocation,
                         bool8                          use_unique)
{
    Assert(buffer_size > 0);

    vulkan_buffer_t result = {};
    result.size                  = buffer_size;
    result.offset                = 0;
    result.used                  = 0;
    result.usage_flags           = usage_flags;

    VkBufferCreateInfo info = {};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = buffer_size;
    info.usage       = usage_flags;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkAssert(vkCreateBuffer(vulkan_context->device, &info, vulkan_context->cpu_allocation_callbacks, &result.handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vulkan_context->device, result.handle, &memory_requirements);
    result.allocation = vk_allocator_allocate(&vulkan_context->vulkan_allocator, 
                                              &memory_requirements, 
                                               usage_type,
                                               false,
                                               false);
    VkResult code = vkBindBufferMemory(vulkan_context->device, result.handle, result.allocation.parent_block->memory, result.allocation.offset);
    if(!vk_backend_result_is_success(code))
    {
        Expect(false, "Failed to bind the memory for this GPU buffer...\n");
    }
#if 0
    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage    = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocation_info.priority = 1.0f;
    vmaCreateBuffer(vulkan_context->vulkan_allocator, &info, &allocation_info, &result.handle, &result.gpu_memory, &result.allocation_info);
#endif

    return(result);
}

/*
=============
vk_backend_buffer_destroy
=============
*/

void
vk_backend_buffer_destroy(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer)
{
    Assert(buffer);
    if(buffer->allocation.parent_block->is_unique)
    {
        vk_allocator_free_block(&vulkan_context->vulkan_allocator, buffer->allocation.parent_block);
    }
    vkDestroyBuffer(vulkan_context->device, buffer->handle, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_buffer_copy_buffer
=============
*/

// TODO(Sleepster): Maybe the backend should just store a bunch of these commands that just need to get done arbitrarily in no 
// particular order with a fence instead of just creating a command buffer here and just shooting it off.
void
vk_backend_buffer_copy_buffer(vulkan_context_t *vulkan_context,
                              vulkan_buffer_t  *source_buffer,
                              vulkan_buffer_t  *destination_buffer,
                              VkCommandBuffer   scratch_buffer,
                              u64               source_offset,
                              u64               source_copy_size,
                              u64               destination_offset)
{
    VkBufferCopy copy_range = {};
    copy_range.srcOffset = source_offset;
    copy_range.dstOffset = destination_offset;
    copy_range.size      = source_copy_size;

    vkCmdCopyBuffer(scratch_buffer, source_buffer->handle, destination_buffer->handle, 1, &copy_range);
    destination_buffer->used = source_copy_size;
}

/*
=============
vk_backend_buffer_copy_data
=============
*/

void
vk_backend_buffer_copy_data(vulkan_buffer_t *buffer,
                            void            *data,
                            u64              copy_size,
                            u64              offset)
{
    Assert(buffer->allocation.allocation_type != VULKAN_MEMORY_USAGE_GPU_ONLY);
    Assert(buffer->allocation.mapped_data != null);
    Assert(buffer->allocation.allocation_size - offset >= copy_size);

    memcpy(buffer->allocation.mapped_data, data, copy_size);
    buffer->used += copy_size;
}

/*
=============
vk_backend_buffer_resize
=============
*/

// TODO(Sleepster): Maybe optimize this so that if both buffers are HOST_VISIBLE it's just a memcpy, but meh
void
vk_backend_buffer_resize(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, VkCommandBuffer command_buffer, u64 new_size)
{
    Assert(buffer->handle);
    Assert(new_size > buffer->size);
    vulkan_buffer_t new_buffer = vk_backend_buffer_create(vulkan_context, 
                                                          new_size, 
                                                          buffer->usage_flags, 
                                                          buffer->allocation.allocation_type, 
                                                          buffer->allocation.parent_block->is_transient, 
                                                          buffer->allocation.parent_block->is_unique);
    vk_backend_buffer_copy_buffer(vulkan_context, buffer, &new_buffer, command_buffer, 0, buffer->size, 0);
    vk_backend_buffer_destroy(vulkan_context, buffer);

    *buffer = new_buffer;
}

/*
=============
vk_backend_staging_buffer_create
=============
*/

// NOTE(Sleepster):
// The way of handling staging buffers is simple. Each staging buffer is a node of a singly linked-list where each of the nodes
// tells us information about the copy. Items like the command buffer used, the buffer-copy is simply turned into a sort of render
// command that gets executed later. All staging buffers are uploaded at once. This is so that instead of having to set multiple
// pipeline barriers and fences, we can instead just set a single fence and barrier, making life much better.
//
// NOTE(Sleepster): Maybe it can just be a ring buffer??? 
vulkan_staging_buffer_t
vk_backend_staging_buffer_create(vulkan_context_t *vulkan_context, u64 size, VkBufferUsageFlags usage_flags, vulkan_allocation_usage_type_t memory_type)
{
    vulkan_staging_buffer_t result;
    result.buffer    = vk_backend_buffer_create(vulkan_context, size, usage_flags, memory_type, false, false);
    result.submitted = false;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(vulkan_context->device, &fence_info, vulkan_context->cpu_allocation_callbacks, &result.upload_complete_fence);

    return(result);
}

/*
=============
vk_backend_buffer_stage_data
=============
*/

// NOTE(Sleepster): You have to align this yourself
void
vk_backend_buffer_stage_data(vulkan_context_t *vulkan_context, byte *data, u64 data_size, vulkan_buffer_t *target_buffer)
{
    vulkan_staging_info_t staging_info = {};
    staging_info.data_to_upload = data;
    staging_info.upload_size    = data_size;
    staging_info.target_buffer  = target_buffer;

    c_dynarray_push(vulkan_context->staging_infos, staging_info);
}

/*
=============
vk_backend_buffer_upload_staged_data
=============
*/

void
vk_backend_buffer_upload_staged_data(vulkan_context_t *vulkan_context, VkCommandBuffer command_buffer) 
{
    vulkan_staging_buffer_t *staging_buffer = vulkan_context->staging_buffers + vulkan_context->current_frame_index;
    vulkan_buffer_t         *buffer         = &staging_buffer->buffer;
    c_dynarray_for(vulkan_context->staging_infos, info_index)
    {
        vulkan_staging_info_t *info = c_dynarray_get_ptr(vulkan_context->staging_infos, info_index);
        if(info->upload_size > buffer->size) 
        {
            Expect(false, 
                   "Requested upload size of: '%lu' for staging buffer of size: '%lu' is not a valid request...\n", 
                   info->upload_size, buffer->size);
        }

        u64 new_offset = buffer->used + info->upload_size;
        if(new_offset > buffer->size)
        {
            Expect(false, "Size of transfer buffer exceeded...\n");
        }
        vk_backend_buffer_copy_data(buffer, info->data_to_upload, info->upload_size, buffer->used);

        info->staging_buffer_offset = buffer->used;
        buffer->used               += info->upload_size;
    }    
}

/*
=============
vk_backend_buffer_flush_staging_buffer
=============
*/

void
vk_backend_buffer_flush_staging_buffer(vulkan_context_t *vulkan_context, 
                                       VkCommandBuffer   command_buffer)
{
    vulkan_staging_buffer_t *staging_buffer = vulkan_context->staging_buffers + vulkan_context->current_frame_index;
    c_dynarray_for(vulkan_context->staging_infos, info_index)
    {
        vulkan_staging_info_t *info = vulkan_context->staging_infos + info_index;
        Expect(info->upload_size + info->staging_buffer_offset <= staging_buffer->buffer.size, "Staging buffer size exceeeded...\n");

        VkBufferCopy region = {
            .srcOffset = info->staging_buffer_offset,
            .dstOffset = info->target_buffer->used,
            .size      = info->upload_size,
        };

        vkCmdCopyBuffer(command_buffer, staging_buffer->buffer.handle, info->target_buffer->handle, 1, &region);
        info->target_buffer->used += info->upload_size;
    }
    staging_buffer->buffer.used   = 0;
    staging_buffer->buffer.offset = 0;
    staging_buffer->submitted     = true;
    
    c_dynarray_clear(vulkan_context->staging_infos);
}
