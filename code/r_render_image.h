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

struct image_create_info_t
{
    string_t data;
    u32      width;
    u32      height;
    u32      format;
};

// NOTE(Sleepster): The asset manager should track the actual asset file data. The renderer
// is in charge of tracking rendering-related GPU resources. Meaning, this is fine here.
struct image_t
{
    image_create_info_t create_jnfo;
    union {
        vulkan_image_t vulkan_image;
    };
};

struct renderer_state_t;
image_t s_renderer_image_create(renderer_state_t *render_state, image_create_info_t *image_create_info);
void    s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image);
void    s_renderer_image_update_data(void *backend_context, image_t *image);


#endif // R_RENDER_IMAGE_H

