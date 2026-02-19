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

#include <c_base.h>
#include <c_types.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_log.h>

#include <vk_backend_image.h>
#include <vk_backend_buffer.h>
#include <vk_backend_allocator.h>

static const u32 MAX_FRAMES_IN_FLIGHT     = 3;
static const s32 g_device_extension_count = 1;

static const char *g_device_extensions[g_device_extension_count] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct gpu_info_t 
{
	VkPhysicalDevice                    device;
	VkPhysicalDeviceProperties          properties;
	VkPhysicalDeviceMemoryProperties    memory_properties;
	VkPhysicalDeviceFeatures            features;
	VkSurfaceCapabilitiesKHR            surface_capabilities;

    u32                                 queue_family_count;
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
    u32                image_count;
};

struct vulkan_context_t 
{
    memory_arena_t                     initialization_arena;
    memory_arena_t                     swapchain_arena;
    memory_arena_t                     permanent_arena;
    memory_arena_t                     frame_arena;

    SDL_Window                        *window;
    u32                                current_window_width;
    u32                                current_window_height;
    u32                                last_window_width;
    u32                                last_window_height;
    u64                                window_size_generation;
    u64                                last_window_size_generation;

    u32                                current_frame_index;
    u32                                current_image_index;

    VkInstance                         instance;
    VkSurfaceKHR                       render_surface;

    VkDebugUtilsMessengerEXT           debug_messenger;
    VkAllocationCallbacks             *cpu_allocation_callbacks;
#if 0
    VmaAllocator                       vulkan_allocator;
#else
    vulkan_allocator_t                 vulkan_allocator;
#endif

    gpu_info_t                         gpu;
    VkDevice                           device;

    s32                                graphics_queue_family_idx;
    s32                                present_queue_family_idx;
    s32                                transfer_queue_family_idx;
    s32                                compute_queue_family_idx;

    VkCommandPool                      graphics_command_pool;

    VkQueue	                           graphics_queue;
    VkQueue                            present_queue;
    VkQueue                            transfer_queue;
    VkQueue                            compute_queue;

    VkFormat                           depth_format;
    swapchain_info_t                   swapchain;
    VkImage                           *swapchain_images;
    VkImageView                       *swapchain_views;
    vulkan_image_t                     depth_buffer;

    bool32                             rebuilding_swapchain;

    VkCommandBuffer                   *frame_command_buffers;
    bool32                            *frame_command_buffer_recorded;
    VkFence                           *frame_command_buffer_fences;

    VkSemaphore                       *swapchain_image_acquired_semaphores;
    VkSemaphore                       *render_complete_semaphores;

    VkFence                           *image_render_idle_fences;
    VkFence                          **image_in_flight_fences;

    VkFence                          **image_in_flight_fence;
    VkFence                           *image_render_idle_fence;
    VkSemaphore                       *render_complete_semaphore;
    VkSemaphore                       *image_acquired_semaphore;
    VkCommandBuffer                   *render_command_buffer;
    VkFramebuffer                     *render_framebuffer;

    VkDescriptorPool                   descriptor_pool;

    VkRenderPass                       primary_renderpass;
    VkFramebuffer                     *framebuffers;

    // NOTE(Sleepster): We don't really need these, they're static and only modified when created. 
    vulkan_buffer_t                    main_vertex_buffer;
    vulkan_buffer_t                    main_index_buffer;
    vulkan_buffer_t                    main_instance_buffer[MAX_FRAMES_IN_FLIGHT];
    vulkan_buffer_t                    frame_render_buffer[MAX_FRAMES_IN_FLIGHT];

    // NOTE(Sleepster): Each staging buffer is a large singular buffer with which we stage by incrementing the offset value of the buffer
    // and later uploading at once when we flush the buffer. However, if the buffer gets full before the designated "flush" time, then we
    // flush it ourselves
    vulkan_staging_buffer_t           staging_buffers[MAX_FRAMES_IN_FLIGHT];
    DynArray_t(vulkan_staging_info_t) staging_infos;
    u32                               next_staging_info;
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
void        vk_backend_handle_window_resize(vulkan_context_t *vulkan_context, vec2_t window_size);
void        vk_backend_render_frame(vulkan_context_t *vulkan_context);

#endif // VK_BACKEND_CORE_H

