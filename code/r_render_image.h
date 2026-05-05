#if !defined(R_RENDER_IMAGE_H)
/* ========================================================================
   $File: r_render_image.h $
   $Date: March 05 2026 12:32 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_IMAGE_H
#include <c_string.h>
#include <vk_backend_image.h>

// NOTE(Sleepster): 
// The invalid states are meant to help us know if the create_info is 
// valid or should be ignored completely. 
enum render_image_filter_type_t 
{
    IMAGE_FILTER_TYPE_INVALID,
    IMAGE_FILTER_TYPE_NEAREST,
    IMAGE_FILTER_TYPE_LINEAR,
};

enum render_image_wrapping_type_t 
{
    IMAGE_WRAPPING_INVALID,
    IMAGE_WRAPPING_CLAMP_TO_EDGE,
    IMAGE_WRAPPING_CLAMP_TO_BORDER,
    IMAGE_WRAPPING_REPEAT,
};

enum render_image_usage_t 
{
    IMAGE_USAGE_INVALID                     = BIT(0),
    IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT = BIT(1),
    IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT = BIT(2),
    IMAGE_USAGE_SHADER_SAMPLED_IMAGE        = BIT(3),
    IMAGE_USAGE_BLIT_SOURCE                 = BIT(4),
    IMAGE_USAGE_BLIT_DESTINATION            = BIT(5),
};

struct sampler_create_info_t
{
    render_image_filter_type_t   filtering;
    bool32                       anisotropy_enabled;
    u32                          max_anisotropy;

    render_image_wrapping_type_t wrapu;
    render_image_wrapping_type_t wrapv;

    bool32                       compare_ops_enabled;
    u32                          compare_operation;

    bool32                       use_normalized_coordinates;
};

struct image_create_info_t
{
    string_t               data;
    u32                    width;
    u32                    height;
    u32                    format;
    render_image_usage_t   usage;
    sampler_create_info_t  sampler_info;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct image_t
{
    u32                 ID;
    image_create_info_t create_info;
    union {
        vulkan_image_t vulkan_image;
    };
};

typedef struct bitmap bitmap_t;
struct renderer_state_t;

image_t s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
image_t s_renderer_image_create_from_bitmap(bitmap_t *bitmap);
void    s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void    s_renderer_image_update_data(renderer_state_t *renderer_state, image_t *image);

#endif // R_RENDER_IMAGE_H

