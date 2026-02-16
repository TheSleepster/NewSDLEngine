#if !defined(VK_BACKEND_BUFFER_H)
/* ========================================================================
   $File: vk_backend_buffer.h $
   $Date: February 15 2026 06:11 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define VK_BACKEND_BUFFER_H
#include <vulkan/vulkan.h>
#include <vk_backend_allocator.h>

struct vulkan_buffer_t
{
    VkBuffer                 handle;

    u64                      size;
    u64                      used;
    u64                      offset;
    VkBufferUsageFlags       usage_flags;
#if 1
    vulkan_allocation_info_t allocation;
#else
    VmaAllocationInfo allocation_info;
    VmaAllocation     gpu_memory;
#endif
};

vulkan_buffer_t
vk_backend_buffer_create(vulkan_context_t              *vulkan_context, 
                         u64                            buffer_size, 
                         VkBufferUsageFlags             usage_flags, 
                         vulkan_allocation_usage_type_t usage_type,
                         bool8                          transient_allocation,
                         bool8                          use_unique);
void
vk_backend_buffer_copy_buffer(vulkan_context_t *vulkan_context,
                              vulkan_buffer_t  *source_buffer,
                              vulkan_buffer_t  *destination_buffer,
                              u64               source_offset,
                              u64               source_copy_size,
                              u64               destination_offset);
void
vk_backend_buffer_copy_data(vulkan_buffer_t *buffer,
                            void            *data,
                            u64              copy_size,
                            u64              offset);

void vk_backend_buffer_destroy(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer);
void vk_backend_buffer_resize(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, u64 new_size);

#endif // VK_BACKEND_BUFFER_H

