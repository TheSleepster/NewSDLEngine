#if !defined(S_RENDERER_H)
/* ========================================================================
   $File: s_renderer.h $
   $Date: February 23 2026 06:24 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_RENDERER_H

#include <c_base.h>
#include <c_types.h>
#include <c_globals.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <c_log.h>

#include <vk_backend_core.h>
#include <s_asset_manager.h>

constexpr u32 MAX_RENDER_TARGET_ATTACHMENTS = 10;

// NOTE(Sleepster): The memory for each of these is transient, don't rely 
// on them sticking around between frames...
struct constant_buffer_t
{
    void *data;
    u32   buffer_size;
};

struct render_instance_t
{
};

constexpr u32 MAX_RENDER_COMMANDS  = 10000;
constexpr u32 MAX_COMMAND_LISTS    = 100;
constexpr u32 MAX_CONSTANT_BUFFERS = 100;

enum image_type_t
{
    IMAGE_TYPE_Texture,
    IMAGE_TYPE_ColorAttachment,
    IMAGE_TYPE_DepthStencilAttachment
};

struct image_create_info_t
{
    string_t data;
    u32      width;
    u32      height;
    u32      image_type;
    u32      format;

    bool8    color_attachment; 
    bool8    depth_stencil_attachment;
    bool8    use_device_depth_format;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct image_t
{
    union {
        vulkan_image_t vulkan_image;
    };
};

enum render_target_attachment_load_operations_t
{
    RTALO_Clear,
    RTALO_Load,
    RTALO_DontCare
};

enum render_target_attachment_store_operations_t
{
    RTASO_Store,
    RTASO_DontCare
};

struct render_target_create_info_t
{
    image_t       *attachments;
    u32            attachment_count;

    VkImageLayout *attachment_type;

    VkImageLayout *attachment_initial_format;
    VkImageLayout *attachment_final_format;

    u32           *attachment_load_operations;
    u32           *attachment_store_operations;
    VkClearValue  *attachment_clear_values;
    
    u32            width;
    u32            height;
};

struct render_target_t
{
    VkFramebuffer framebuffer;
    VkRenderPass  renderpass;

    VkClearValue  clear_values[MAX_RENDER_TARGET_ATTACHMENTS];
};

enum render_command_type_t
{
    RCT_Invalid,

    RCT_BeginRenderGroup,
    RCT_RenderInstance,
    RCT_BufferUpdate,
    RCT_TextureUpdate,
    RCT_SetRenderTarget,
    RCT_SetMaterial,
    RCT_SetShader,
    RCT_EndRenderGroup,

    RCT_Count
};

struct render_command_t
{
    render_command_type_t command_type;
    union {
        render_instance_t instance;
        constant_buffer_t buffer;
    };
};

struct render_command_list_t
{
    render_command_t commands[MAX_RENDER_COMMANDS];
    u32              command_count;
};

// TODO(Sleepster): Maybe one day we'll have to have this store backend related function pointers like:
//
// renderer_state->r_backend_create_texture(...);
//
// But for now we're good.
struct renderer_state_t
{
    memory_arena_t        renderer_arena;
    memory_arena_t        transient_arena;

    void                 *render_context;

    render_command_list_t command_lists[MAX_COMMAND_LISTS];
    u32                   command_list_count;

    constant_buffer_t     constant_buffers[MAX_CONSTANT_BUFFERS];
    u32                   used_constant_buffers;

    u32                   total_render_instances;
};

#endif // S_RENDERER_H

