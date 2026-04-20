/* ========================================================================
   $File: r_render_image.cpp $
   $Date: March 05 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <r_render_image.h>
#include <s_render_RHI.h>

image_t 
s_renderer_image_create(renderer_state_t *renderer_state, image_create_info_t *image_create_info)
{
    image_t result = {};
    result.ID          = c_fnv_hash_value((byte*)image_create_info, sizeof(image_create_info_t));
    result.create_info = *image_create_info;

    renderer_state->backend_image_create(image_create_info, &result);
    return(result);
}

image_t 
s_renderer_image_create_from_bitmap(bitmap_t *bitmap)
{
    image_t result;
    image_create_info_t info = {};
    info.width          = bitmap->width;
    info.height         = bitmap->height;
    info.data           = bitmap->pixels;
    info.format         = bitmap->format;
    info.usage          = ImageUsage_SampledTexture;

    result = s_renderer_image_create(global_context->renderer_state, &info);

    return(result);
}

void
s_renderer_image_destroy(renderer_state_t *renderer_state, image_t *image)
{
    renderer_state->backend_image_destroy(image);
}

void
s_renderer_image_update_data(renderer_state_t *renderer_state, image_t *image)
{
    renderer_state->backend_image_update_contents(image);
}


