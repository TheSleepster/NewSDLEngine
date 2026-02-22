#if !defined(VK_BACKEND_ALLOCATOR_H)
/* ========================================================================
   $File: vk_backend_allocator.h $
   $Date: February 15 2026 03:07 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define VK_BACKEND_ALLOCATOR_H
#include <vulkan/vulkan.h>

#include <c_types.h>
#include <c_memory_arena.h>

#define VK_ALLOCATOR_DEBUG_ID (0xC0FFEE)

struct vulkan_context_t;
struct vulkan_allocation_block_t;
struct vulkan_allocator_t;

enum vulkan_allocation_usage_type_t
{
    VULKAN_MEMORY_USAGE_GPU_ONLY   = BIT(1),
    VULKAN_MEMORY_USAGE_CPU_ONLY   = BIT(2),
    VULKAN_MEMORY_USAGE_CPU_TO_GPU = BIT(3),
    VULKAN_MEMORY_USAGE_GPU_TO_CPU = BIT(4),
};

// NOTE(Sleepster): This is essentially just a chunk suballocated from the block
struct vulkan_allocation_info_t
{
    vulkan_allocation_usage_type_t allocation_type;

    VkDeviceSize                   offset;
    VkDeviceSize                   allocation_size;    
    VkMemoryPropertyFlags          allocation_flags;
    VkMemoryRequirements           memory_requirements;
    VkDeviceMemory                 memory;

    byte                          *mapped_data;
};

struct vulkan_allocator_t
{
    VkAllocationCallbacks     *cpu_allocation_callbacks;
    void                      *gpu_info;
    VkDevice                   device;

    memory_arena_t             block_allocator;
    VkDeviceSize               default_block_size;

    vulkan_allocation_block_t *first_free_block;
};

vulkan_allocator_t         vk_allocator_create(vulkan_context_t *vulkan_context, u64 default_block_size);
void                       vk_allocator_destroy(vulkan_allocator_t *allocator);

vulkan_allocation_info_t   vk_allocator_allocate(vulkan_allocator_t *allocator, VkMemoryRequirements *requirements, vulkan_allocation_usage_type_t type);
void                       vk_allocator_free(vulkan_allocator_t *allocator, vulkan_allocation_info_t *info);

#endif // VK_BACKEND_ALLOCATOR_H

