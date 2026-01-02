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
#include <c_string.h>
#include <c_math.h>

#define VkAssert(result) Statement(Assert(result == VK_SUCCESS))
#define INVALID_SWAPCHAIN_IMAGE_INDEX ((u32)-1)

// TODO: TEMPORARY

// NOTE(Sleepster): Nvidia needs 256 byte alignment
typedef struct global_matrix_uniforms
{
    mat4_t view_matrix;
    mat4_t projection_matrix;
}global_matrix_uniforms;

typedef struct push_constant
{
    vec4_t DrawColor;
}push_constant_t;


//////////////////////////////////
// VULKAN BUFFER STUFF 
//////////////////////////////////

typedef struct vulkan_buffer_data
{
    bool8                 is_valid;
    bool8                 is_mapped;
    u64                   buffer_size;

    VkBuffer              handle;
    VkDeviceMemory        device_memory;
    VkBufferUsageFlagBits usage_flags;
    s32                   memory_index;
    u32                   memory_property_flags;
}vulkan_buffer_data_t;

//////////////////////////////////
// VULKAN PIPELINE STUFF 
//////////////////////////////////

typedef struct vulkan_pipeline_data
{
    VkPipeline       handle;
    VkPipelineLayout layout;
}vulkan_pipeline_data_t;

//////////////////////////////////
// VULKAN SHADER STUFF 
//////////////////////////////////
#define MAX_VULKAN_SHADER_STAGES (10)
#define MAX_DESCRIPTOR_TYPES  (SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1)

// TODO(Sleepster): These "set indices" are completely fine for what we want. 
// Set 0 are the "Static" uniforms. These are pieces of information only updated to once a frame. Think "Read Only storage buffers"
// Set 1 are the "Draw" uniforms. Updated once per draw (texture bindings, samplers, camera matrices, etc.)
// Set 2 are the "Instance" uniforms. These are generally items that are updated rapidly. Sometimes per object rendered.
//
// the bindings indices in these standardized uniform sets are not controlled at all.
typedef enum vulkan_shader_uniform_type
{
    // NOTE(Sleepster): "Static" per frame
    SDS_Static   = 0,
    // NOTE(Sleepster): "PerDraw" per draw call
    SDS_Draw     = 1,
    // NOTE(Sleepster): "PerInstance" per object in the scene / vkCmdDrawInstance 
    SDS_Instance = 2,
    SDS_Count
}vulkan_shader_uniform_type_t;

typedef struct vulkan_shader_descriptor_set_info
{
    VkDescriptorSetLayoutBinding *bindings;
    u32                           binding_count;
    u32                           binding_upload_size;

    // NOTE(Sleepster): 3 sets, 1 per "image index" in our triple buffering
    VkDescriptorSet               sets[3];
    vulkan_buffer_data_t          buffer;
}vulkan_shader_descriptor_set_info_t;

typedef struct vulkan_shader_uniform_data
{
    vulkan_shader_uniform_type type;
    void                      *data;
    u32                        size;
}vulkan_shader_uniform_data_t;

typedef struct vulkan_shader_stage_info
{
    VkShaderStageFlagBits                type;
    const char                          *entry_point;
    VkShaderModuleCreateInfo             module_create_info;
    VkPipelineShaderStageCreateInfo      shader_stage_create_info; 

    VkShaderModule                       handle;
}vulkan_shader_stage_info_t;

typedef struct vulkan_shader_data
{
    memory_arena_t                      arena;
    SpvReflectShaderModule              spv_reflect_module;

    vulkan_shader_stage_info_t          stages[MAX_VULKAN_SHADER_STAGES];
    u32                                 stage_count;

    VkDescriptorPool                    primary_pool;
    u32                                 type_counts[MAX_DESCRIPTOR_TYPES];
    
    // NOTE(Sleepster): This has to be like this instead of in the
    // vulkan_shader_stage_info_t because our pipeline wants this information 
    // as an array of VkDescriptorSetLayouts
    VkDescriptorSetLayout               *layouts;

    // NOTE(Sleepster): Group these by the frequency of their updates
    // "Static" per frame (0)
    // "PerDraw" are many times a frame, per draw call (1)
    // "PerInstance" per object in the scene / vkCmdDrawInstance  (2)
    // amounting to 3 different sets, this applies to the layouts as well 
    u32                                  total_descriptor_set_count;
    u32                                  used_descriptor_set_count;
    vulkan_shader_descriptor_set_info_t *set_info;

    u32                                  push_constant_count;
    VkPushConstantRange                 *push_constants;

    // TODO(Sleepster): This will be moved out of here after a while once the asset 
    // system is fully capable of handling a asset -> vulkan piping.  
    vulkan_shader_uniform_data_t        *uniforms;
    u32                                  uniform_count;

    // TODO(Sleepster): Store what kind of pipeline we use, either GRAPHICS or COMPUTE  
    // ideally something like:
    //
    // VkPipelineBindPoint current_bind_point = shader->pipeline.binding_point;
    vulkan_pipeline_data_t              pipeline;

    // TODO(Sleepster): TEMPORARY 
    global_matrix_uniforms camera_matrices;
}vulkan_shader_data_t;

