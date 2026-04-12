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

constexpr u32 MAX_RENDER_COMMANDS     = 10000;
constexpr u32 MAX_COMMAND_LISTS       = 1000;
constexpr u32 MAX_CONSTANT_BUFFERS    = 1000;
constexpr u32 MAX_RENDER_TARGETS      = 100;
constexpr u32 MAX_SHADER_IMAGE_PARAMS = 16;

////////////////////
// GPU BUFFERS 
////////////////////

enum render_buffer_type_t
{
    RenderBufferType_Invalid      = BIT(0),
    RenderBufferType_VertexBuffer = BIT(1),
    RenderBufferType_IndexBuffer  = BIT(2),
};

enum render_buffer_advance_rate_t 
{
    RenderBufferAdvanceRate_PerElement  = BIT(0),
    RenderBufferAdvanceRate_PerInstance = BIT(1),
};

enum render_buffer_memory_type_t
{
    RenderBufferAllocationTypeMapped  = BIT(0),
    RenderBufferAllocationTypeGPUOnly = BIT(1)
};

struct render_buffer_desc_t
{
    render_buffer_type_t         type;
    render_buffer_memory_type_t  allocation_type;
    render_buffer_advance_rate_t advance_rate;

    u64                          buffer_capacity;
    u32                          element_size;

    void                        *initial_data;
};

struct render_buffer_t
{
    render_buffer_type_t        type;
    render_buffer_memory_type_t allocation_type;

    u32                         buffer_capacity;
    u32                         buffer_element_size;
    u32                         buffer_elements_used;

    vulkan_buffer_t             buffer;
};

////////////////////
// RENDER COMMAND STUFF
////////////////////
struct renderpass_t;

// NOTE(Sleepster): The memory for each of these is transient, don't rely 
// on them sticking around between frames...
struct uniform_constant_buffer_t
{
    void *mapped_data;
    u32   size;
    u32   offset;
    u64   uniform_hash_index;
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
    RCT_BeginRenderpass,
    RCT_EndRenderpass,
    RCT_DrawRectangle,
    RCT_DrawBitmap,
    RCT_UpdateUniformConstantBuffer,
    RCT_UpdatePushConstants,
    RCT_BindMaterial,
    RCT_BindShader,
    RCT_BindVertexBuffer,
    RCT_BindIndexBuffer,
    RCT_SetViewport,
    RCT_SetScissor,
    RCT_BindTexture,
    RCT_SetRenderState,
    RCT_ResetRenderState,
    RCT_Draw,
    RCT_DrawIndexed,
    RCT_DispatchCompute,
    RCT_BlitImage,
    RCT_PresentFrame,

    RCT_Count
};

struct render_command_header_t
{
    render_command_type_t command_type;
};

struct render_command_begin_renderpass_t
{
    u32 ID;
};

struct render_command_draw_bitmap_t
{
    u32                     instanceID;

    geometry_data_t         quad_data;
    asset_handle_t          bitmap;
};

struct render_command_draw_rectangle_t
{
    u32                     instanceID;

    geometry_data_t         quad_data;
};

struct render_command_bind_vertex_buffer_t
{
    render_buffer_t        *buffer;
};

struct render_command_bind_index_buffer_t
{
    render_buffer_t        *buffer;
};

struct render_command_update_texture_t
{
    asset_handle_t            bitmap;
    uniform_constant_buffer_t buffer;
};

struct render_command_bind_material_t
{
    u32            materialID;
    asset_handle_t material;
};

struct render_command_bind_shader_t
{
    u32            shaderID;
    asset_handle_t shader;
};

struct render_command_set_viewport_t
{
    vec2_t offset; 
    vec2_t size;
};

struct render_command_set_scissor_t
{
    vec2_t offset;
    vec2_t size;
};

struct render_command_update_push_constant_t
{
    void *data;
    u32   size;
    u32   offset;
};

struct render_command_update_uniform_constant_buffer_t
{
    void *backend_uniform_buffer_ptr;
    u64   uniform_hash_index;
    u32   constant_data_size;
};

struct render_command_bind_texture_t 
{
    image_t *texture;
};

struct render_command_dispatch_compute_t
{
    u32 invoke_x;
    u32 invoke_y;
    u32 invoke_z;
};

struct render_command_set_pipeline_state_t
{
    render_pipeline_state_t pipeline_state;
};

