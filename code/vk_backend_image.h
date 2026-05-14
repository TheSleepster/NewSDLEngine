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
#include <c_math.h>

#include <vk_backend_allocator.h>

struct image_create_info_t;
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

    bool32 use_normalized_coordinates;
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
    u32      final_layout;
    string_t data;

    vulkan_sampler_info_t *sampler_info;
};

struct vulkan_image_t
{
    bool8                    is_valid;
    vulkan_image_info_t      info;
    VkImage                  handle;
    VkImageView              view;
    VkSampler                sampler;
    VkImageLayout            layout;
    VkImageLayout            renderpass_initial_layout;
    VkImageLayout            renderpass_final_layout;
    VkFormat                 internal_format;
    VkImageAspectFlags       aspect_mask;
    vulkan_allocation_info_t allocation;

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

vulkan_image_t
vk_backend_image_init_from_image_handle(vulkan_context_t    *vulkan_context, 
                                        VkImage              image, 
                                        VkImageView         *view,
                                        vulkan_image_info_t *info);

void
vk_backend_image_blit(vulkan_context_t       *vulkan_context, 
                      vulkan_image_t         *source_image, 
                      vulkan_image_t         *destination_image, 
                      vec2_t                  source_offset,
                      vec2_t                  source_blit_size,
                      vec2_t                  destination_offset,
                      vec2_t                  destination_size,
                      VkImageLayout           source_initial_layout,
                      VkImageLayout           destination_initial_layout,
                      VkImageLayout           destination_final_layout,
                      VkImageSubresourceRange source_range, 
                      VkImageSubresourceRange destination_range);

void  vk_backend_image_ensure_shader_readonly_optimal(vulkan_context_t *vulkan_context, vulkan_image_t *image);
bool8 vk_backend_is_image_format_stencil_format(vulkan_image_t *image);
bool8 vk_backend_is_image_format_depth_format(vulkan_image_t *image);

void vk_backend_transfer_image_to_intial_layout(vulkan_context_t *vulkan_context, VkCommandBuffer render_command_buffer, vulkan_image_t *image);
void vk_backend_transfer_image_to_final_layout(vulkan_context_t *vulkan_context, VkCommandBuffer render_command_buffer, vulkan_image_t *image);

VkFormat          vk_bitmap_format_to_vulkan_format(u32 bitmap_format);
VkImageUsageFlags vk_image_usage_flags_from_image_format(u32 format);
bool8             vk_sampler_info_is_valid(image_create_info_t *create_info);
VkFilter          vk_sampler_filter_type_to_vk_filter(u32 filter);
bool8             vk_is_depth_format(u32 format);
VkImageLayout     vk_get_image_initial_layout_from_usage(u32 usage, u32 format);
VkImageLayout     vk_get_image_final_layout_from_usage(u32 usage, u32 format);
     
#endif // VK_BACKEND_IMAGE_H

