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
#include <c_global_context.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_log.h>

#include <vk_backend_image.h>
#include <vk_backend_buffer.h>
#include <vk_backend_allocator.h>

struct image_t;
typedef struct material_archetype material_archetype_t;

constexpr u32 MAX_FRAMES_IN_FLIGHT         = 3;
constexpr u64 MAX_VULKAN_INDEX_BUFFER_SIZE = 600000;
constexpr u64 MAX_VULKAN_INSTANCES         = MAX_VULKAN_INDEX_BUFFER_SIZE / 6;
constexpr u32 MAX_VULKAN_SHADER_STAGES     = 10;
constexpr u32 SWAPCHAIN_MAX_IMAGES         = 10;

constexpr u32 MAX_DESCRIPTOR_SETS          = 16384; 
constexpr u32 MAX_DESCRIPTOR_SET_WRITES    = 32;

// NOTE(Sleepster): Used for rendering
struct alignas(16) render_vertex_t
{
    vec4_t vPosition;
    vec4_t vColor;
    vec2_t vTexCoord;
    vec2_t vPadding;
};

typedef enum renderer_effect_application_flags
{
    REAF_None,
    REAF_Bloom,
    REAF_Emmision,
    REAF_Vignette,
    REAF_FilmGrain,
    REAF_Count,
}renderer_effect_application_flags_t;

typedef enum render_pipeline_blending_mode
{
    RBM_Invalid,
    RBM_Zero,
    RBM_One,
    RBM_Constant,

    RBM_SrcColor,
    RBM_OneMinusSrcColor,
    RBM_DstColor,
    RBM_OneMinusDstColor,

    RBM_SrcAlpha,
    RBM_OneMinusSrcAlpha,
    RBM_DstAlpha,
    RBM_OneMinusDstAlpha,
    RBM_Count
}render_pipeline_blending_mode_t;

typedef enum render_pipeline_blending_equation
{
    RBE_Invalid,
    RBE_Add,
    RBE_Subtract,
    RBE_ReverseSubtract,
    RBE_Min,
    RBE_Max,
}render_pipeline_blending_equation_t;

typedef enum render_pipeline_depth_function
{
    RDF_Invalid,
    RDF_Never,
    RDF_Always,

    RDF_Greater,
    RDF_Less,
    RDF_Equal,
    RDF_NotEqual,
    RDF_LessOrEqual,
    RDF_GreaterOrEqual,
    RDF_Count
}render_pipeline_depth_function_t;

typedef struct render_pipeline_state
{
    bool32 blend_enabled         = true;
    u32    src_color_blend_mode  = RBM_SrcAlpha;
    u32    dst_color_blend_mode  = RBM_OneMinusSrcAlpha;

    u32    src_alpha_blend_mode  = RBM_SrcAlpha;
    u32    dst_alpha_blend_mode  = RBM_OneMinusSrcAlpha;

    u32    color_blend_op        = RBE_Add;
    u32    alpha_blend_op        = RBE_Add;

    bool32 depth_testing_enabled = true;
    bool32 depth_writing_enabled = true;
    u32    depth_func            = RDF_Less;

    bool32 stencil_enabled       = false;
    u32    stencil_state         = 0;
    u32    stencil_keep          = 0;
}render_pipeline_state_t;

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

    VkImage            *images;
    VkImageView        *views;
    VkImageLayout      *image_layouts;
};

struct vulkan_context_t 
{
    memory_arena_t                      initialization_arena;
    memory_arena_t                      swapchain_arena;
    memory_arena_t                      permanent_arena;
    memory_arena_t                      frame_arena;

    SDL_Window                         *window;
    u32                                 current_window_width;
    u32                                 current_window_height;
    u32                                 last_window_width;
    u32                                 last_window_height;
    u64                                 window_size_generation;
    u64                                 last_window_size_generation;

    u64                                 frame_tsc;
    u32                                 current_frame_index;
    u32                                 current_image_index;

    VkInstance                          instance;
    VkSurfaceKHR                        render_surface;

    VkDebugUtilsMessengerEXT            debug_messenger;
    VkAllocationCallbacks              *cpu_allocation_callbacks;
    vulkan_allocator_t                  vulkan_allocator;

    gpu_info_t                          gpu;
    VkDevice                            device;

    s32                                 graphics_queue_family_idx;
    s32                                 present_queue_family_idx;
    s32                                 transfer_queue_family_idx;
    s32                                 compute_queue_family_idx;

    VkCommandPool                       graphics_command_pool;

    VkQueue	                            graphics_queue;
    VkQueue                             present_queue;
    VkQueue                             transfer_queue;
    VkQueue                             compute_queue;

    VkFormat                            depth_format;
    VkSurfaceFormatKHR                  swapchain_format;
    swapchain_info_t                    swapchain;

    vulkan_image_t                     *swapchain_image_data;
    vulkan_image_t                      depth_buffer;

    bool32                              rebuilding_swapchain;

    VkCommandBuffer                    *frame_command_buffers;
    bool32                             *frame_command_buffer_recorded;
    VkFence                            *frame_command_buffer_fences;

