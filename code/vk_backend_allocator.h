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
    vulkan_allocation_block_t     *parent_block;
    vulkan_allocation_usage_type_t allocation_type;

    VkDeviceSize                   offset;
    VkDeviceSize                   allocation_size;    

    byte                          *mapped_data;
};

struct vulkan_allocation_block_t
{
    u32                        DEBUG_id;
    bool32                     is_transient;
    bool32                     is_unique;

    VkDeviceSize               block_size;
    VkDeviceSize               used;
    u32                        memory_index;
    VkMemoryPropertyFlags      memory_flags;

    vulkan_allocator_t        *parent_allocator;
    VkDeviceMemory             memory;
    byte                      *persistent_mapped_ptr;

    vulkan_allocation_block_t *next_block;
    vulkan_allocation_block_t *prev_block;

    vulkan_allocation_usage_type_t allocation_type;
};

struct vulkan_allocator_t
{
    VkAllocationCallbacks     *cpu_allocation_callbacks;
    void                      *gpu_info;
    VkDevice                   device;

    memory_arena_t             block_allocator;
    VkDeviceSize               default_block_size;

    vulkan_allocation_block_t *first_transient_block; 
    vulkan_allocation_block_t *last_transient_block;

    vulkan_allocation_block_t *first_allocated_block;
    vulkan_allocation_block_t *last_allocated_block;

    vulkan_allocation_block_t *first_free_block;
};

vulkan_allocator_t         vk_allocator_create(vulkan_context_t *vulkan_context, u64 default_block_size);
void                       vk_allocator_destroy(vulkan_allocator_t *allocator);

vulkan_allocation_block_t* vk_allocator_get_or_create_block(vulkan_allocator_t *allocator, u32 memory_index, u32 allocation_size, bool8 temporary_allocation);
vulkan_allocation_info_t   vk_allocator_allocate(vulkan_allocator_t *allocator,  VkMemoryRequirements *requirements, vulkan_allocation_usage_type_t type, bool8 use_unique_block, bool8 temporary_allocation);
void                       vk_allocator_free_block(vulkan_allocator_t *allocator, vulkan_allocation_block_t *block_to_free);

void                       vk_allocator_clear_free_list(vulkan_allocator_t *allocator);
void                       vk_allocator_clear_transient_blocks(vulkan_allocator_t *allocator);
void                       vk_allocator_reset(vulkan_allocator_t *allocator);

#endif // VK_BACKEND_ALLOCATOR_H

