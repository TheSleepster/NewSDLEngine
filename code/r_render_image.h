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
    ImageFilterType_Invalid,
    ImageFilterType_Nearest,
    ImageFilterType_Linear,
};

enum render_image_wrapping_type_t 
{
    ImageWrapping_Invalid,
    ImageWrapping_ClampToEdge,
    ImageWrapping_ClampToBorder,
    ImageWrapping_Repeat,
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
    sampler_create_info_t  sampler_info;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct image_t
{
    image_create_info_t create_info;
    union {
        vulkan_image_t vulkan_image;
    };
};

struct renderer_state_t;
image_t s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
void    s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void    s_renderer_image_update_data(void *backend_context, image_t *image);

VkFormat          s_renderer_bitmap_format_to_vulkan_format(u32 bitmap_format);
VkImageUsageFlags s_renderer_image_usage_flags_from_image_format(image_create_info_t *image_create_info);

#endif // R_RENDER_IMAGE_H

