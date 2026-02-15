/* ========================================================================
   $File: vk_backend_buffer.cpp $
   $Date: February 14 2026 04:09 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>

struct vulkan_buffer_t
{
    VkBuffer          handle;

    u64               size;
    u64               offset;
    u32               usage_flags;
    u32               memory_property_flags;
    u32               memory_index;

    byte             *mapped_data;
    VmaAllocationInfo allocation_info;
    VmaAllocation     gpu_memory;
};

vulkan_buffer_t
vk_backend_buffer_create(vulkan_context_t *vulkan_context, 
                         u64               buffer_size, 
                         u32               usage_flags, 
                         u32               memory_properties)
{
    vulkan_buffer_t result = {};
    result.size                  = buffer_size;
    result.offset                = 0;
    result.memory_property_flags = memory_properties;
    result.usage_flags           = usage_flags;

    VkBufferCreateInfo info = {};
    info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size        = buffer_size;
    info.usage       = usage_flags;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    vkAssert(vkCreateBuffer(vulkan_context->device, &info, vulkan_context->cpu_allocation_callbacks, &result.handle));

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage    = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocation_info.priority = 1.0f;
    vmaCreateBuffer(vulkan_context->vulkan_allocator, &info, &allocation_info, &result.handle, &result.gpu_memory, &result.allocation_info);

    return(result);
}
