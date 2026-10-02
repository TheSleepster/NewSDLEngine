#if !defined(S_RENDER_GRAPH_H)
/* ========================================================================
   $File: s_render_graph.h $
   $Date: September 30 2026 04:21 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_RENDER_GRAPH_H

#include <c_types.h>
#include <c_base.h>
#include <c_global_context.h>
#include <c_dynarray.h>
#include <c_memory_arena.h>
#include <s_RHI_core.h>
#include <s_RHI_image.h>

CODE_GEN_IGNORE_FILE

/* 
===================================
RENDER GROUPS

Basically there's 3 steps here:

Recording -> Scheduling -> Execution

Where the render group is a set of render commands needed to render the desired output
and the render graph is a scheduler in charge of setting things up so that the RHI has
as few state changes as possible.
===================================
*/
struct render_graph_command_t;
struct render_group_sort_key_t
{
    u32 renderpassID;
    u32 submission_index;
    u64 pipeline_state_hash;
    u64 shader_hash;
    u64 vertex_input_hash;
    u64 descriptor_hash;
};

struct render_group_vertex_stream_t
{
    RHI_vertex_buffer_t *vertex_buffer;

    array_view_t<byte> vertices;
    u32                vertex_count;
    u32                max_vertices;
    u32                vertex_offset;
    u32                vertex_stride;
};

struct render_group_constant_buffer_info_t
{
    RHI_uniform_constant_buffer_t *buffer;

    void *buffer_data;
    u32   data_size;
};

struct render_group_push_buffer_t
{
    array_view_t<byte> base;
    u32                used;
};

using render_group_texture_array_t = fixed_array_t<asset_handle_t, RHI_MAX_SHADER_IMAGE_PARAMS>;
using vertex_stream_array_t        = fixed_array_t<render_group_vertex_stream_t, 10>;
using constant_buffer_array_t      = fixed_array_t<render_group_constant_buffer_info_t, 10>;
struct render_group_t
{
    render_group_push_buffer_t   push_buffer;

    render_group_sort_key_t      sort_key;
    u32                          ID;
    u32                          renderpassID;

    asset_handle_t               shader;
    RHI_pipeline_state_t         pipeline_state;
    RHI_index_buffer_t          *index_buffer;

    // TODO(Sleepster):  Add line width to the state key 
    u32                          line_width;
    u32                          draw_element_count;
    vertex_stream_array_t        vertex_streams;
    u32                          vertex_stream_count;

    render_group_texture_array_t textures;
    u32                          texture_count;

    constant_buffer_array_t      constant_buffers;
    u32                          constant_buffer_count;
    struct {
        vec2_t offset;
        vec2_t extent;
    }viewport;

    struct {
        vec2_t offset;
        vec2_t extent;
    }scissor;
};

enum render_graph_command_type_t
{
    RENDER_GRAPH_COMMAND_TYPE_INVALID,
    RENDER_GRAPH_COMMAND_TYPE_RENDER_GROUP,
    RENDER_GRAPH_COMMAND_TYPE_CLEAR_RENDERPASS,
    RENDER_GRAPH_COMMAND_TYPE_BLIT,
};

// NOTE(Sleepster): Render record 
struct render_command_t
{
    s32 type;
    u64 timestamp;
    union {
        render_group_t *render_group;
        struct {
            u32 A;
            u32 B;
        }blit;
        struct {
            u32 A;
        }clear;
    };
};

struct render_graph_node_usage_key_t
{
    u32                  read_renderpassID;
    u32                  write_renderpassID;
    RHI_pipeline_state_t pipeline_state;
    asset_handle_t       shader;
    u64                  descriptor_hash;
    u64                  command_timestamp;
};

enum render_graph_node_type
{
    RENDER_GRAPH_NODE_TYPE_INVALID,
    RENDER_GRAPH_NODE_TYPE_BEGIN_RENDERPASS,
    RENDER_GRAPH_NODE_TYPE_SET_PIPELINE_STATE,
    RENDER_GRAPH_NODE_TYPE_BIND_SHADER,
    RENDER_GRAPH_NODE_TYPE_SET_DESCRIPTORS,
    RENDER_GRAPH_NODE_TYPE_DRAW,
    RENDER_GRAPH_NODE_TYPE_END_RENDERPASS,
    RENDER_GRAPH_NODE_TYPE_BLIT_RENDERPASS,
    RENDER_GRAPH_NODE_TYPE_CLEAR_RENDERPASS,
};

// NOTE(Sleepster): Render Command 
struct render_graph_node_t
{
    s32 type;
    render_graph_node_usage_key_t usage_key;
    union 
    {
        struct {
            u32 renderpassID;
        }begin_renderpass;

        struct {
            RHI_pipeline_state_t state;
        }pipeline_state;

        struct {
            asset_handle_t handle;
        }shader;

        struct {
            render_group_constant_buffer_info_t *buffers;
            u32                                  buffer_count;

            asset_handle_t                      *textures;
            u32                                  texture_count;
        }descriptors;

        struct {
            RHI_index_buffer_t           *index_buffer;
            render_group_vertex_stream_t *vertex_streams;
            u32 vertex_stream_count;
            u32 element_count;
            float32 line_width;

            struct {
                vec2_t offset;
                vec2_t extent;
            }viewport;
            struct {
                vec2_t offset;
                vec2_t extent;
            }scissor;
        }draw;

        struct {
            u32 renderpassID;
        }clear;

        struct {
            u32 renderpassA;
            u32 renderpassB;
        }blit;
    };
};

using render_command_array_t = fixed_array_t<render_command_t, 1024>;
using render_group_array_t   = fixed_array_t<render_group_t, 10>;
struct render_graph_t
{
    // NOTE(Sleepster): Data allocated from here has a lifetime of 1 frame 
    memory_arena_t         arena;
    render_group_array_t   render_groups;
    u32                    active_render_group_count;

    render_command_array_t render_commands;
    u32                    render_command_count;

    fixed_array_t<render_graph_node_t, 1024> available_node_pool;
    u32 next_available_node;
};

struct render_state_t 
{
    RHI_context_t                 *RHI_context;
    RHI_image_t                    game_color_buffer;
    RHI_image_t                    game_depth_buffer;

    RHI_image_t                    fullscreen_color_buffer;
    RHI_image_t                    fullscreen_depth_buffer;

    RHI_vertex_buffer_t            vertex_buffer;
    RHI_index_buffer_t             index_buffer;

    RHI_uniform_constant_buffer_t *camera_matrices_buffer;

    u32                            game_renderpass_ID;
    u32                            fullscreen_renderpass_ID; 
    
    render_graph_t                 render_graph;
};

render_group_t* s_render_group_begin(render_graph_t *render_graph, u32 renderpassID);
void s_render_group_end(render_graph_t *render_graph, render_group_t *render_group);
s32 s_render_group_bind_texture(render_group_t *render_group, asset_handle_t texture);
void s_render_graph_output(render_state_t *render_state, RHI_image_t *present_image);
render_group_vertex_stream_t* s_render_group_append_vertex_stream(render_group_t *render_group, u32 element_stride, s32 element_count, RHI_vertex_buffer_t *vertex_buffer);
void s_render_group_add_constant_buffer(render_group_t *render_group, RHI_uniform_constant_buffer_t *constant_buffer, void *data, u32 data_size);
void s_render_graph_command_blit_renderpasses(render_graph_t *render_graph, u32 renderpassA, u32 renderpassB);
void s_render_graph_command_clear_renderpass(render_graph_t *render_graph, u32 renderpassID);

#endif // S_RENDER_GRAPH_H

