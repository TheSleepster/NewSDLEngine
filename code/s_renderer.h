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
#include <c_math.h>

#include <vk_backend_core.h>
#include <s_asset_manager.h>

constexpr u32 MAX_RENDER_TARGET_ATTACHMENTS = 10;

constexpr u32 MAX_RENDER_COMMANDS  = 10000;
constexpr u32 MAX_COMMAND_LISTS    = 1000;
constexpr u32 MAX_CONSTANT_BUFFERS = 1000;
constexpr u32 MAX_RENDER_TARGETS   = 100;

enum image_type_t
{
    IMAGE_TYPE_Undefined              = 0,
    IMAGE_TYPE_General                = 1,
    IMAGE_TYPE_ColorAttachment        = 2,
    IMAGE_TYPE_DepthStencilAttachment = 3,
    IMAGE_TYPE_DepthStencilReadOnly   = 4,
    IMAGE_TYPE_ShaderReadOnly         = 5,
    IMAGE_TYPE_TransferSrcImage       = 6,
    IMAGE_TYPE_TransferDstImage       = 7,
    IMAGE_TYPE_Preinitialized         = 8,
    IMAGE_TYPE_PresentSRC             = 1000001002,
};

struct image_create_info_t
{
    string_t data;
    u32      width;
    u32      height;
    u32      image_type;
    u32      format;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct image_t
{
    image_create_info_t create_jnfo;
    union {
        vulkan_image_t vulkan_image;
    };
};

enum render_target_attachment_load_operations_t
{
    RTALO_Load     = 0,
    RTALO_Clear    = 1,
    RTALO_DontCare = 2
};

enum render_target_attachment_store_operations_t
{
    RTASO_Store    = 0,
    RTASO_DontCare = 1
};

struct clear_value_t
{
    float32 clear_depth;
    u32     clear_stencil;
    union {
        vec4_t  float_color;
        ivec4_t int_color;
        u32     uint_color[4];
    }clear_color;
};

struct render_target_attachment_info_t
{
    image_t      *attachment;
    u32           ID;
    u32           attachment_type;

    u32           initial_layout;
    u32           final_layout;

    u32           load_operation;
    u32           store_operation;

    clear_value_t clear_value;
};

struct render_target_create_info_t
{
    render_target_attachment_info_t *attachments;
    u32                              attachment_count;
    bool32                           resize_with_window;

    u32                              width;
    u32                              height;
};

struct render_target_t
{
    u32                             ID;
    bool32                          resize_with_window;
    render_target_create_info_t     create_info; 

    VkFramebuffer                   framebuffer;
    VkRenderPass                    renderpass;

    image_t                        *primary_color_buffer;
    image_t                        *depth_buffer;

    render_target_attachment_info_t attachment_info[MAX_RENDER_TARGET_ATTACHMENTS];
    u32                             attachment_count;

    VkClearValue                    clear_values[MAX_RENDER_TARGET_ATTACHMENTS];
};

enum render_command_type_t
{
    RCT_Invalid,

    RCT_ClearRenderTarget,
    RCT_BeginRenderGroup,
    RCT_DrawRectangle,
    RCT_DrawBitmap,
    RCT_UpdateTexture,
    RCT_UpdateBuffer,
    RCT_BindRenderTarget,
    RCT_BindMaterial,
    RCT_BindShader,
    RCT_EndRenderGroup,
    RCT_BlitToRenderTarget,
    RCT_PresentFrame,

    RCT_Count
};

// NOTE(Sleepster): The memory for each of these is transient, don't rely 
// on them sticking around between frames...
struct constant_buffer_t
{
    void *data;
    u32   buffer_size;
};

// TODO(Sleepster): Handle layer data here.
// Layers should be seperate from Z in case we do a 2.5D game
struct render_instance_t
{
    mat4_t transform;
    vec4_t color;
    vec2_t uv_min;
    vec2_t uv_max;
    u32    bitmap_index;
    u32    material_index;
    u32    matrix_index;
};

struct geometry_data_t
{
    vec2_t  position;
    vec2_t  size;
    vec4_t  render_color;
    float32 rotation;
};

struct render_command_header_t
{
    render_command_type_t command_type;
};

struct render_command_clear_render_target_t 
{
    render_command_header_t header;
    render_target_t        *render_target;
};

struct render_command_bind_render_target_t
{
    render_command_header_t header;
    u32                     render_groupID;

    render_target_t        *render_target;
};

struct render_command_begin_render_group_t
{
    render_command_header_t header;
};

struct render_command_draw_bitmap_t
{
    render_command_header_t header;
    u32                     instanceID;

    geometry_data_t         quad_data;
    asset_handle_t          bitmap;
};

struct render_command_draw_rectangle_t
{
    render_command_header_t header;
    u32                     instanceID;

    geometry_data_t         quad_data;
};

struct render_command_update_constant_buffer_t
{
    render_command_header_t header;
    u32                     bufferID;

    constant_buffer_t       buffer;
};

struct render_command_update_texture_t
{
    render_command_header_t header;

