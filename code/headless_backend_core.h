#if !defined(HEADLESS_BACKEND_CORE_H)
/* ========================================================================
   $File: headless_backend_core.h $
   $Date: September 21 2026 07:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define HEADLESS_BACKEND_CORE_H
#include <c_types.h>

// BACKEND RHI DEFINITIONS
// 
// Unfortunately we can't define these in one place across all platforms. We must define these for each
// API so that the values here match what the GPU api expects.

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
    RBM_Zero                  = 0,
    RBM_One                   = 1,
    RBM_SrcColor              = 2,
    RBM_OneMinusSrcColor      = 3,
    RBM_DstColor              = 4,
    RBM_OneMinusDstColor      = 5,
    RBM_SrcAlpha              = 6,
    RBM_OneMinusSrcAlpha      = 7,
    RBM_DstAlpha              = 8,
    RBM_OneMinusDstAlpha      = 9,
    RBM_ConstantColor         = 10,
    RBM_OneMinusConstantColor = 11,
    RBM_ConstantAlpha         = 12,
    RBM_OneMinusConstantAlpha = 13,
    RBM_Count
}render_pipeline_blending_mode_t;

typedef enum render_pipeline_blending_equation
{
    RBE_Add             = 0,
    RBE_Subtract        = 1,
    RBE_ReverseSubtract = 2,
    RBE_Min             = 3,
    RBE_Max             = 4,
}render_pipeline_blending_equation_t;

typedef enum render_pipeline_depth_function
{
    RDF_Never          = 0,
    RDF_Less           = 1,
    RDF_Equal          = 2,
    RDF_LessOrEqual    = 3,
    RDF_Greater        = 4,
    RDF_NotEqual       = 5,
    RDF_GreaterOrEqual = 6,
    RDF_Always         = 7,
}render_pipeline_depth_function_t;

typedef enum render_pipeline_polygon_mode
{
    RENDER_PIPELINE_POLYGON_MODE_FILL  = 0,
    RENDER_PIPELINE_POLYGON_MODE_LINE  = 1,
    RENDER_PIPELINE_POLYGON_MODE_POINT = 2
}render_pipeline_polygon_mode_t;

typedef enum render_pipeline_primitive_type
{
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_POINT_LIST                    = 0,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_LIST                     = 1,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_STRIP                    = 2,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST                 = 3,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP                = 4,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN                  = 5,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY      = 6,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY     = 7,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY  = 8,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_PATCH_LIST                    = 10,
}render_pipeline_primitive_type_t;

typedef struct RHI_pipeline_state 
{
    bool32 blend_enabled         = true;
    u32    src_color_blend_mode  = RBM_SrcAlpha;
    u32    dst_color_blend_mode  = RBM_OneMinusSrcAlpha;

    u32    src_alpha_blend_mode  = RBM_One;
    u32    dst_alpha_blend_mode  = RBM_Zero;

    u32    color_blend_op        = RBE_Add;
    u32    alpha_blend_op        = RBE_Add;

    bool32 depth_testing_enabled = true;
    bool32 depth_writing_enabled = true;
    u32    depth_func            = RDF_Less;

    bool32 stencil_enabled       = false;
    u32    stencil_state         = 0;
    u32    stencil_keep          = 0;

    u32    polygon_mode          = RENDER_PIPELINE_POLYGON_MODE_FILL;
    u32    primitive_type        = RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}RHI_pipeline_state_t;

CODE_GEN_IGNORE_FILE

typedef struct headless_image
{
    bool8 is_valid;
    void *handle;
}headless_image_t;

typedef s32 backend_render_context_t;
typedef s32 backend_command_buffer_t;
typedef s32 backend_renderpass_handle_t;
typedef s32 backend_framebuffer_handle_t;
typedef s32 backend_shader_t;
typedef s32 backend_buffer_t;

typedef headless_image_t backend_image_t;

global_variable constexpr RHI_pipeline_state_t g_pipeline_default_state_key = {};

#endif // HEADLESS_BACKEND_CORE_H