    VkSemaphore                        *swapchain_image_acquired_semaphores;
    VkSemaphore                        *render_complete_semaphores;

    VkFence                            *image_render_idle_fences;
    VkFence                           **image_in_flight_fences;
 
    VkFence                           **image_in_flight_fence;
    VkFence                            *image_render_idle_fence;
    VkSemaphore                        *render_complete_semaphore;
    VkSemaphore                        *image_acquired_semaphore;
    VkCommandBuffer                    *render_command_buffer;
    VkFramebuffer                      *render_framebuffer;

    VkRenderPass                        primary_renderpass;
    VkFramebuffer                      *framebuffers;

    // NOTE(Sleepster): Constant Buffers update into here. Essentially, a memory arena.
    vulkan_buffer_t                     constant_buffer_data;
    // NOTE(Sleepster): The staging buffers copy from the above constant_buffer_data into these in a single command. 
    vulkan_buffer_t                     shader_uniform_buffers[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorPool                    descriptor_pools[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSet                     descriptor_sets[MAX_FRAMES_IN_FLIGHT][MAX_DESCRIPTOR_SETS];
    u32                                 descriptor_count;

    // NOTE(Sleepster): Each staging buffer is a large singular buffer with which we stage by incrementing the offset value of the buffer
    // and later uploading at once when we flush the buffer. However, if the buffer gets full before the designated "flush" time, then we
    // flush it ourselves
    vulkan_staging_buffer_t             staging_buffers[MAX_FRAMES_IN_FLIGHT];
    DynArray_t(vulkan_staging_info_t)   staging_infos;
    u32                                 next_staging_info;

    VkCommandPool                       staging_command_pool;
    VkFence                             staging_fences;

    HashTable_t(VkSampler)              image_samplers;
    VkSampler                           default_nearest_sampler;
    VkSampler                           default_linear_sampler;
};

// NOTE(Sleepster): 
//
// These store the default behavior of the API as constants.
global_variable const char *g_device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

global_variable const VkDynamicState g_pipeline_dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_LINE_WIDTH,
};

// These items are used to create a default version of every shader loaded, that way you have an interface beyond
// that of just simply higher-level materials. Materials would fill these out and make the states different as
// needed.
//
// The idea is that if it's a shader that's never been loaded, we have no idea what the state of the pipeline should be.
// So, we just use these defaults instead.
global_variable constexpr VkPipelineRasterizationStateCreateInfo g_pipeline_default_rasterization_state = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
};

global_variable constexpr VkPipelineDepthStencilStateCreateInfo g_pipeline_default_depth_stencil_state = {
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable       = true,
    .depthWriteEnable      = true,
    .depthCompareOp        = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = false,
    .stencilTestEnable     = false,
};

global_variable constexpr VkPipelineColorBlendAttachmentState g_pipeline_default_blend_settings = {
    .blendEnable         = true,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
    .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
};

global_variable constexpr render_pipeline_state_t g_pipeline_default_state_key = {};

struct vulkan_shader_t;
struct renderer_state_t;

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
VkPipeline  vk_backend_create_render_pipeline(vulkan_context_t *vulkan_context, vulkan_shader_t *shader, const VkPipelineRasterizationStateCreateInfo *rasterization_state, const VkPipelineDepthStencilStateCreateInfo *depth_stencil_state, const VkPipelineColorBlendAttachmentState *blend_settings, VkPipelineVertexInputStateCreateInfo *pipeline_vertex_input_state);
void        vk_backend_render_frame(vulkan_context_t *vulkan_context, renderer_state_t *renderer_state);
void        vk_backend_renderpass_destroy(vulkan_context_t *vulkan_context, VkRenderPass renderpass);
void        vk_backend_framebuffer_destroy(vulkan_context_t *vulkan_context, VkFramebuffer framebuffer);
void        vk_backend_renderpass_destroy(vulkan_context_t *vulkan_context, VkRenderPass renderpass);

VkCommandBuffer  vk_backend_get_and_begin_scratch_command_buffer(vulkan_context_t *vulkan_context, bool8 is_primary);
void             vk_backend_submit_and_release_scratch_command_buffer(vulkan_context_t *vulkan_context, VkCommandBuffer *command_buffer);
void             vk_backend_allocate_descriptor_sets(vulkan_context_t *vulkan_context, material_archetype_t *archetype);

void* vk_backend_append_uniform_constant_buffer_data(vulkan_context_t *vulkan_context, void *user_data, u32 data_size, u32 *offset_out);

VkFramebuffer
vk_backend_framebuffer_create(vulkan_context_t  *vulkan_context, 
                              VkRenderPass       renderpass,
                              image_t           *attachments,
                              u32                attachment_count, 
                              u32                width,
                              u32                height);

VkRenderPass
vk_backend_renderpass_create(vulkan_context_t    *vulkan_context, 
                             image_t             *attachments,
                             u32                  attachment_count,
                             VkImageLayout       *initial_layouts,
                             VkImageLayout       *final_layouts,
                             VkAttachmentLoadOp  *load_operations,
                             VkAttachmentStoreOp *store_operations,
                             VkImageLayout       *attachment_types);
#endif // VK_BACKEND_CORE_H

