/* ========================================================================
   $File: vk_backend_allocator.cpp $
   $Date: February 14 2026 05:30 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>
#include <vk_backend_allocator.h>

internal_api u64 get_device_heap_size(void *device, u32 memory_index);
/*
=============
find_memory_index
=============
*/

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
            // NOTE(Sleepster): We don't use device local here because of 2 reasons:
            // 1.) The heap is capped to 240MB on my GPU
            // 2.) Apparently it's faster to just copy from HOST_VISIBLE|HOST_COHERENT memory to the GPU
            required  |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            preferred |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
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

/*
=============
vk_allocator_create
=============
*/

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

/*
=============
vk_allocator_destroy
=============
*/

void
vk_allocator_destroy(vulkan_allocator_t *allocator)
{
    c_arena_destroy(&allocator->block_allocator);
}


/*
=============
vk_allocator_allocate
=============
*/

vulkan_allocation_info_t
vk_allocator_allocate(vulkan_allocator_t            *allocator, 
                      VkMemoryRequirements          *requirements, 
                      vulkan_allocation_usage_type_t type)
{
    vulkan_allocation_info_t result = {};

    s32 memory_index = find_memory_index((gpu_info_t*)allocator->gpu_info, requirements->memoryTypeBits, type);
    if(memory_index == -1) 
    {
        Expect(false, "Failed to find the correct memory index for this allocation...\n");
    }
    VkMemoryAllocateInfo memory_allocation_info = {};
    memory_allocation_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocation_info.memoryTypeIndex = memory_index;
    memory_allocation_info.allocationSize  = requirements->size;
    VkResult code = vkAllocateMemory(allocator->device, &memory_allocation_info, allocator->cpu_allocation_callbacks, &result.memory);
    if(!vk_backend_result_is_success(code))
    {
        Expect(false, "Failed to allocate memory for the depth buffer...\n");
    }

    result.allocation_size = requirements->size;
    result.allocation_type = type;
    result.offset          = 0;

    gpu_info_t *gpu_info = (gpu_info_t *)allocator->gpu_info;
    VkMemoryPropertyFlags flags = gpu_info->memory_properties.memoryTypes[memory_index].propertyFlags;
    result.allocation_flags     = flags;
    result.memory_requirements  = *requirements;

    // NOTE(Sleepster): 
    // "Keeping your memory persistently mapped is generally OK in Vulkan. You don't need to unmap it before using its data on the GPU."
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
    if(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(allocator->device, result.memory, 0, result.allocation_size, 0, (void**)&result.mapped_data);
    }

    return(result);
}

/*
=============
vk_allocator_free
=============
*/

void
vk_allocator_free(vulkan_allocator_t *allocator, vulkan_allocation_info_t *info)
{
    if(info->allocation_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkUnmapMemory(allocator->device, info->memory);
    }
    vkFreeMemory(allocator->device, info->memory, allocator->cpu_allocation_callbacks);
    ZeroStruct(*info);
}
