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

// TODO(Sleepster): Thread safety...
struct vulkan_buffer_t
{
    VkBuffer                 handle;
    bool32                   is_mapped;

    u64                      size;
    u64                      used;
    u64                      offset;
    VkBufferUsageFlags       usage_flags;
    vulkan_allocation_info_t allocation;
};

// NOTE(Sleepster): We can build a list of these infos so that they can all be uploaded at once
// right before rendering so that we can limit the amount of pipeline barriers and waits. Waiting on
// one barrier and set of sync objects instead of many duplicate fences and commands.
struct vulkan_staging_info_t
{
    vulkan_buffer_t *target_buffer;
    byte            *data_to_upload;
    u64              upload_size;

    // NOTE(Sleepster): Written to us by the uploader 
    u64              staging_buffer_offset;
};

struct vulkan_staging_buffer_t
{
    vulkan_buffer_t buffer;
    bool32          submitted;
    VkFence         upload_complete_fence;
};

void* vk_backend_buffer_append_data(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, void *data, u32 size);

vulkan_buffer_t
vk_backend_buffer_create(vulkan_context_t              *vulkan_context, 
                         u64                            buffer_size, 
                         VkBufferUsageFlags             usage_flags, 
                         vulkan_allocation_usage_type_t usage_type);
void
vk_backend_buffer_copy_buffer(vulkan_context_t *vulkan_context,
                              vulkan_buffer_t  *source_buffer,
                              vulkan_buffer_t  *destination_buffer,
                              VkCommandBuffer   scratch_buffer,
                              u64               source_offset,
                              u64               source_copy_size,
                              u64               destination_offset);
void
vk_backend_buffer_copy_data(vulkan_context_t *vulkan_context,
                            vulkan_buffer_t  *buffer,
                            void             *data,
                            u64               copy_size,
                            u64               offset);

void                    vk_backend_buffer_destroy(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer);
void                    vk_backend_buffer_resize(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, VkCommandBuffer command_buffer, u64 new_size);
void*                   vk_backend_buffer_map(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer, u32 offset, u32 size);
void                    vk_backend_buffer_unmap(vulkan_context_t *vulkan_context, vulkan_buffer_t *buffer);

vulkan_staging_buffer_t vk_backend_staging_buffer_create(vulkan_context_t *vulkan_context, u64 size, VkBufferUsageFlags usage_flags, vulkan_allocation_usage_type_t memory_type);
void                    vk_backend_buffer_upload_staged_data(vulkan_context_t *vulkan_context, VkCommandBuffer command_buffer, vulkan_buffer_t *target_buffer);
void                    vk_backend_buffer_flush_staging_buffer(vulkan_context_t *vulkan_context, VkCommandBuffer   command_buffer);
void                    vk_backend_buffer_stage_data(vulkan_context_t *vulkan_context, byte *data, u64 data_size, vulkan_buffer_t *target_buffer);

#endif // VK_BACKEND_BUFFER_H

