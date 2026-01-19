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
struct render_state_t;

//#define VkAssert(result) Statement(Assert(result == VK_SUCCESS))
#define vkAssert(result) ({                                                \
    if(!r_vulkan_result_is_success(result))                                \
    {                                                                      \
        Expect(false, "Vulkan Assertion Failed!\nVulkan Error: '%s'...\n", \
               r_vulkan_result_string(result, true));                      \
    }                                                                      \
}) 

void  r_renderer_init(vulkan_render_context_t *render_context, render_state_t *render_state, vec2_t window_size);
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

void r_vulkan_on_resize(vulkan_render_context_t *render_context, vec2_t new_window_size);

void r_vulkan_shader_set_uniform_data(asset_handle_t *shader_handle, string_t uniform_name, void *data, u64 data_size);
void r_vulkan_shader_uniform_update_data(vulkan_shader_data_t *shader, string_t uniform_name, void *data);
void r_vulkan_shader_uniform_update_texture(vulkan_shader_data_t *shader, string_t texture_name, vulkan_texture_t *texture);
void r_vulkan_shader_assign_vulkan_buffer(vulkan_shader_data_t *shader, string_t uniform_name, vulkan_buffer_data_t *buffer);

vulkan_shader_uniform_data_t *r_vulkan_shader_get_uniform(asset_handle_t *shader_handle, string_t uniform_name);

bool8 r_vulkan_begin_frame(vulkan_render_context_t *render_context, render_state_t *render_state, float32 delta_time);
bool8 r_vulkan_end_frame(vulkan_render_context_t *render_context, render_state_t *render_state, float32 delta_time);

// NOTE(Sleepster): The goal of this is that you only have to set the data pointer a single time, 
//                  and just update that pointer every frame, not needing to query for the uniform every frame.
//void  r_vulkan_shader_set_uniform_data(vulkan_shader_data_t *shader, string_t uniform_name, void *data, u64 data_size);

#endif // R_VULKAN_H

