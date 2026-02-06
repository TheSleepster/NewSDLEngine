#if !defined(R_RENDER_GROUP_H)
/* ========================================================================
   $File: r_render_group.h $
   $Date: January 14 2026 07:07 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_memory_arena.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_math.h>

#include <s_asset_manager.h>

#define R_RENDER_GROUP_H

#define MAX_RENDER_LAYERS (32)

GENERATE_TYPE_INFO
typedef enum renderer_effect_application_flags
{
    REAF_None,
    REAF_Bloom,
    REAF_Emmision,
    REAF_Vignette,
    REAF_FilmGrain,
    REAF_Count,
}renderer_effect_application_flags_t;

GENERATE_TYPE_INFO
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

GENERATE_TYPE_INFO
typedef enum render_pipeline_blending_equation
{
    RBE_Invalid,
    RBE_Add,
    RBE_Subtract,
    RBE_ReverseSubtract,
    RBE_Min,
    RBE_Max,
}render_pipeline_blending_equation_t;

GENERATE_TYPE_INFO
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

// TODO(Sleepster): This seems unnecessary for now
#if 0
typedef enum render_pipeline_stencil_function
{
}render_pipeline_stencil_function_t;
#endif

struct render_geometry_instance_t 
{
    mat4_t  transform;
    vec4_t  color;
    vec2_t  uv_min;
    vec2_t  uv_max;
    u32     texture_index;
    u32     camera_index;
};

// NOTE(Sleepster): This stores all the geometry information needed when rendering. The "vertices" array contains all the vertex data associated with this buffer.
//                  Since render_groups are simply hashed from IDs and such, we can afford to make the vertex array here smaller and create lists of them if we need more.
struct render_geometry_batch_t
{
    render_camera_t             camera_data;

    bool32                      is_valid;
    u32                         primitive_count;
    u32                         master_array_start_offset;

    // NOTE(Sleepster):       size: 2500 
    render_geometry_instance_t *instances;
    render_geometry_batch_t    *next_buffer;
};

// NOTE(Sleepster): Stores the state that is shared between all the buffers attached to this render_group. 
//                  Items like dynamic pipeline state, the used shader (eventually material), textures needed, and the master_vertex_array. 
//                  This master vertex array stores all the vertices across all geometry_buffers attached to the render_group.
struct render_group_t 
{
    // TODO(Sleepster): Custom Viewport? Custom Scissor?
    u64                            ID;

    // TODO(Sleepster): Merge these into the material 
    render_pipeline_state_t        dynamic_pipeline_state;
    asset_handle_t                *shader;
    //

    texture2D_t                   *textures[MAX_RENDER_GROUP_BOUND_TEXTURES];
    u32                            current_texture_count;

    // NOTE(Sleepster):            size: 10000, if this fills, expand it doubly.
    render_geometry_instance_t    *master_batch_array;
    u32                            total_primitive_count;

    render_geometry_batch_t       *cached_buffer;
    render_geometry_batch_t        first_buffer;
};

struct draw_frame_t
{
    // NOTE(Sleepster): Array of what render_groups are used this frame. size of MAX_RENDER_GROUPS
    render_group_t **used_render_groups;
    u32              used_render_group_count;

    render_camera_t  used_camera[MAX_RENDER_GROUP_CAMERA_COUNT];
    u32              used_camera_count;

    struct
    {
        u64                     cached_camera_ID;
        u32                     active_render_layer;
        render_group_t         *active_render_group;
        render_camera_t        *active_camera;
        asset_handle_t         *active_shader;
        render_pipeline_state_t active_pipeline_state;
    }state;
};

// NOTE(Sleepster): Stores the hash_table of render_groups, these are hash tabled so that an ID lookup will always give us the same render_group so long 
//                  as the state is the same. And when that render_group is used that frame, we store it's pointer inside the render_groups array and 
//                  increment the render_group_used_this_frame counter.
struct render_state_t
{
    bool8                        is_initialized;
    memory_arena_t               renderer_arena;

    vulkan_render_context_t     *render_context;
    vulkan_render_frame_state_t *current_frame_data;
    HashTable_t(render_group_t)  render_group_hash;

    draw_frame_t                 draw_frame;
};

render_group_t* r_render_group_begin(render_state_t *render_state);
void            r_render_group_end(render_state_t *render_state);
void            r_render_state_init(render_state_t *render_state, vulkan_render_context_t *render_context);
void            r_render_group_update_used_groups(render_state_t *render_state);

void r_push_texture_ex(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color, float32 rotation, subtexture_data_t *subtexture_data);
void r_push_texture(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color, float32 rotation, asset_handle_t *texture_handle);
void r_push_rect(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color, float32 rotation);

#endif // R_RENDER_GROUP_H

