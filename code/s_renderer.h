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

////////////////////
// RENDER COMMAND STUFF
////////////////////
struct render_frame_graph_t;

struct render_target_t;

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

enum render_command_type_t
{
    RCT_Invalid,

    RCT_ClearRenderTarget,
    RCT_BeginRenderGroup,
    RCT_EndRenderGroup,
    RCT_BeginRenderpass,
    RCT_EndRenderpass,
    RCT_DrawRectangle,
    RCT_DrawBitmap,
    RCT_UpdateTexture,
    RCT_UpdateBuffer,
    RCT_BindMaterial,
    RCT_BindShader,
    RCT_PresentFrame,

    RCT_Count
};

struct render_command_header_t
{
    render_command_type_t command_type;
};

struct render_command_begin_render_group_t
{
    render_command_header_t header;
};

struct render_command_end_render_group_t
{
    render_command_header_t header;
};

struct render_command_begin_renderpass_t
{
    render_command_header_t header;
    render_frame_graph_t   *frame_graph;
    u32                     ID;
};

struct render_command_end_renderpass_t
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
        render_command_begin_render_group_t     begin_render_group;
        render_command_end_render_group_t       end_render_group;
        render_command_begin_renderpass_t       begin_renderpass;
        render_command_end_renderpass_t         end_renderpass;
        render_command_draw_rectangle_t         draw_rectangle;
        render_command_draw_bitmap_t            draw_texture;
        render_command_update_constant_buffer_t update_constant_buffer;
        render_command_update_texture_t         update_texture_contents;
        render_command_bind_material_t          bind_material;
        render_command_bind_shader_t            bind_shader;
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

////////////////////
// RENDER TARGETS 
////////////////////

enum render_target_attachment_type_t 
{
    RTAT_Undefined              = 0,
    RTAT_ColorAttachment        = 1,
    RTAT_DepthStencilAttachment = 2,
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

enum render_target_attachment_permissions_t
{
    RTAT_InvalidAttachment   = BIT(0),
    RTAT_ReadAttachment      = BIT(1),
    RTAT_WriteAttachment     = BIT(2),
    RTAT_ReadWriteAttachment = RTAT_ReadAttachment|RTAT_WriteAttachment,
};

struct render_target_attachment_info_t
{
    u32           ID;
    u32           attachment_type;
    image_t      *attachment;

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

////////////////////
// RENDER TARGETS 
////////////////////

const u32 FRAME_GRAPH_MAX_RENDERPASSES = 10;

enum renderpass_attachment_type_t
{
    RenderpassAttachmentRead      = BIT(0),
    RenderpassAttachmentWrite     = BIT(1),
    RenderpassAttachmentReadWrite = RenderpassAttachmentRead|RenderpassAttachmentWrite,
};

struct renderpass_attachment_t
{
    renderpass_attachment_type_t type;
    image_t                     *image;
    clear_value_t                clear_value;
};

struct renderpass_desc_t
{
    renderpass_attachment_t attachments[MAX_RENDER_TARGET_ATTACHMENTS];
    u32                     attachment_count;
};

struct render_frame_graph_desc_t 
{
    renderpass_desc_t renderpass_descs[FRAME_GRAPH_MAX_RENDERPASSES];
    u32               renderpass_count;
};

struct frame_graph_renderpass_t 
{
    u32           ID;
    VkRenderPass  renderpass_handle;
    VkFramebuffer framebuffer_handle;

    u32           width;
    u32           height;
    u32           attachment_count;

    clear_value_t attachment_clear_values[MAX_RENDER_TARGET_ATTACHMENTS];
};

struct render_frame_graph_t 
{
    frame_graph_renderpass_t renderpasses[FRAME_GRAPH_MAX_RENDERPASSES];
    u32                      renderpass_count;
};

////////////////////
// Renderer State 
////////////////////

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

#if 0 
    render_target_t        render_targets[MAX_RENDER_TARGETS];
    u32                    render_target_count;
    DynArray_t(render_target_attachment_info_t) attachments;
#endif
};

struct image_create_info_t;

void             s_renderer_state_init(renderer_state_t *renderer_state, void *render_context);
void             s_renderer_handle_window_resize(renderer_state_t *renderer_state, vec2_t window_size);
render_target_t* s_renderer_render_target_create(renderer_state_t *renderer_state, render_target_create_info_t *create_info);
void             s_renderer_render_target_destroy(renderer_state_t *renderer_state, render_target_t *render_target);
void             s_renderer_resize_render_targets(renderer_state_t *renderer_state, vec2_t window_size);

image_t                s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
void                   s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void                   s_renderer_image_update_data(void *backend_context, image_t *image);
render_command_list_t* s_renderer_get_command_list(renderer_state_t *renderer_state);
void                   s_renderer_reset_command_list(render_command_list_t *command_list);


void s_renderer_frame_graph_desc_init(render_frame_graph_desc_t *desc);
void s_renderer_renderpass_desc_init(renderpass_desc_t *desc);
void s_renderer_renderpass_attach_image(renderpass_desc_t *renderpass, image_t *image, renderpass_attachment_type_t type, clear_value_t clear_value);
u32  s_renderer_frame_graph_attach_renderpass(render_frame_graph_desc_t *frame_graph, renderpass_desc_t *renderpass_desc);
render_frame_graph_t s_renderer_frame_graph_construct(renderer_state_t *renderer_state, render_frame_graph_desc_t *frame_graph_desc);

internal_api void construct_frame_graph_renderpass(vulkan_context_t *vulkan_context, frame_graph_renderpass_t *renderpass, renderpass_desc_t *desc, u32 ID);

void r_cmd_renderpass_begin(render_command_list_t *command_list, render_frame_graph_t *frame_graph, u32 renderpassID);
void r_cmd_renderpass_end(render_command_list_t *command_list);
void r_cmd_begin_render_group(render_command_list_t *command_list);
void r_cmd_end_render_group(render_command_list_t *command_list);
void r_cmd_blit_render_target(render_command_list_t *command_list, render_command_blit_info_t *blit_info);
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

