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

#include <c_types.h>
#include <c_math.h>

#define VkAssert(result) Statement(Assert(result == VK_SUCCESS))
#define INVALID_SWAPCHAIN_IMAGE_INDEX ((u32)-1)

//////////////////////////////////
// VULKAN PHYSICAL DEVICE STUFF 
//////////////////////////////////

typedef struct vulkan_physical_device_swapchain_support_info
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR      *valid_surface_formats;
    VkPresentModeKHR        *valid_present_modes;

    u32                      valid_surface_format_count;
    u32                      valid_present_mode_count;
}vulkan_physical_device_swapchain_support_info_t;

typedef struct vulkan_physical_device_queue_info
{
    u32 graphics_queue_family_index;
    u32 present_queue_family_index;
    u32 compute_queue_family_index;
    u32 transfer_queue_family_index;
}vulkan_physical_device_queue_info_t;

typedef struct vulkan_physical_device_requirements
{
    bool8        has_graphics_queue;
    bool8        has_present_queue;
    bool8        has_transfer_queue;
    bool8        has_compute_queue;

    const char **required_extensions;
}vulkan_physical_device_requirements_t;

typedef struct vulkan_physical_device_data
{
    VkPhysicalDevice                                handle;
    VkFormat                                        device_depth_format;
    VkPhysicalDeviceProperties                      properties;
    VkPhysicalDeviceFeatures                        features;
    VkPhysicalDeviceMemoryProperties                memory_properties;
    vulkan_physical_device_swapchain_support_info_t swapchain_support_info;
}vulkan_physical_device_t;

typedef struct vulkan_rendering_device
{
    vulkan_physical_device_t physical_device;

    u32                      graphics_queue_family_index;
    u32                      present_queue_family_index;
    u32                      compute_queue_family_index;
    u32                      transfer_queue_family_index;
    
    VkQueue                  graphics_queue;
    VkQueue                  present_queue;
    VkQueue                  transfer_queue;
    VkQueue                  compute_queue;

    VkDevice                 logical_device;
}vulkan_rendering_device_t; 

//////////////////////////////////
// VULKAN IMAGE STUFF 
//////////////////////////////////

typedef struct vulkan_image_data
{
    VkImage        handle;
    VkDeviceMemory memory;
    VkFormat       format;

    VkImageView    view;

    u32            width;
    u32            height;
}vulkan_image_data_t;

typedef struct vulkan_swapchain_data
{
    VkSwapchainKHR      handle;
    VkSurfaceFormatKHR  image_format;
    VkPresentModeKHR    present_mode;
    vulkan_image_data_t depth_attachment;
    bool32              has_depth_attachment;

    u32                max_frames_in_flight;
    u32                image_count;

    VkImage           *images;
    VkImageView       *views;
}vulkan_swapchain_data_t;

//////////////////////////////////
// VULKAN COMMAND BUFFER STUFF 
//////////////////////////////////

typedef enum vulkan_command_buffer_state
{
    VKCBS_INVALID,
    VKCBS_NOT_ALLOCATED,
    VKCBS_READY,
    VKCBS_RECORDING,
    VKCBS_WITHIN_RENDERPASS,
    VKCBS_RECORDING_ENDED,
    VKCBS_SUBMITTED,
    VKCBS_COUNT,
}vulkan_command_buffer_state_t;

typedef struct vulkan_command_buffer_data
{
    VkCommandBuffer               handle;
    vulkan_command_buffer_state_t state;
}vulkan_command_buffer_data_t;

//////////////////////////////////
// VULKAN RENDERPASS STUFF 
//////////////////////////////////

typedef enum vulkan_renderpass_state
{
    VKRPS_INVALID,
    VKRPS_NOT_ALLOCATED,
    VKRPS_RECORDING,
    VKRPS_WITHIN_RENDERPASS,
    VKRPS_RECORDING_ENDED,
    VKRPS_COMMANDS_SUBMITTED,

    VKRPS_COUNT,
}vulkan_renderpass_state_t;

typedef struct vulkan_renderpass_data
{
    VkRenderPass              handle;
    vec2_t                    offset;
    vec2_t                    size;
    vec4_t                    clear_color;
    float32                   depth_clear;
    u32                       stencil_clear;

    vulkan_renderpass_state_t renderpass_state;
}vulkan_renderpass_data_t;


//////////////////////////////////
// VULKAN RENDER CONTEXT 
//////////////////////////////////

typedef struct vulkan_render_context
{
    SDL_Window               *window;
    u32                       window_width;
    u32                       window_height;
        
    VkInstance                instance;
    VkAllocationCallbacks    *allocators;

    VkSurfaceKHR              render_surface;
    vulkan_rendering_device_t rendering_device;

    vulkan_swapchain_data_t   swapchain;
    u32                       current_image_index;
    u32                       current_frame_index;
    u32                       framebuffer_width;
    u32                       framebuffer_height;
    bool8                     recreating_swapchain;

    vulkan_renderpass_data_t  main_renderpass;

    VkDebugUtilsMessengerEXT debug_callback;
}vulkan_render_context_t;

void  r_renderer_init(vulkan_render_context_t *render_context);
void  r_vulkan_physical_device_get_swapchain_support_info(vulkan_render_context_t *context, VkPhysicalDevice device, vulkan_physical_device_swapchain_support_info_t *swapchain_info);
bool8 r_vulkan_physical_device_detect_depth_format(vulkan_physical_device_t *device);

s32  r_vulkan_find_memory_index(vulkan_render_context_t *render_context, u32 type_filter, u32 property_flags);
#endif // R_VULKAN_H

