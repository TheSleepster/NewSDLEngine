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
    u32 pipeline_state_hash;
    u32 shader_hash;
    u32 vertex_input_hash;
    u32 descriptor_hash;
    u32 submission_index;
};

struct render_group_vertex_stream_t
{
    RHI_vertex_buffer_t *vertex_buffer;

    void *vertices;
    u32   vertex_count;
    u32   vertex_offset;
    u32   vertex_stride;
};

struct render_group_constant_buffer_info_t
{
    RHI_uniform_constant_buffer_t *buffer;

    void *buffer_data;
    u32   data_size;
};

using render_group_texture_array_t = fixed_array_t<asset_handle_t, RHI_MAX_SHADER_IMAGE_PARAMS>;
using vertex_stream_array_t        = fixed_array_t<render_group_vertex_stream_t, 10>;
using constant_buffer_array_t      = fixed_array_t<render_group_constant_buffer_info_t, 10>;
struct render_group_t
{
    memory_arena_t               push_buffer;
    render_group_sort_key_t      sort_key;
    u32                          ID;
    u32                          renderpassID;

    bool8                        clear_attachments;

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

using render_group_array_t = fixed_array_t<render_group_t, 10>;
struct render_graph_section_t
{
    s32                  sectionID;
    render_group_array_t render_groups;
    u32                  used_render_group_count;
};

struct render_graph_blit_command_t
{
    u32 renderpassA;
    u32 renderpassB;
};

enum render_graph_command_type_t
{
    RENDER_GRAPH_COMMAND_SET_TYPE_RENDER_SECTION,
    RENDER_GRAPH_COMMAND_SET_TYPE_BLIT_SECTION,
    RENDER_GRAPH_COMMAND_SET_TYPE_CLEAR_RENDERPASS_ATTACHMENTS,
};

struct render_graph_command_t 
{
    s32 type;
    union {
        render_graph_section_t      *render_section;
        render_graph_blit_command_t  blit_command;
        struct {
            u32 renderpassID;
        }clear_renderpass;
    };
};

struct render_graph_t
{
    fixed_array_t<render_graph_section_t, 32> render_sections;
    u32                                       render_section_count;

    fixed_array_t<render_graph_command_t, 32> command_nodes;
    u32                                       command_node_count;
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

true_inline render_graph_command_t* s_render_graph_get_next_command(render_graph_t *render_graph);
render_graph_section_t* s_render_graph_acquire_render_section(render_graph_t *render_graph, s32 sectionID);
render_group_t* s_render_group_begin(render_graph_section_t *section, u32 renderpassID);
void s_render_group_end(render_group_t *render_group);
s32 s_render_group_append_vertex_stream(render_group_t *render_group, u32 vertex_stride, RHI_vertex_buffer_t *vertex_buffer);
void s_render_group_push_vertex_stream_data(render_group_t *render_group, s32 vertex_stream_index, void *buffer, s32 vertex_count);
s32 s_render_group_bind_texture(render_group_t *render_group, asset_handle_t texture);
void s_render_group_add_constant_buffer(render_group_t *render_group, RHI_uniform_constant_buffer_t *buffer, void *buffer_data, u32 data_size);
void s_render_graph_add_section_barrier(render_graph_command_t *command, render_graph_section_t *dependancy);
void s_render_graph_cmd_blit_renderpasses(render_graph_command_t *command, u32 A, u32 B);
void s_render_graph_cmd_clear_renderpass_attachments(render_graph_t *render_graph, u32 renderpassID);
void render_groups_to_output(render_state_t *render_state);

#endif // S_RENDER_GRAPH_H

