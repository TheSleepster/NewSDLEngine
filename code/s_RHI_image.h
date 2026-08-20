#if !defined(R_RENDER_IMAGE_H)
/* ========================================================================
   $File: s_RHI_image.h $
   $Date: March 05 2026 12:32 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_IMAGE_H
#include <c_string.h>
#include <vk_backend_core.h>

// NOTE(Sleepster): 
// The invalid states are meant to help us know if the create_info is 
// valid or should be ignored completely. 
enum RHI_image_filter_type_t 
{
    RHI_IMAGE_FILTER_TYPE_INVALID,
    RHI_IMAGE_FILTER_TYPE_NEAREST,
    RHI_IMAGE_FILTER_TYPE_LINEAR,
};

enum RHI_image_wrapping_type_t 
{
    RHI_IMAGE_WRAPPING_INVALID,
    RHI_IMAGE_WRAPPING_CLAMP_TO_EDGE,
    RHI_IMAGE_WRAPPING_CLAMP_TO_BORDER,
    RHI_IMAGE_WRAPPING_REPEAT,
};

enum RHI_image_usage_t 
{
    RHI_IMAGE_USAGE_INVALID                     = BIT(0),
    RHI_IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT = BIT(1),
    RHI_IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT = BIT(2),
    RHI_IMAGE_USAGE_SHADER_SAMPLED_IMAGE        = BIT(3),
    RHI_IMAGE_USAGE_BLIT_SOURCE                 = BIT(4),
    RHI_IMAGE_USAGE_BLIT_DESTINATION            = BIT(5),
};

struct RHI_sampler_create_info_t
{
    RHI_image_filter_type_t   filtering;
    bool32                    anisotropy_enabled;
    u32                       max_anisotropy;

    RHI_image_wrapping_type_t wrapu;
    RHI_image_wrapping_type_t wrapv;

    u32                       compare_operation;
    bool32                    compare_ops_enabled;
    bool32                    use_normalized_coordinates;
};

struct RHI_image_create_info_t
{
    string_t                  data;
    u32                       width;
    u32                       height;
    u32                       format;
    RHI_image_usage_t         usage;
    RHI_sampler_create_info_t sampler_info;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct RHI_image_t
{
    u32                     ID;
    RHI_image_create_info_t create_info;
    backend_image_t         backend_image;
};

typedef struct bitmap bitmap_t;
struct RHI_context_t;

RHI_image_t RHI_image_create(RHI_context_t *RHI_context, RHI_image_create_info_t *image_create_info);
RHI_image_t RHI_image_create_from_bitmap(bitmap_t *bitmap);
void        RHI_image_destroy(RHI_context_t *RHI_context, RHI_image_t *image);
void        RHI_image_update_data(RHI_context_t *RHI_context, RHI_image_t *image);

#endif // R_RENDER_IMAGE_H

