#if !defined(R_VULKAN_H)
/* ========================================================================
   $File: r_vulkan.h $
   $Date: December 14 2025 05:16 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_VULKAN_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <c_types.h>
#include <c_memory_arena.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_math.h>

#include <r_vulkan_types.h>

typedef struct texture2D texture2D_t;

//#define VkAssert(result) Statement(Assert(result == VK_SUCCESS))
#define vkAssert(result) ({                                                \
    if(!r_vulkan_result_is_success(result))                                \
    {                                                                      \
        Expect(false, "Vulkan Assertion Failed!\nVulkan Error: '%s'...\n", \
               r_vulkan_result_string(result, true));                      \
    }                                                                      \
}) 

void  r_renderer_init(vulkan_render_context_t *render_context);
void  r_vulkan_physical_device_get_swapchain_support_info(vulkan_render_context_t *context, VkPhysicalDevice device, vulkan_physical_device_swapchain_support_info_t *swapchain_info);
bool8 r_vulkan_physical_device_detect_depth_format(vulkan_physical_device_t *device);

s32   r_vulkan_find_memory_index(vulkan_render_context_t *render_context, u32 type_filter, u32 property_flags);
bool8 r_vulkan_rebuild_swapchain(vulkan_render_context_t *render_context);

vulkan_command_buffer_data_t r_vulkan_command_buffer_acquire_scratch_buffer(vulkan_render_context_t *render_context, VkCommandPool command_pool);
vulkan_shader_data_t r_vulkan_shader_create(vulkan_render_context_t *render_context, string_t shader_source);
void r_vulkan_make_gpu_texture(vulkan_render_context_t *render_context, texture2D_t *texture);

void
r_vulkan_command_buffer_dispatch_scratch_buffer(vulkan_render_context_t      *render_context,
                                                vulkan_command_buffer_data_t *command_buffer,
                                                VkQueue                       queue);
#endif // R_VULKAN_H

