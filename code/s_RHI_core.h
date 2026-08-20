#if !defined(S_RENDERER_H)
/* ========================================================================
   $File: s_RHI_core.h $
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

constexpr u32 RHI_MAX_RENDER_TARGET_ATTACHMENTS = 10;

constexpr u32 RHI_MAX_RENDER_COMMANDS     = 10000;
constexpr u32 RHI_MAX_COMMAND_LISTS       = 1000;
constexpr u32 RHI_MAX_CONSTANT_BUFFERS    = 1000;
constexpr u32 RHI_MAX_RENDER_TARGETS      = 100;
constexpr u32 RHI_MAX_SHADER_IMAGE_PARAMS = 16;

// TODO(Sleepster): Remove this...
struct camera_matrices_t
{
    mat4_t view_matrix;
    mat4_t projection_matrix;
};

////////////////////
// GPU BUFFERS 
////////////////////

enum RHI_render_buffer_type_t
{
    RHI_RENDER_BUFFER_TYPE_INVALID       = BIT(0),
    RHI_RENDER_BUFFER_TYPE_VERTEX_BUFFER = BIT(1),
    RHI_RENDER_BUFFER_TYPE_INDEX_BUFFER  = BIT(2),
};

enum RHI_render_buffer_advance_rate_t 
{
    RHI_RENDER_BUFFER_ADVANCE_RATE_PER_ELEMENT  = BIT(0),
    RHI_RENDER_BUFFER_ADVANCE_RATE_PER_INSTANCE = BIT(1),
};

enum RHI_render_buffer_memory_type_t
{
    RHI_RENDER_BUFFER_ALLOCATION_TYPE_MAPPED   = BIT(0),
    RHI_RENDER_BUFFER_ALLOCATION_TYPE_GPU_ONLY = BIT(1),
};

struct RHI_render_buffer_desc_t
{
    RHI_render_buffer_type_t         type;
    RHI_render_buffer_memory_type_t  allocation_type;
    RHI_render_buffer_advance_rate_t advance_rate;

    u64                              buffer_capacity;
    u32                              element_size;

    void                            *initial_data;
};

// TODO(Sleepster): 
// Clean this structure up. we have WAYYYYYYYY to many ways of tracking the same thing,
// the same thing being that of the amount used within the buffer.
struct RHI_render_buffer_t
{
    RHI_render_buffer_type_t        type;
    RHI_render_buffer_memory_type_t allocation_type;
    u64                             constant_buffer_hash_ID;
    u64                             buffer_ID;

    u32                             buffer_capacity;
    u32                             buffer_element_size;
    u32                             buffer_elements_used;
    u32                             working_offset;

    byte                           *mapped_data;
    backend_buffer_t                buffer_info;
};

struct RHI_vertex_buffer_t
{
    RHI_render_buffer_t buffer_data;

    byte               *vertex_data;
    u32                 max_vertices;
    u32                 vertex_count;
};

struct RHI_index_buffer_t
{
    RHI_render_buffer_t buffer_data;
    
    byte               *index_data;
    u32                 max_indices;
    u32                 index_count;
    u32                 index_offset;
};

////////////////////
// RENDER COMMAND STUFF
////////////////////
struct RHI_renderpass_t;

// NOTE(Sleepster): The memory for each of these is transient, don't rely 
// on them sticking around between frames...
struct RHI_uniform_constant_buffer_t
{
    void *mapped_data;
    u32   size;
    u32   offset;
    u64   uniform_hash_index;
};

#if 0
// TODO(Sleepster): Handle layer data here.
// Layers should be seperate from Z in case we do a 2.5D game
struct RHI_render_instance_t
{
    mat4_t transform;
    vec4_t color;
    vec2_t uv_min;
    vec2_t uv_max;
    u32    bitmap_index;
    u32    material_index;
    u32    matrix_index;
};

struct RHI_geometry_data_t
{
    vec2_t  position;
    vec2_t  size;
    vec4_t  render_color;
    float32 rotation;
};
#endif

enum RHI_command_type_t
{
    RHI_RENDER_COMMAND_TYPE_INVALID,
    RHI_RENDER_COMMAND_TYPE_CLEAR_RENDER_TARGET,
    RHI_RENDER_COMMAND_TYPE_BEGIN_RENDERPASS,
    RHI_RENDER_COMMAND_TYPE_END_RENDERPASS,
    RHI_RENDER_COMMAND_TYPE_UPDATE_UNIFORM_CONSTANT_BUFFER,
    RHI_RENDER_COMMAND_TYPE_UPDATE_PUSH_CONSTANTS,
    RHI_RENDER_COMMAND_TYPE_UPDATE_BUFFER_CONTENTS,
    RHI_RENDER_COMMAND_TYPE_BIND_MATERIAL,
    RHI_RENDER_COMMAND_TYPE_BIND_TEXTURE,
    RHI_RENDER_COMMAND_TYPE_BIND_SHADER,
    RHI_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER,
    RHI_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER,
    RHI_RENDER_COMMAND_TYPE_SET_VIEWPORT,
    RHI_RENDER_COMMAND_TYPE_SET_SCISSOR,
    RHI_RENDER_COMMAND_TYPE_SET_RENDER_STATE,
    RHI_RENDER_COMMAND_TYPE_RESET_RENDER_STATE,
    RHI_RENDER_COMMAND_TYPE_DISPATCH_COMPUTE,
    RHI_RENDER_COMMAND_TYPE_DRAW,
    RHI_RENDER_COMMAND_TYPE_DRAW_INDEXED,
    RHI_RENDER_COMMAND_TYPE_BLIT_IMAGE,
    RHI_RENDER_COMMAND_TYPE_BLIT_RENDERPASS,
    RHI_RENDER_COMMAND_TYPE_PRESENT_FRAME,
    RHI_RENDER_COMMAND_TYPE_COUNT,
};

struct RHI_command_header_t
{
    RHI_command_type_t command_type;
};

struct RHI_command_begin_renderpass_t
{
    u32 ID;
};

struct RHI_command_end_renderpass_t
{
    u32 ID;
};

struct RHI_command_bind_vertex_buffer_t
{
    RHI_render_buffer_t *vertex_buffer;
};

struct RHI_command_bind_index_buffer_t
{
    RHI_index_buffer_t *index_buffer;
};

struct RHI_command_update_texture_t
{
    asset_handle_t                bitmap;
    RHI_uniform_constant_buffer_t buffer;
};

struct RHI_command_bind_material_t
{
    u32            materialID;
    asset_handle_t material;
};

struct RHI_command_bind_shader_t
{
    u32            shaderID;
    asset_handle_t shader;
};

struct RHI_command_set_viewport_t
{
    vec2_t offset; 
    vec2_t size;
};

struct RHI_command_set_scissor_t
{
    vec2_t offset;
    vec2_t size;
};

struct RHI_command_update_push_constant_t
{
    void *data;
    u32   size;
    u32   offset;
};

struct RHI_command_update_uniform_constant_buffer_t
{
    void                          *backend_uniform_buffer_ptr;
    RHI_uniform_constant_buffer_t *buffer;
    u64                            uniform_hash_index;
    u32                            constant_data_size;
    u32                            constant_buffer_offset;
};

struct RHI_command_update_render_buffer_contents_t
{
    RHI_render_buffer_t *buffer;
    u32                  offset;
    u32                  data_size;
};

struct RHI_command_bind_texture_t 
{
    RHI_image_t *texture;
};

struct RHI_command_dispatch_compute_t
{
    u32 invoke_x;
    u32 invoke_y;
    u32 invoke_z;
};

struct RHI_command_set_pipeline_state_t
{
    RHI_pipeline_state_t pipeline_state;
};

struct RHI_command_draw_t 
{
// NOTE(Sleepster): 
// Since the command lists are deferred, we must store additional information related to the draw call. Such as
// what the current offset into the vertex buffer is (as all vertex buffers will be merged into one MASSIVE buffer
// so the GPU can source data from it efficiently) and what descriptors are used at the time of the draw.

    // TODO(Sleepster): Do we need this??? 
    RHI_image_t *bound_images;
    u32          bound_image_count;

    u32          backend_vertex_buffer_offset;

    u32          vertices_to_draw;
    u32          vertex_offset;

    u32          indices_to_draw;
    u32          index_offset;

    u32          instance_count;
    u32          first_instance;
};

struct RHI_command_blit_image_t
{
    RHI_image_t *source_image;
    RHI_image_t *dest_image;
    vec2_t       source_offset;
    vec2_t       source_size;
    vec2_t       dest_offset;
    vec2_t       dest_size;
};

struct RHI_command_blit_renderpass_t
{
    RHI_renderpass_t *source;
    RHI_renderpass_t *destination;
};

struct RHI_command_present_frame_t
{
    RHI_image_t *presentation_source;
};

struct RHI_command_t
{
    RHI_command_header_t header;
    void                *data;
};

enum RHI_command_list_type_t 
{
    RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS,
    RHI_RENDER_COMMAND_LIST_TYPE_COMPUTE,
};

struct RHI_command_list_t
{
    bool8                            is_initialized;
    RHI_context_t                   *RHI_context;
    // NOTE(Sleepster): Everything within this is reset once all commands are executed 
    memory_arena_t                   transient_arena;
    memory_arena_t                   command_arena;

    RHI_command_t                   *commands;
    u32                              command_count;

    u32                              draw_instance_command_count;
    u32                              bind_shader_command_count;
    u32                              bind_render_target_command_count;
    u32                              bind_material_command_count;

    RHI_pipeline_state_t             active_render_state;

    dynarray_t<RHI_render_buffer_t*> active_vertex_buffers;
    u32                              vertex_buffer_count;

    RHI_index_buffer_t              *active_index_buffer;
    RHI_command_t                   *active_scissor_command;
    RHI_command_t                   *active_viewport_command;
    asset_handle_t                  *active_shader_program;

    //u32                        vertex_offset;
    //u32                        index_offset;
    //u32                        instance_offset;

    RHI_renderpass_t                *active_renderpass;
    bool32                           presenting;

    RHI_image_t                     *image_shader_params[RHI_MAX_SHADER_IMAGE_PARAMS];
    u32                              image_count;

    u32                              image_ids_to_bind[RHI_MAX_SHADER_IMAGE_PARAMS];
    u32                              bound_image_count;

    RHI_command_list_type_t          command_list_type;
    backend_command_buffer_t         backend_command_buffer;
};

////////////////////
// RENDER TARGETS 
////////////////////

union RHI_clear_value_t
{
    union {
        vec4_t  float_color;
        ivec4_t int_color;
        u32     uint_color[4];
        struct {
            float32 depth;
            u32     stencil;
        };
    };
};

enum RHI_renderpass_attachment_access_t
{
    RHI_RENDERPASS_ATACHMENT_ACCESS_INVALID     = BIT(0),
    RHI_RENDERPASS_ATTACHMENT_ACCESS_READ       = BIT(1),
    RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE      = BIT(2),
    RHI_RENDERPASS_ATTACHMENT_ACCESS_READ_WRITE = RHI_RENDERPASS_ATTACHMENT_ACCESS_READ|RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
};

enum RHI_renderpass_attachment_load_operation_t
{
    RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_INVALID = BIT(0),
    RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR   = BIT(1),
    RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_LOAD    = BIT(2)
};

enum RHI_renderpass_attachment_store_operation_t
{
    RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_INVALID   = BIT(0),
    RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE     = BIT(1),
    RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_DONT_CARE = BIT(2)
};

struct RHI_renderpass_attachment_t
{
    RHI_renderpass_attachment_access_t          access;
    RHI_renderpass_attachment_load_operation_t  load_operation;
    RHI_renderpass_attachment_store_operation_t store_operation;

    RHI_image_t                                *image;
    RHI_clear_value_t                           clear_value;
};

// NOTE(Sleepster): 
// We don't have the option to append a stencil attachment because originally in Vulkan
// depth attachments and stencil attachments were merged into a single depthStencilAttachment
struct RHI_renderpass_desc_t
{
    RHI_renderpass_attachment_t color_attachments[RHI_MAX_RENDER_TARGET_ATTACHMENTS];
    RHI_renderpass_attachment_t depth_stencil_attachment;

    u32                         render_width;
    u32                         render_height;
    u32                         color_attachment_count;
    bool8                       resize_with_window;
};

struct RHI_renderpass_key_t
{
    bitmap_format_t attachment_formats[RHI_MAX_RENDER_TARGET_ATTACHMENTS];
};

struct RHI_renderpass_t 
{
    u32                          ID;

    RHI_renderpass_key_t         renderpass_key;
    RHI_renderpass_desc_t        create_info;

    backend_renderpass_handle_t  renderpass_handle;
    backend_framebuffer_handle_t framebuffer_handle;

    RHI_renderpass_attachment_t  depth_stencil_attachment;
    RHI_renderpass_attachment_t  color_attachments[RHI_MAX_RENDER_TARGET_ATTACHMENTS];
    u32                          color_attachment_count;
    u32                          total_attachment_count;

    u32                          render_width;
    u32                          render_height;

    RHI_clear_value_t            attachment_clear_values[RHI_MAX_RENDER_TARGET_ATTACHMENTS];

    bool8                        has_depth_stencil_attachment;
    bool8                        resize_with_window;
};

////////////////////
// RHI_context_t
////////////////////

struct RHI_context_t
{
    SDL_Window                                *window;
    backend_render_context_t                  *backend_render_context;

    memory_arena_t                             RHI_arena;
    memory_arena_t                             transient_arena;

    RHI_command_list_t                        *command_lists;
    u32                                        command_list_count;

    HashTable_t(RHI_uniform_constant_buffer_t) constant_buffer_hash;
    u32                                        used_constant_buffers;

    u32                                        total_render_instances;
    u32                                        total_materials;
    u32                                        total_shaders;
    u32                                        total_buffers;

    vec2_t                                     window_size;
    u32                                        current_window_size_generation;
    u32                                        last_window_size_generation;

    RHI_renderpass_t                           renderpasses[100];
    u32                                        renderpass_count;

    RHI_command_present_frame_t               *present_command;

    void                     backend_initialize(SDL_Window *window);
    void                     backend_handle_window_resize(vec2_t window_size);
    void                     backend_render_frame(void);

    RHI_render_buffer_t      backend_buffer_create(RHI_render_buffer_desc_t *buffer_desc);
    void                     backend_buffer_copy_data(RHI_render_buffer_t *buffer, void *data, u32 size, u32 offset);
    void                     backend_buffer_append_data(RHI_render_buffer_t *buffer, void *data, u32 data_size);
    void*                    backend_constant_buffer_append_data(void *data, u32 data_size, u32 *buffer_offset_out);
    void                     backend_buffer_reset(RHI_render_buffer_t *buffer);

    u32                      backend_renderpass_initialize(RHI_renderpass_desc_t *desc, RHI_renderpass_t *renderpass);

    void                     backend_image_create(RHI_image_create_info_t *create_info, RHI_image_t *image);
    void                     backend_image_destroy(RHI_image_t *image);
    void                     backend_image_update_contents(RHI_image_t *image);
    void                     backend_acquire_image_sampler(RHI_image_t *image);

    void                     backend_shader_create(RHI_shader_t *shader, string_t shader_source);
    backend_command_buffer_t backend_get_command_buffer(RHI_command_list_t *command_list);
};

// TODO(Sleepster):  
//
// - Texture set filter mode functions (to allow us to set the sampler settings)

struct RHI_image_create_info_t;

void             RHI_context_init(RHI_context_t *RHI_context, backend_render_context_t *render_context);
void             RHI_handle_window_resize(RHI_context_t *RHI_context, vec2_t window_size);
void             RHI_resize_render_targets(RHI_context_t *RHI_context, vec2_t window_size);
u32              RHI_build_renderpass(RHI_context_t *RHI_context, RHI_renderpass_desc_t *renderpass_desc);
true_inline void RHI_resize_renderpass(RHI_context_t *RHI_context, RHI_renderpass_t *renderpass);

RHI_uniform_constant_buffer_t* RHI_get_constant_buffer(RHI_context_t *RHI_context, string_t uniform_name);

            RHI_render_buffer_t RHI_render_buffer_create(RHI_context_t *RHI_context, RHI_render_buffer_desc_t *buffer_desc);
true_inline RHI_vertex_buffer_t RHI_vertex_buffer_create(RHI_context_t *RHI_context, RHI_render_buffer_memory_type_t memory_type, RHI_render_buffer_advance_rate_t rate, byte* vertex_buffer_data, u32 vertex_size, u32 max_vertices);
true_inline RHI_index_buffer_t  RHI_index_buffer_create(RHI_context_t *RHI_context, RHI_render_buffer_memory_type_t memory_type, u32 element_size, void *data, u32 size);
true_inline void                RHI_render_buffer_copy_data(RHI_context_t *RHI_context, RHI_render_buffer_t *buffer, void *data, u32 size, u32 offset);
true_inline void                RHI_buffer_reset(RHI_context_t *RHI_context,  RHI_render_buffer_t *buffer);
true_inline void                RHI_buffer_reset(RHI_context_t *RHI_context,  RHI_vertex_buffer_t *buffer);
true_inline void                RHI_buffer_reset(RHI_context_t *render_state, RHI_index_buffer_t *buffer);

RHI_image_t            RHI_image_create(RHI_context_t *render_state, RHI_image_create_info_t *image_create_info);
void                   RHI_image_destroy(RHI_context_t *RHI_context, RHI_image_t *image);
void                   RHI_image_update_data(void *backend_context, RHI_image_t *image);
RHI_command_list_t    *RHI_get_command_list(RHI_context_t *RHI_context, RHI_command_list_type_t type);
s32                    RHI_find_texture_index(RHI_command_list_t *command_list, u64 ID);
void                   RHI_reset_command_list(RHI_command_list_t *command_list);

// TODO(Sleepster): Do we want these here???
s32                    RHI_is_texture_bound(RHI_command_list_t *command_list, texture2D_t *texture);
void                   RHI_set_texture_filter_mode(RHI_context_t *render_state, texture2D_t *texture, u32 filter_mode);

// NOTE(Sleepster): RHI commands 
void RHI_cmd_renderpass_begin(RHI_command_list_t *command_list, u32 renderpassID);
void RHI_cmd_renderpass_end(RHI_command_list_t *command_list);
void RHI_cmd_begin_render_group(RHI_command_list_t *command_list);
void RHI_cmd_end_render_group(RHI_command_list_t *command_list);
void RHI_cmd_bind_vertex_buffer(RHI_command_list_t *command_list, RHI_render_buffer_t *buffer);
void RHI_cmd_bind_index_buffer(RHI_command_list_t *command_list, RHI_render_buffer_t *buffer);
void RHI_cmd_set_scissor(RHI_command_list_t *command_list, vec2_t offset, vec2_t size);
void RHI_cmd_set_viewport(RHI_command_list_t *command_list, vec2_t offset, vec2_t size);
void RHI_cmd_update_push_constants(RHI_command_list_t *command_list, u32 offset, u32 size, void *data);
void RHI_cmd_update_buffer_contents(RHI_command_list_t *command_list, RHI_render_buffer_t *buffer, void *data, u32 data_size);
void RHI_cmd_use_shader_program(RHI_command_list_t *command_list, asset_handle_t program);
void RHI_cmd_update_constant_buffer(RHI_command_list_t *command_list, RHI_uniform_constant_buffer_t *buffer, void *data, u32 data_size);
void RHI_cmd_bind_texture_image(RHI_command_list_t *command_list, texture2D_t *texture);
void RHI_cmd_bind_texture_from_handle(RHI_command_list_t *command_list, asset_handle_t *asset_handle);
void RHI_cmd_reset_render_state(RHI_command_list_t *command_list, RHI_pipeline_state_t *render_pipeline_state);
void RHI_cmd_set_render_state(RHI_command_list_t *command_list, RHI_pipeline_state_t *render_pipeline_state);
void RHI_cmd_draw(RHI_command_list_t *command_list, u32 vertex_count, u32 vertex_offset, u32 instance_count, u32 first_instance);
void RHI_cmd_draw_indexed(RHI_command_list_t *command_list, u32 index_count, u32 index_offset, u32 instance_count, u32 first_instance);
void RHI_cmd_dispatch_compute(RHI_command_list_t *command_list, u32 invoke_x, u32 invoke_y, u32 invoke_z);
void RHI_cmd_blit_image(RHI_command_list_t *command_list, RHI_image_t *source_image, RHI_image_t *dest_image, vec2_t source_offset, vec2_t source_blit_size, vec2_t dest_offset, vec2_t dest_blit_size);
void RHI_cmd_blit_renderpass(RHI_command_list_t *command_list, u32 source_ID, u32 destination_ID);
void RHI_cmd_present(RHI_command_list_t *command_list, RHI_image_t *presentation_source);

// NOTE(Sleepster): Aliases for operator overloading...
true_inline void RHI_cmd_update_buffer_contents(RHI_command_list_t *command_list, RHI_vertex_buffer_t *buffer);
true_inline void RHI_cmd_bind_vertex_buffer(RHI_command_list_t *command_list, RHI_vertex_buffer_t *buffer);

void RHI_execute_backend_commands(RHI_context_t *RHI_context);

#endif // S_RENDERER_H