struct render_command_draw_t 
{
    u32 vertices_to_draw;
    u32 vertex_offset;

    u32 indices_to_draw;
    u32 index_offset;

    u32 instance_count;
    u32 first_instance;
};

struct render_command_blit_image_t
{
    image_t *source_image;
    image_t *dest_image;
    vec2_t   source_offset;
    vec2_t   source_size;
    vec2_t   dest_offset;
    vec2_t   dest_size;
};

struct render_command_present_frame_t
{
    image_t *presentation_source;
};

struct render_command_t
{
    render_command_header_t header;
    void                   *data;
};

// NOTE(Sleepster): For some stupid fucking reason this has to be heap allocated instead of 
// just stored normally like a normal object. My compiler is an autistic chimp
struct render_command_list_t
{
    bool8                   is_initialized;
    renderer_state_t       *renderer_state;
    // NOTE(Sleepster): Everything within this is reset once all commands are executed 
    memory_arena_t          transient_arena;
    memory_arena_t          command_arena;

    render_command_t       *commands;
    u32                     command_count;

    u32                     draw_instance_command_count;
    u32                     bind_shader_command_count;
    u32                     bind_render_target_command_count;
    u32                     bind_material_command_count;

    render_pipeline_state_t active_render_state;

    render_buffer_t        *active_vertex_buffer;
    render_buffer_t        *active_index_buffer;

    render_command_t       *active_scissor_command;
    render_command_t       *active_viewport_command;

    asset_handle_t         *active_shader_program;

    renderpass_t           *active_renderpass;
    bool32                  presenting;

    image_t                *image_shader_params[MAX_SHADER_IMAGE_PARAMS];
    u32                     image_count;
};

////////////////////
// RENDER TARGETS 
////////////////////

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

enum renderpass_attachment_access_t
{
    RenderpassAtachmentAccessInvalid    = BIT(0),
    RenderpassAttachmentAccessRead      = BIT(1),
    RenderpassAttachmentAccessWrite     = BIT(2),
    RenderpassAttachmentAccessReadWrite = RenderpassAttachmentAccessRead|RenderpassAttachmentAccessWrite,
};

enum renderpass_attachment_load_operation_t
{
    RenderpassAttachmentLoadOperationInvalid = BIT(0),
    RenderpassAttachmentLoadOperationClear   = BIT(1),
    RenderpassAttachmentLoadOperationLoad    = BIT(2)
};

enum renderpass_attachment_store_operation_t
{
    RenderpassAttachmentStoreOperationInvalid  = BIT(0),
    RenderpassAttachmentStoreOperationStore    = BIT(1),
    RenderpassAttachmentStoreOperationDontCare = BIT(2)
};

struct renderpass_attachment_t
{
    renderpass_attachment_access_t          access;
    renderpass_attachment_load_operation_t  load_operation;
    renderpass_attachment_store_operation_t store_operation;

    image_t                                *image;
    clear_value_t                           clear_value;
};

// NOTE(Sleepster): 
// We don't have the option to append a stencil attachment because originally in Vulkan
// depth attachments and stencil attachments were merged into a single depthStencilAttachment
struct renderpass_desc_t
{
    renderpass_attachment_t color_attachments[MAX_RENDER_TARGET_ATTACHMENTS];
    renderpass_attachment_t depth_stencil_attachment;

    u32                     render_width;
    u32                     render_height;
    bool8                   resize_with_window;

    u32                     color_attachment_count;
};

struct renderpass_t 
{
    u32               ID;
    renderpass_desc_t create_info;
    VkRenderPass      renderpass_handle;
    VkFramebuffer     framebuffer_handle;

    renderpass_attachment_t depth_stencil_attachment;
    renderpass_attachment_t color_attachments[MAX_RENDER_TARGET_ATTACHMENTS];
    u32                     color_attachment_count;
    u32                     total_attachment_count;

    u32           render_width;
    u32           render_height;

    clear_value_t attachment_clear_values[MAX_RENDER_TARGET_ATTACHMENTS];

    bool8         has_depth_stencil_attachment;
    bool8         resize_with_window;
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
    SDL_Window                            *window;

    memory_arena_t                         renderer_arena;
    memory_arena_t                         transient_arena;

    void                                  *render_context;

    render_command_list_t                 *command_lists;
    u32                                    command_list_count;

