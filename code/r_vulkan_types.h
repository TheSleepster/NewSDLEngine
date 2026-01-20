#if !defined(R_VULKAN_TYPES_H)
/* ========================================================================
   $File: r_vulkan_types.h $
   $Date: January 13 2026 11:21 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_VULKAN_TYPES_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <c_types.h>
#include <c_memory_arena.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_math.h>

// TODO(Sleepster): I hate that these defines are here. But I can't put them anywhere else 
#define MAX_RENDER_GROUPS                    (192)
#define MAX_HASHED_RENDER_GROUPS             (4093)

#define MAX_RENDER_GROUP_BUFFER_VERTEX_COUNT (2500)
#define MAX_RENDER_GROUP_VERTEX_COUNT        (MAX_RENDER_GROUP_BUFFER_VERTEX_COUNT * 4)
#define MAX_RENDER_GROUP_CAMERA_COUNT        (64)
#define MAX_RENDER_GROUP_BOUND_TEXTURES      (16)

// NOTE(Sleepster): This seems comically large. But it's literally 20MB
#define MAX_VULKAN_INDEX_BUFFER_SIZE         (600000)
#define MAX_VULKAN_INSTANCES                 (MAX_VULKAN_INDEX_BUFFER_SIZE / 6)

// NOTE(Sleepster): "Frame in flight" is not a requirement, but it's a cap.
#define VULKAN_MAX_FRAMES_IN_FLIGHT (3)

// NOTE(Sleepster): Used for rendering
struct alignas(0) vertex_t
{
    vec4_t vPosition;
    vec2_t vCorner;
    vec2_t vPadding;
};

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

// NOTE(Sleepster): The view and projection matrix used for rendering, this is per geometry_buffer to prevent an explosion of render_group counts.
//                  If a piece of geometry has a different set of camera_matrices, but the rest of the data is identical, we simply allocate a new 
//                  render_geometry_buffer_t and attach it to the render_group.
struct render_camera_t 
{
    mat4_t view_matrix;
    mat4_t projection_matrix;

    u64 ID;
};

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

// TODO(Sleepster): https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineBindPoint.html
typedef struct vulkan_pipeline_data
{
    VkPipeline          handle;
    VkPipelineBindPoint binding;
    VkPipelineLayout    layout;
}vulkan_pipeline_data_t;

//////////////////////////////////
// VULKAN SHADER STUFF 
//////////////////////////////////
#define MAX_VULKAN_SHADER_STAGES (10)
#define MAX_DESCRIPTOR_TYPES     (SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1)

typedef struct spv_vulkan_type_map 
{
    SpvReflectDescriptorType spv_type;
    VkDescriptorType         vk_type;
}spv_vulkan_type_map_t;

global_variable spv_vulkan_type_map_t type_map[] = {
    {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER,                VK_DESCRIPTOR_TYPE_SAMPLER               },
    {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         },
    {SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        },
    {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        },
    {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         },
    {SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
};

// TODO(Sleepster): These "set indices" are completely fine for what we want. 
// Set 0 are the "Static" uniforms. These are pieces of information only updated to once a frame. Think "Read Only storage buffers"
// Set 1 are the "Draw" uniforms. Updated once per draw (texture bindings, samplers, camera matrices, etc.)
// Set 2 are the "Instance" uniforms. These are generally items that are updated rapidly. Sometimes per object rendered.
//
// the bindings indices in these standardized uniform sets are not controlled at all. 
typedef enum vulkan_shader_descriptor_set_type
{
    // NOTE(Sleepster): "Static" per frame
    SDS_Static   = 0,
    // NOTE(Sleepster): "PerDraw" per draw call
    SDS_Draw     = 1,
    // NOTE(Sleepster): "PerInstance" per object in the scene / vkCmdDrawInstance 
    SDS_Instance = 2,
    SDS_Count
}vulkan_shader_descriptor_set_binding_type_t;

typedef struct vulkan_shader_descriptor_set_info
{
    bool8                         is_valid;
    u32                           set_type;

    VkDescriptorSetLayoutBinding *bindings;
    u32                           binding_count;
    u32                           binding_upload_size;

    VkImageView                   image_views[MAX_RENDER_GROUP_BOUND_TEXTURES];
    VkSampler                     samplers[MAX_RENDER_GROUP_BOUND_TEXTURES];
    u32                           image_count;
    u32                           sampler_count;

    // NOTE(Sleepster): 3 sets, 1 per "image index" in our triple buffering
    VkDescriptorSet               sets[VULKAN_MAX_FRAMES_IN_FLIGHT];
    vulkan_buffer_data_t          uniform_buffer;
    u32                           single_frame_uniform_buffer_size;
}vulkan_shader_descriptor_set_info_t;

// NOTE(Sleepster): Uniform types

typedef struct vulkan_shader_uniform_texture_data
{
    VkImageView image_views[MAX_RENDER_GROUP_BOUND_TEXTURES];
    VkSampler   image_samplers[MAX_RENDER_GROUP_BOUND_TEXTURES];

    u32         image_counter;
}vulkan_shader_uniform_texture_data_t;

// NOTE(Sleepster): Uniform types

typedef struct vulkan_shader_uniform_data_range
{
    void *data;
    u64   data_size;
}vulkan_shader_uniform_data_range_t;

typedef struct vulkan_shader_uniform_data
{
    u32                                         owner_shader_id;
    VkDescriptorType                            uniform_type;
    vulkan_shader_descriptor_set_binding_type_t set_type;
    u32                                         uniform_location;
    u32                                         push_constant_index;

    string_t                                    name;
    u32                                         uniform_size;
    bool8                                       is_texture;

    union 
    {
        struct {
            u32                    mapped_buffer_update_size;
            void                  *mapped_uniform_buffer;
            vulkan_buffer_data_t   storage_buffer;
        };

        vulkan_shader_uniform_texture_data_t texture_data;
    };

#if 0
    void                                       *data;
    VkImageView                                 image_view;
    VkSampler                                   sampler;
#endif
}vulkan_shader_uniform_data_t;

typedef struct vulkan_shader_stage_info 
{
    VkShaderStageFlagBits                type;
    const char                          *entry_point;
    VkShaderModuleCreateInfo             module_create_info;
    VkPipelineShaderStageCreateInfo      shader_stage_create_info; 

    VkShaderModule                       handle;
}vulkan_shader_stage_info_t;

// TODO(Sleepster): The shader should store what kind of pipeline bind point it needs
typedef struct vulkan_shader_data
{
    // TODO(Sleepster): Shader catalog will handle this... 
    u32                                 shader_id;
    string_t                            name;

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
    VkPushConstantRange                 *push_constant_data;

    vulkan_shader_uniform_data_t        *uniforms;
    u32                                  uniform_count;

    vulkan_shader_uniform_data_t       **static_uniforms;
    u32                                  static_uniform_count;

    vulkan_shader_uniform_data_t       **draw_uniforms;
    u32                                  draw_uniform_count;

    vulkan_shader_uniform_data_t       **instance_uniforms;
    u32                                  instance_uniform_count;

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
    VkImageLayout  layout;

    VkImageView    view;

    u32            width;
    u32            height;
}vulkan_image_data_t;


// TODO: TEMPORARY

typedef struct vulkan_image_data vulkan_image_data_t;
typedef struct vulkan_texture
{
    // TODO(Sleepster): this stuff can go into the actual texture, but the VkSampler and the imagedata can stay. 
    u32                 width;
    u32                 height;
    u32                 channel_count;
    u32                 current_generation;

    vulkan_image_data_t image_data;
    VkSampler           sampler;
}vulkan_texture_t;
// TODO: TEMPORARY

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
    VkCommandPool                 owner_pool;
    vulkan_command_buffer_state_t state;

    bool8                         is_primary_buffer;
    bool8                         is_single_use;
}vulkan_command_buffer_data_t;

//////////////////////////////////
// VULKAN RENDER CONTEXT 
//////////////////////////////////

// TODO(Sleepster): This maybe??? 
typedef struct vulkan_render_backend_function_data
{
    void *create_gpu_texture;
    void *create_gpu_shader;
    void *create_gpu_buffer;

    void *destroy_gpu_texture;
    void *destroy_gpu_shader;
    void *destroy_gpu_buffer;
}vulkan_render_backend_function_data_t;

typedef struct vulkan_render_frame_state
{
    vulkan_fence_t               *image_render_idle_fence;
    vulkan_fence_t              **frame_in_flight_fence_ptr;
    VkSemaphore                  *image_avaliable_semaphore;
    VkSemaphore                  *presentation_complete_semaphore;

    vulkan_framebuffer_data_t    *current_framebuffer;
    vulkan_command_buffer_data_t *render_command_buffer;

    vulkan_buffer_data_t         *instanced_rendering_buffer;

    vulkan_shader_data_t         *bound_shader;
}vulkan_render_frame_state_t;

// TODO(Sleepster): Get rid of this crap
typedef struct asset_handle asset_handle_t;
typedef struct texture2D    texture2D_t;
////////////////////

typedef struct vulkan_render_context
{
    memory_arena_t                initialization_arena;
    memory_arena_t                frame_arena;
    memory_arena_t                permanent_arena;

    // NOTE(Sleepster): Double or triple buffering 
    u32                           additional_buffer_count;

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
    vulkan_fence_t               *image_render_idle_fences;
    // NOTE(Sleepster): Tells the CPU it can use the specific image index for rendering 
    vulkan_fence_t              **frame_in_flight_fence_ptrs;
    // NOTE(Sleepster): This tells us the image is avaliable from the swapchain for presenting 
    VkSemaphore                  *image_avaliable_semaphores;
    // NOTE(Sleepster): This tells us if the swapchain is done presenting that image 
    VkSemaphore                  *presentation_complete_semaphores;
    vulkan_command_buffer_data_t *render_command_buffers;

    vulkan_render_frame_state_t  *frames;
    vulkan_render_frame_state_t  *current_frame;

    vulkan_buffer_data_t          instanced_rendering_buffer[VULKAN_MAX_FRAMES_IN_FLIGHT];
    vulkan_buffer_data_t          index_buffer;

    vulkan_buffer_data_t          vertex_buffer;

    vulkan_renderpass_data_t      main_renderpass;
    asset_handle_t               *default_shader;
    asset_handle_t               *default_texture;

    texture2D_t                  *invalid_texture_data;

    u32                           vertex_offset;
    u32                           geometry_index;

    render_camera_t               test_camera;

    VkDebugUtilsMessengerEXT debug_callback;
}vulkan_render_context_t;

#endif // R_VULKAN_TYPES_H

