#if !defined(VK_BACKEND_IMAGE_H)
/* ========================================================================
   $File: vk_backend_image.h $
   $Date: February 14 2026 01:43 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define VK_BACKEND_IMAGE_H
#include <vulkan/vulkan.h>
#include <c_types.h>
#include <c_string.h>

#include <vk_backend_allocator.h>

struct vulkan_context_t;
struct vulkan_sampler_info_t
{
    u32    min_filter;
    u32    mag_filter;
    u32    wrapu;
    u32    wrapv;

    bool32 anisotropy_enabled;
    u32    max_anisotropy;

    bool32 compare_enabled;
    u32    compare_operation;

    u32    use_normalized_coordinates;

};

struct vulkan_image_info_t
{
    u32      width;
    u32      height; 
    u32      usage;
    u32      sample_count;
    u32      mip_count;
    u32      type;
    u32      format;
    u32      initial_layout;

    string_t data;
};

struct vulkan_image_t
{
    vulkan_image_info_t      info;
    VkImage                  handle;
    VkImageView              view;
    VkSampler                sampler;
    VkImageLayout            layout;
    VkFormat                 internal_format;
#if 1
    vulkan_allocation_info_t allocation;
#else
    VmaAllocation            gpu_memory;
#endif
    u32                      width;
    u32                      height;
};

vulkan_image_t vk_backend_image_create(vulkan_context_t *vulkan_context, vulkan_image_info_t *image_info);
void           vk_backend_image_update_data(vulkan_context_t *vulkan_context, vulkan_image_t *image);
void           vk_backend_image_destroy(vulkan_context_t *vulkan_context, vulkan_image_t *image);
VkSampler      vk_backend_sampler_create(vulkan_context_t *vulkan_context, vulkan_sampler_info_t *info);
void           vk_backend_sampler_destroy(vulkan_context_t *vulkan_context, VkSampler sampler);

void
vk_backend_image_change_layout(vulkan_context_t       *vulkan_context, 
                               VkCommandBuffer         command_buffer,
                               VkImage                 image, 
                               VkImageLayout           current_layout,
                               VkImageLayout           target_layout, 
                               VkPipelineStageFlags    src_stage_flag,
                               VkPipelineStageFlags    dst_stage_flag,
                               VkAccessFlags           src_access_flags,
                               VkAccessFlags           dst_access_flags,
                               VkImageSubresourceRange range);

#endif // VK_BACKEND_IMAGE_H

