/* ========================================================================
   $File: vk_backend_allocator.cpp $
   $Date: February 14 2026 05:30 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vulkan/vulkan.h>
#include <vk_backend_core.h>

#include <c_types.h>
#include <c_memory_arena.h>

#define VK_ALLOCATOR_DEBUG_ID (0xC0FFEE)

enum vulkan_allocation_usage_type_t
{
    VULKAN_MEMORY_USAGE_GPU_ONLY   = BIT(1),
    VULKAN_MEMORY_USAGE_CPU_ONLY   = BIT(2),
    VULKAN_MEMORY_USAGE_CPU_TO_GPU = BIT(3),
    VULKAN_MEMORY_USAGE_GPU_TO_CPU = BIT(4),
};

struct vulkan_allocation_block_t;
struct vulkan_allocation_chunk_t 
{
    vulkan_allocation_block_t *parent_block;
    VkDeviceSize               offset;
    VkDeviceSize               allocation_size;

    byte                      *mapped;
};

struct vulkan_allocation_block_t
{
    u32                        DEBUG_id;
    bool32                     is_transient;

    VkDeviceSize               block_size;
    VkDeviceSize               used;
    u32                        memory_index;

    VkDeviceMemory             memory;

    vulkan_allocation_block_t *next_block;
    vulkan_allocation_block_t *prev_block;
};

struct vulkan_allocator_t
{
    VkAllocationCallbacks     *cpu_allocation_callbacks;
    gpu_info_t                *gpu_info;
    VkDevice                   device;

    memory_arena_t             block_allocator;
    VmaAllocator               heap_allocator;
    VkDeviceSize               default_block_size;

    vulkan_allocation_block_t *first_transient_block; 
    vulkan_allocation_block_t *last_transient_block;

    vulkan_allocation_block_t *first_allocated_block;
    vulkan_allocation_block_t *last_allocated_block;

    vulkan_allocation_block_t *first_free_block;
};

internal_api s32
find_memory_index(gpu_info_t *gpu, u32 memory_type_bits, vulkan_allocation_usage_type_t usage_type)
{
    s32 result = -1;

    VkMemoryPropertyFlags required  = 0;
    VkMemoryPropertyFlags preferred = 0;
    switch(usage_type) 
    {
        case VULKAN_MEMORY_USAGE_GPU_ONLY:
        {
            preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }break;
        case VULKAN_MEMORY_USAGE_CPU_ONLY:
        {
            required |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }break;
        case VULKAN_MEMORY_USAGE_CPU_TO_GPU:
        {
            required  |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            preferred |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }break;
        case VULKAN_MEMORY_USAGE_GPU_TO_CPU:
        {
            required  |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            preferred |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }break;
        default:
        {
            Expect(false, "Failure to find the correct memory type for this allocation... Unknown memory usage...\n");
        }break;
    }

    // NOTE(Sleepster): Loop and find the correct memory index, returning a preferred one. 
    VkPhysicalDeviceMemoryProperties *memory_properties = &gpu->memory_properties;
    for(u32 index = 0;
        index < memory_properties->memoryTypeCount;
        ++index)
    {
        VkMemoryPropertyFlags flags = memory_properties->memoryTypes[index].propertyFlags;
        if((flags & required) != required)
        {
            continue;
        }

        if((flags & preferred) == preferred)
        {
            result = index;
            break;
        }
    }

    // NOTE(Sleepster): If we do NOT find the preferred memory type, just return whatever is valid.
    if(result == -1)
    {
        for(u32 index = 0;
            index < memory_properties->memoryTypeCount;
            ++index)
        {
            VkMemoryPropertyFlags flags = memory_properties->memoryTypes[index].propertyFlags;
            if((flags & required) == required)
            {
                result = index;
                break;
            }
        }
    }

    return(result);

}

vulkan_allocator_t
vk_allocator_create(vulkan_context_t *vulkan_context, u64 default_block_size)
{
    vulkan_allocator_t result = {};
    result.default_block_size       =  default_block_size;
    result.block_allocator          =  c_arena_create(MB(20));
    result.device                   =  vulkan_context->device;
    result.cpu_allocation_callbacks =  vulkan_context->cpu_allocation_callbacks;
    result.gpu_info                 = &vulkan_context->gpu;

#if 0
    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice   = vulkan_context->gpu.device;
    allocator_create_info.device           = vulkan_context->device;
    allocator_create_info.instance         = vulkan_context->instance;
    vmaCreateAllocator(&allocator_create_info, &result.heap_allocator);
#endif

    return(result);
}

void
vk_allocator_free_block(vulkan_allocator_t        *allocator,
                        vulkan_allocation_block_t *block_to_free)
{
    vulkan_allocation_block_t *next_block = block_to_free->next_block;
    vulkan_allocation_block_t *prev_block = block_to_free->next_block;

    prev_block->next_block = next_block;
    next_block->prev_block = prev_block;

    if(allocator->first_free_block == null)
    {
        allocator->first_free_block = block_to_free;
    }
    else
    {
        block_to_free->next_block   = allocator->first_free_block;
        allocator->first_free_block = block_to_free;
    }
}

void
vk_allocator_clear_free_list(vulkan_allocator_t *allocator)
{
    for(vulkan_allocation_block_t *current_block = allocator->first_free_block;
        current_block;
        current_block = current_block->next_block)
    {
        current_block->memory_index = -1;
        vkFreeMemory(allocator->device, current_block->memory, allocator->cpu_allocation_callbacks);
    }
}

void
vk_allocator_clear_transient_blocks(vulkan_allocator_t *allocator)
{
    for(vulkan_allocation_block_t *current_block = allocator->first_transient_block;
        current_block;
        current_block = current_block->next_block)
    {
        current_block->used = 0;
        vk_allocator_free_block(allocator, current_block);
    }
}

// TODO(Sleepster): Reset all blocks, freeing their memory
void
vk_allocator_reset(vulkan_allocator_t *allocator)
{
    vk_allocator_clear_free_list(allocator);

    allocator->first_allocated_block = null;
    allocator->last_allocated_block  = null;
    allocator->first_transient_block = null;
    allocator->last_transient_block  = null;
    allocator->first_free_block      = null;
    c_arena_reset(&allocator->block_allocator);
}

void
vk_allocator_get_or_create_block(vulkan_allocator_t        *allocator, 
                                 vulkan_allocation_block_t *valid_block,
                                 u32                        memory_index,
                                 u32                        allocation_size,
                                 bool8                      temporary_allocation)
{
    for(vulkan_allocation_block_t *current_block = allocator->first_free_block;
        current_block;
        current_block = current_block->next_block)
    {
        if((s32)current_block->memory_index == -1)
        {
            vulkan_allocation_block_t *prev_block = current_block->prev_block;
            vulkan_allocation_block_t *next_block = current_block->next_block;

            valid_block = current_block;

            prev_block->next_block = next_block;
            next_block->prev_block = prev_block;

            break;
        }
    }
    if(valid_block == null)
    {
        valid_block = c_arena_push_struct(&allocator->block_allocator, vulkan_allocation_block_t);
    }

    ZeroStruct(*valid_block);

    valid_block->DEBUG_id     = VK_ALLOCATOR_DEBUG_ID;
    valid_block->block_size   = allocator->default_block_size > allocation_size ? allocator->default_block_size : allocation_size;
    valid_block->memory_index = memory_index;

    VkMemoryAllocateInfo info = {};
    info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.memoryTypeIndex = memory_index;
    info.allocationSize  = allocation_size;
    vkAllocateMemory(allocator->device, &info, allocator->cpu_allocation_callbacks, &valid_block->memory);

    vulkan_allocation_block_t *first_block = temporary_allocation ? 
                                             allocator->first_transient_block : 
                                             allocator->first_allocated_block;

    vulkan_allocation_block_t *last_block  = temporary_allocation ? 
                                             allocator->last_transient_block  : 
                                             allocator->last_allocated_block;
    // NOTE(Sleepster): Chain the new block 
    if(first_block == null)
    {
        first_block = valid_block;
    }

    if(last_block != null)
    {
        last_block->next_block = valid_block;

        valid_block->prev_block = last_block;
        valid_block->next_block = null;

        last_block = valid_block;
    }
    else if(last_block == null)
    {
        last_block = valid_block;
    }
}

vulkan_allocation_chunk_t
vk_allocator_allocate(vulkan_allocator_t            *allocator, 
                      VkMemoryRequirements          *requirements, 
                      vulkan_allocation_usage_type_t type,
                      bool8                          use_unique_block,
                      bool8                          temporary_allocation)
{
    vulkan_allocation_chunk_t result = {};

    s32 memory_index = find_memory_index(allocator->gpu_info, requirements->memoryTypeBits, type);
    if(memory_index == -1) 
    {
        Expect(false, "Failed to find the correct memory index for this allocation...\n");
    }
    u32 allocation_size = Align(requirements->size, requirements->alignment);

    vulkan_allocation_block_t *valid_block = null;
    if(!use_unique_block)
    {
        vulkan_allocation_block_t *first_block = temporary_allocation ? 
                                                 allocator->first_transient_block : 
                                                 allocator->first_allocated_block;
        for(vulkan_allocation_block_t *current_block = first_block;
            current_block;
            current_block = current_block->next_block)
        {
            if(current_block->memory_index == (u32)memory_index)
            {
                if(current_block->block_size - current_block->used >= allocation_size)
                {
                    valid_block = current_block;
                    break;
                }
            }
        }
    }
    if(valid_block == null)
    {
        vk_allocator_get_or_create_block(allocator, valid_block, memory_index, allocation_size, temporary_allocation);
    }
    Assert(valid_block->DEBUG_id == VK_ALLOCATOR_DEBUG_ID);
    u32 offset = valid_block->used;

    valid_block->used        += allocation_size;
    valid_block->is_transient = temporary_allocation;

    result.parent_block    = valid_block;
    result.allocation_size = allocation_size;
    result.offset          = offset;

    return(result);
}