//////////////////////////////////
// VULKAN PHYSICAL DEVICE STUFF 
//////////////////////////////////

#define MAX_VALID_VULKAN_FORMATS (128)
typedef struct vulkan_physical_device_swapchain_support_info
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR       valid_surface_formats[MAX_VALID_VULKAN_FORMATS];
    VkPresentModeKHR         valid_present_modes[MAX_VALID_VULKAN_FORMATS];

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

    VkCommandPool            graphics_command_pool;

    VkDevice                 logical_device;
}vulkan_rendering_device_t; 

//////////////////////////////////
// VULKAN FENCES 
//////////////////////////////////

typedef struct vulkan_fence
{
    VkFence handle;
    bool8   signaled;
}vulkan_fence_t;

//////////////////////////////////
// VULKAN RENDERPASS STUFF 
//////////////////////////////////

typedef enum vulkan_renderpass_state
{
    // NOTE(Sleepster): Not allocated 
    VKRPS_INVALID,

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
// VULKAN FRAMEBUFFER STUFF 
//////////////////////////////////

typedef struct vulkan_framebuffer_data
{
    VkFramebuffer             handle;
    vulkan_renderpass_data_t *renderpass;

    VkImageView              *attachments;
    u32                       attachment_count;
}vulkan_framebuffer_data_t;

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

//////////////////////////////////
// VULKAN SWAPCHAIN 
//////////////////////////////////

typedef struct vulkan_swapchain_data
{
    bool8                      is_valid;
    memory_arena_t             arena;

    VkSwapchainKHR             handle;
    VkSurfaceFormatKHR         image_format;
    VkPresentModeKHR           present_mode;
    vulkan_image_data_t        depth_attachment;
    bool32                     has_depth_attachment;

    u32                        max_frames_in_flight;
    u32                        image_count;

    vulkan_framebuffer_data_t *framebuffers;
    VkImage                   *images;
    VkImageView               *views;
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

    bool8                         is_primary_buffer;
    bool8                         is_single_use;
}vulkan_command_buffer_data_t;

//////////////////////////////////
// VULKAN RENDER CONTEXT 
//////////////////////////////////

typedef struct vulkan_render_context
{
    memory_arena_t                initialization_arena;
    memory_arena_t                frame_arena;
    memory_arena_t                permanent_arena;

    SDL_Window                   *window;
    u32                           window_width;
    u32                           window_height;
        
    VkInstance                    instance;
    VkAllocationCallbacks        *allocators;

    VkSurfaceKHR                  render_surface;
    vulkan_rendering_device_t     rendering_device;

    vulkan_swapchain_data_t       swapchain;
    u32                           current_image_index;
    u32                           current_frame_index;

    u32                           framebuffer_width;
    u32                           framebuffer_height;
    u32                           cached_framebuffer_width;
    u32                           cached_framebuffer_height;

    u32                           current_framebuffer_size_generation;
    u32                           last_framebuffer_size_generation;
    bool8                         recreating_swapchain;

    // NOTE(Sleepster): Semaphores are for GPU -> GPU signaling
    //                  Fences are for CPU -> GPU operation ordering 
    VkSemaphore                  *queue_finished_semaphores;
    VkSemaphore                  *image_avaliable_semaphores;
    vulkan_fence_t               *image_fences;
    vulkan_fence_t              **frame_fences;

    vulkan_command_buffer_data_t *graphics_command_buffers;

    vulkan_renderpass_data_t      main_renderpass;
    vulkan_shader_data_t          default_shader;

    vulkan_buffer_data_t          main_staging_buffer;

    vulkan_buffer_data_t          vertex_buffer;
    vulkan_buffer_data_t          index_buffer;

    u32                           vertex_offset;
    u32                           geometry_index;

    VkDebugUtilsMessengerEXT debug_callback;
}vulkan_render_context_t;

void  r_renderer_init(vulkan_render_context_t *render_context);
void  r_vulkan_physical_device_get_swapchain_support_info(vulkan_render_context_t *context, VkPhysicalDevice device, vulkan_physical_device_swapchain_support_info_t *swapchain_info);
bool8 r_vulkan_physical_device_detect_depth_format(vulkan_physical_device_t *device);

s32   r_vulkan_find_memory_index(vulkan_render_context_t *render_context, u32 type_filter, u32 property_flags);
bool8 r_vulkan_rebuild_swapchain(vulkan_render_context_t *render_context);

vulkan_command_buffer_data_t r_vulkan_command_buffer_acquire_scratch_buffer(vulkan_render_context_t *render_context, VkCommandPool command_pool);

void
r_vulkan_command_buffer_dispatch_scratch_buffer(vulkan_render_context_t      *render_context,
                                                vulkan_command_buffer_data_t *command_buffer,
                                                VkCommandPool                 command_pool,
                                                VkQueue                       queue);
#endif // R_VULKAN_H

