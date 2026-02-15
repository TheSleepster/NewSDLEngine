#if !defined(VK_BACKEND_CORE_H)
/* ========================================================================
   $File: vk_backend_core.h $
   $Date: February 13 2026 10:51 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define VK_BACKEND_CORE_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

// VMA
#include <vk_mem_alloc.h>

#include <c_base.h>
#include <c_types.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_log.h>

#include <vk_backend_image.h>

static const s32 g_device_extension_count = 1;
static const char *g_device_extensions[g_device_extension_count] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const u32 MAX_FRAMES_IN_FLIGHT = 3;

struct gpu_info_t 
{
	VkPhysicalDevice                    device;
	VkPhysicalDeviceProperties          properties;
	VkPhysicalDeviceMemoryProperties    memory_properties;
	VkPhysicalDeviceFeatures            features;
	VkSurfaceCapabilitiesKHR            surface_capabilities;

    DynArray_t(VkSurfaceFormatKHR)      valid_surface_formats;
    DynArray_t(VkPresentModeKHR)        valid_present_modes;
    DynArray_t(VkQueueFamilyProperties)	queue_family_properties;
    DynArray_t(VkExtensionProperties)   extension_properties;
};

struct swapchain_info_t 
{
    VkSwapchainKHR     handle;
    VkPresentModeKHR   present_mode;
    VkSurfaceFormatKHR format;
    VkExtent2D         extent;
};

struct vulkan_context_t 
{
    memory_arena_t           initialization_arena;

    SDL_Window              *window;
    u32                      window_width;
    u32                      window_height;

    VkInstance               instance;
    VkSurfaceKHR             render_surface;

    VkDebugUtilsMessengerEXT debug_messenger;
    VkAllocationCallbacks   *cpu_allocation_callbacks;
    VmaAllocator             vulkan_allocator;

    gpu_info_t               gpu;
    VkDevice                 device;

    s32                      graphics_queue_family_idx;
    s32                      present_queue_family_idx;
    s32                      transfer_queue_family_idx;
    s32                      compute_queue_family_idx;

    VkCommandPool            graphics_command_pool;

    VkQueue	                 graphics_queue;
    VkQueue                  present_queue;
    VkQueue                  transfer_queue;
    VkQueue                  compute_queue;

    VkFormat                 depth_format;
    swapchain_info_t         swapchain;
    vulkan_image_t           depth_buffer;

    VkImage                  swapchain_images[MAX_FRAMES_IN_FLIGHT];
    VkImageView              swapchain_views[MAX_FRAMES_IN_FLIGHT];

    VkCommandBuffer          frame_command_buffer[MAX_FRAMES_IN_FLIGHT];
    VkFence                  frame_command_buffer_fences[MAX_FRAMES_IN_FLIGHT];
    bool32                   frame_command_buffer_recorded[MAX_FRAMES_IN_FLIGHT];

    VkSemaphore              swapchain_image_acquired_semaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore              render_complete_semaphores[MAX_FRAMES_IN_FLIGHT];
};

#define vkAssert(result) ({                                                \
    if(!vk_backend_result_is_success(result))                              \
    {                                                                      \
        Expect(false, "Vulkan Assertion Failed!\nVulkan Error: '%s'...\n", \
               vk_backend_vulkan_result_string(result, true));             \
    }                                                                      \
}) 

void        vk_backend_init(vulkan_context_t *vulkan_context, SDL_Window *window);
const char *vk_backend_vulkan_result_string(VkResult result, bool8 get_extended);
bool8       vk_backend_result_is_success(VkResult result);

#endif // VK_BACKEND_CORE_H