    HashTable_t(uniform_constant_buffer_t) constant_buffer_hash;
    u32                                    used_constant_buffers;

    u32                                    total_render_instances;
    u32                                    total_materials;
    u32                                    total_shaders;
    u32                                    total_buffers;

    vec2_t                                 window_size;
    u32                                    current_window_size_generation;
    u32                                    last_window_size_generation;

    renderpass_t                           renderpasses[100];
    u32                                    renderpass_count;
};

struct image_create_info_t;

void             s_renderer_state_init(renderer_state_t *renderer_state, void *render_context);
void             s_renderer_handle_window_resize(renderer_state_t *renderer_state, vec2_t window_size);
void             s_renderer_resize_render_targets(renderer_state_t *renderer_state, vec2_t window_size);
u32              s_renderer_build_renderpass(renderer_state_t *renderer_state, renderpass_desc_t *renderpass_desc);

uniform_constant_buffer_t* s_renderer_get_constant_buffer(renderer_state_t *renderer_state, string_t uniform_name);

            render_buffer_t s_renderer_render_buffer_create(renderer_state_t *renderer_state, render_buffer_desc_t *buffer_desc);
true_inline render_buffer_t s_renderer_vertex_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, u32 element_size, render_buffer_advance_rate_t rate, void *data, u32 size);
true_inline render_buffer_t s_renderer_index_buffer_create(renderer_state_t *renderer_state, render_buffer_memory_type_t memory_type, u32 element_size, void *data, u32 size);
true_inline void            s_renderer_render_buffer_copy_data(renderer_state_t *renderer_state, render_buffer_t *buffer, void *data, u32 size, u32 offset);

image_t                s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
void                   s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void                   s_renderer_image_update_data(void *backend_context, image_t *image);
render_command_list_t* s_renderer_get_command_list(renderer_state_t *renderer_state);
bool8                  s_renderer_is_texture_bound(render_command_list_t *command_list, texture2D_t *texture);
void                   s_renderer_reset_command_list(render_command_list_t *command_list);

void r_cmd_renderpass_begin(render_command_list_t *command_list, u32 renderpassID);
void r_cmd_renderpass_end(render_command_list_t *command_list);
void r_cmd_begin_render_group(render_command_list_t *command_list);
void r_cmd_end_render_group(render_command_list_t *command_list);
void r_cmd_bind_vertex_buffer(render_command_list_t *command_list, render_buffer_t *buffer);
void r_cmd_bind_index_buffer(render_command_list_t *command_list, render_buffer_t *buffer);
void r_cmd_set_scissor(render_command_list_t *command_list, vec2_t offset, vec2_t size);
void r_cmd_set_viewport(render_command_list_t *command_list, vec2_t offset, vec2_t size);
void r_cmd_update_push_constants(render_command_list_t *command_list, u32 offset, u32 size, void *data);
void r_cmd_use_shader_program(render_command_list_t *command_list, asset_handle_t program);
void r_cmd_update_buffer_contents(render_command_list_t *command_list, uniform_constant_buffer_t *buffer, void *data, u32 data_size);
void r_cmd_bind_texture_image(render_command_list_t *command_list, texture2D_t *texture);
void r_cmd_bind_texture_from_handle(render_command_list_t *command_list, asset_handle_t *asset_handle);
void r_cmd_reset_render_state(render_command_list_t *command_list, render_pipeline_state_t *render_pipeline_state);
void r_cmd_set_render_state(render_command_list_t *command_list, render_pipeline_state_t *render_pipeline_state);
void r_cmd_draw(render_command_list_t *command_list, u32 vertex_count, u32 vertex_offset, u32 instance_count, u32 first_instance);
void r_cmd_draw_indexed(render_command_list_t *command_list, u32 index_count, u32 index_offset, u32 instance_count, u32 first_instance);
void r_cmd_dispatch_compute(render_command_list_t *command_list, u32 invoke_x, u32 invoke_y, u32 invoke_z);
void r_cmd_blit_image(render_command_list_t *command_list, image_t *source_image, image_t *dest_image, vec2_t source_offset, vec2_t source_blit_size, vec2_t dest_offset, vec2_t dest_blit_size);
void r_cmd_present(render_command_list_t *command_list, image_t *presentation_source);

void s_renderer_execute_backend_commands(renderer_state_t *renderer_state);

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

