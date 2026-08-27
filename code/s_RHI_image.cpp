/* ========================================================================
   $File: s_RHI_image.cpp $
   $Date: March 05 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_RHI_image.h>
#include <s_RHI_core.h>

RHI_image_t 
RHI_image_create(RHI_context_t *RHI_context, RHI_image_create_info_t *image_create_info)
{
    RHI_image_t result = {};
    result.ID          = c_hash_table_hash_key(string_t{(byte*)image_create_info, sizeof(RHI_image_create_info_t)});
    result.create_info = *image_create_info;

    RHI_context->backend_image_create(image_create_info, &result);
    return(result);
}

RHI_image_t 
RHI_image_create_from_bitmap(bitmap_t *bitmap)
{
    RHI_image_t result;
    RHI_image_create_info_t info = {};
    info.width          = bitmap->width;
    info.height         = bitmap->height;
    info.data           = bitmap->pixels;
    info.format         = bitmap->format;
    info.usage          = RHI_IMAGE_USAGE_SHADER_SAMPLED_IMAGE;

    result = RHI_image_create(gc->RHI_context, &info);

    return(result);
}

void
RHI_image_destroy(RHI_context_t *RHI_context, RHI_image_t *image)
{
    RHI_context->backend_image_destroy(image);
}

void
RHI_image_update_data(RHI_context_t *RHI_context, RHI_image_t *image)
{
    RHI_context->backend_image_update_contents(image);
}


