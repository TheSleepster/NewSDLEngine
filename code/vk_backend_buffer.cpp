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
                         vulkan_allocation_usage_type_t usage_type)
{
    Assert(buffer_size > 0);

    vulkan_buffer_t result = {};
    result.size                  = buffer_size;
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
                                               usage_type);
    VkResult code = vkBindBufferMemory(vulkan_context->device, result.handle, result.allocation.memory, result.allocation.offset);
    if(!vk_backend_result_is_success(code))
    {
        Expect(false, "Failed to bind the memory for this GPU buffer...\n");
    }

    if(result.allocation.mapped_data)
    {
        result.is_mapped = true;
    }

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
    vk_allocator_free(&vulkan_context->vulkan_allocator, &buffer->allocation);
    vkDestroyBuffer(vulkan_context->device, buffer->handle, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_buffer_append_data
=============
*/

void*
vk_backend_buffer_append_data(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, void *data, u32 size)
{
    Assert(buffer->allocation.mapped_data != null);
    Assert(size <= buffer->allocation.allocation_size);
    if(buffer->allocation.mapped_data + buffer->used >= buffer->allocation.mapped_data + buffer->allocation.allocation_size)
    {
        buffer->used = 0;
    }
    void *result = buffer->allocation.mapped_data + buffer->used;

    memcpy(result, data, size);
    buffer->used += size;

    return(result);
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

// TODO(Sleepster): 
// This function just sucks... we can't account for offset in here so I don't know why we even bother passing it.
// and even if we could, we would want to offset from the start of the buffer or from the buffer->used offset? Meaning
// Would we want:
//
// buffer->used + offset
//
// or 
//
// buffer->mapped_data + offset
//
// either case, this is dumb...
void
vk_backend_buffer_copy_data(vulkan_context_t *vulkan_context,
                            vulkan_buffer_t  *buffer,
                            void             *data,
                            u64               copy_size,
                            u64               offset)
{
    Assert(buffer->allocation.allocation_size - offset >= copy_size);
    Assert(data != null);

    if(buffer->allocation.allocation_type == VULKAN_MEMORY_USAGE_CPU_TO_GPU ||
       buffer->allocation.allocation_type == VULKAN_MEMORY_USAGE_CPU_ONLY)
    {
        Assert(buffer->allocation.mapped_data != null);
        byte *mapped_data = buffer->allocation.mapped_data + buffer->allocation.offset;

        // NOTE(Sleepster): Data is persistently mapped by the vulkan allocator... 
        memcpy((mapped_data) + offset, data, copy_size);
    }
    else if(buffer->allocation.allocation_type == VULKAN_MEMORY_USAGE_GPU_ONLY ||
            buffer->allocation.allocation_type == VULKAN_MEMORY_USAGE_GPU_TO_CPU)
    {
        vk_backend_buffer_stage_data(vulkan_context, (byte*)data, copy_size, buffer);
    }
    else
    {
        InvalidCodePath;
    }

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
                                                          buffer->allocation.allocation_type);
    vk_backend_buffer_copy_buffer(vulkan_context, buffer, &new_buffer, command_buffer, 0, buffer->size, 0);
    vk_backend_buffer_destroy(vulkan_context, buffer);

    *buffer = new_buffer;
}

// TODO(Sleepster):  
//
// Mapping functionality is just completely unnecessary since CPU accessable buffers are persistently
// mapped so these are just completely redundant...

/*
=============
vk_backend_buffer_map
=============
*/

void*
vk_backend_buffer_map(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, u32 offset, u32 size)
{
    void *result = null;

    Assert(!buffer->is_mapped);
    Assert(buffer->allocation.allocation_type == VULKAN_MEMORY_USAGE_CPU_TO_GPU);

    vkAssert(vkMapMemory(vulkan_context->device, buffer->allocation.memory, offset, size, 0, &result));

    // TODO(Sleepster): Eventually wrap these operations (buffer->is_mapped) in an AtomicCompareExchange()... 
    buffer->is_mapped = true;

    return(result);
}

/*
=============
vk_backend_buffer_unmap
=============
*/

void
vk_backend_buffer_unmap(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer)
{
    Assert(buffer->is_mapped);
    vkUnmapMemory(vulkan_context->device, buffer->allocation.memory);

    // TODO(Sleepster): Same as above. 
    buffer->is_mapped = false;
}

/*
=============
vk_backend_buffer_unmap
=============
*/

true_inline void
vk_backend_buffer_reset(vulkan_buffer_t *buffer)
{
    buffer->used = 0;
}

//////////////////////////
// OLD STAGING BUFFER API
//////////////////////////

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
    result.buffer    = vk_backend_buffer_create(vulkan_context, size, usage_flags, memory_type);
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

// TODO(Sleepster): Bounds check and dynamic resizing.
void
vk_backend_buffer_stage_data(vulkan_context_t *vulkan_context, byte *data, u64 data_size, vulkan_buffer_t *target_buffer)
{
    Assert(target_buffer);
    Assert(data);
    Assert(data_size > 0);

    vulkan_staging_buffer_t *staging_buffer = vulkan_context->staging_buffers + vulkan_context->current_frame_index;
    vulkan_buffer_t         *buffer_handle  = &staging_buffer->buffer;

    // NOTE(Sleepster): You have to align upload_size yourself
    vulkan_staging_info_t staging_info = {};
    staging_info.upload_size           = data_size;
    staging_info.target_buffer         = target_buffer->handle;
    staging_info.staging_buffer_offset = buffer_handle->used;
    staging_info.target_offset         = target_buffer->used;

    // NOTE(Sleepster): This buffer is persistently mapped so it's all fine. 
    memcpy(buffer_handle->allocation.mapped_data + buffer_handle->used, data, data_size);
    buffer_handle->used += data_size;

    c_dynarray_push(vulkan_context->staging_infos, staging_info);
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

    dynarray_header_t *header = c_dynarray_header(vulkan_context->staging_infos);
    if(header && header->indices_used > 0) 
    {
        c_dynarray_for(vulkan_context->staging_infos, info_index)
        {
            vulkan_staging_info_t *info = vulkan_context->staging_infos + info_index;
            Expect(info->upload_size + info->staging_buffer_offset <= staging_buffer->buffer.size, "Staging buffer size exceeeded...\n");

            VkBufferCopy region = {
                .srcOffset = info->staging_buffer_offset,
                .dstOffset = info->target_offset,
                .size      = info->upload_size,
            };

            vkCmdCopyBuffer(command_buffer, staging_buffer->buffer.handle, info->target_buffer, 1, &region);
        }
        staging_buffer->buffer.used   = 0;
        staging_buffer->submitted     = true;

        c_dynarray_clear(vulkan_context->staging_infos);
    }
}