    asset_handle_t          bitmap;
    constant_buffer_t       buffer;
};

struct render_command_bind_material_t
{
    render_command_header_t header;
    u32                     materialID;

    asset_handle_t          material;
};

struct render_command_bind_shader_t
{
    render_command_header_t header;
    u32                     shaderID;

    asset_handle_t          shader;
};

struct render_command_end_render_group_t
{
    render_command_header_t header;
};

// NOTE(Sleepster):
// For this function, we will blit each of the render targets from their indices in the source_target to their indices in the desination target.
// Meaning that an attachment that is in slot 0 of the source target will be blit to slot 0 of the destination target. 
// If that slot in the destination target does not exist, we will skip it preventing a crash.
struct render_command_blit_info_t
{
    render_target_t *source;
    render_target_t *destination;
    
    vec2_t           source_offset;
    vec2_t           destination_offset;
    vec2_t           source_size;
    vec2_t           destination_size;
};

struct render_command_blit_render_target_t
{
    render_command_header_t    header;
    render_command_blit_info_t info;
};

struct render_command_present_frame_t
{
    render_command_header_t header;
    render_target_t        *presentation_target;
};

struct render_command_t
{
    render_command_header_t header;
    union {
        render_command_clear_render_target_t    clear_render_target;
        render_command_bind_render_target_t     bind_render_target; 
        render_command_begin_render_group_t     begin_render_group;
        render_command_draw_rectangle_t         draw_rectangle;
        render_command_draw_bitmap_t            draw_texture;
        render_command_update_constant_buffer_t update_constant_buffer;
        render_command_update_texture_t         update_texture_contents;
        render_command_bind_material_t          bind_material;
        render_command_bind_shader_t            bind_shader;
        render_command_end_render_group_t       end_render_group;
        render_command_blit_render_target_t     blit_render_target;
        render_command_present_frame_t          present_frame;
    };
};

// NOTE(Sleepster): For some stupid fucking reason this has to be heap allocated instead of 
// just stored normally like a normal object. My compiler is an autistic chimp
struct render_command_list_t
{
    bool8            is_initialized;
    // NOTE(Sleepster): Everything within this is reset once all commands are executed 
    memory_arena_t   transient_arena;
    memory_arena_t   command_arena;

    render_command_t *commands;
    u32               command_count;

    u32               draw_instance_command_count;
    u32               bind_shader_command_count;
    u32               bind_render_target_command_count;
    u32               bind_material_command_count;

    render_target_t  *active_render_target;
    bool32            presenting;
};

// TODO(Sleepster): Maybe one day we'll have to have this store backend related function pointers like:
//
// renderer_state->r_backend_create_bitmap(...);
//
// But for now we're good.
struct renderer_state_t
{
    memory_arena_t         renderer_arena;
    memory_arena_t         transient_arena;

    void                  *render_context;

    render_command_list_t *command_lists;
    u32                    command_list_count;

    constant_buffer_t     *constant_buffers;
    u32                    used_constant_buffers;

    u32                    total_render_instances;
    u32                    total_materials;
    u32                    total_shaders;
    u32                    total_buffers;

    vec2_t                 window_size;
    u32                    current_window_size_generation;
    u32                    last_window_size_generation;

    render_target_t        render_targets[MAX_RENDER_TARGETS];
    u32                    render_target_count;
};

void             s_renderer_state_init(renderer_state_t *renderer_state, void *render_context);
void             s_renderer_handle_window_resize(renderer_state_t *renderer_state, vec2_t window_size);
render_target_t* s_renderer_render_target_create(renderer_state_t *renderer_state, render_target_create_info_t *create_info);
void             s_renderer_render_target_destroy(renderer_state_t *renderer_state, render_target_t *render_target);
void             s_renderer_resize_render_targets(renderer_state_t *renderer_state, vec2_t window_size);

image_t                s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
void                   s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void                   s_renderer_image_update_data(void *backend_context, image_t *image);
render_command_list_t* s_renderer_get_command_list(renderer_state_t *renderer_state);

void r_cmd_clear_render_target(render_command_list_t *command_list, render_target_t *render_target);
void r_cmd_bind_render_target(render_command_list_t *command_list, render_target_t *render_target);
void r_cmd_begin_render_group(render_command_list_t *command_list);
void r_cmd_end_render_group(render_command_list_t *command_list);
void r_cmd_blit_render_target(render_command_list_t *command_list, render_target_t *source, render_target_t *destination);
void r_cmd_present(render_command_list_t *command_list);

void
r_cmd_draw_rectangle(render_command_list_t *command_list, 
                     vec2_t                 position, 
                     vec2_t                 size, 
                     vec4_t                 render_color, 
                     float32                rotation);
void
r_cmd_draw_bitmap(render_command_list_t *command_list, 
                  vec2_t                 position, 
                  vec2_t                 size, 
                  vec4_t                 render_color, 
                  float32                rotation,
                  asset_handle_t         bitmap_handle);

#endif // S_RENDERER_H

