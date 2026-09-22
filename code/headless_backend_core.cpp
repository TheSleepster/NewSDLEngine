/* ========================================================================
   $File: headless_backend_core.cpp $
   $Date: September 21 2026 07:24 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <headless_backend_core.h>
#include <s_RHI_core.h>

void RHI_context_t::
backend_initialize(SDL_Window *window)
{
}

void RHI_context_t::
backend_handle_window_resize(vec2_t window_size)
{
}
void RHI_context_t::
backend_render_frame(void)
{
}

RHI_render_buffer_t RHI_context_t::
backend_buffer_create(RHI_render_buffer_desc_t *buffer_desc)
{
    return(0);
}
void RHI_context_t::
backend_buffer_copy_data(RHI_render_buffer_t *buffer, void *data, u32 size, u32 offset)
{
}

void RHI_context_t::
backend_buffer_append_data(RHI_render_buffer_t *buffer, void *data, u32 data_size)
{
}

void* RHI_context_t:: 
backend_constant_buffer_append_data(void *data, u32 data_size, u32 *buffer_offset_out)
{
    return(null);
}

void RHI_context_t::
backend_buffer_reset(RHI_render_buffer_t *buffer)
{
}

u32 RHI_context_t::
backend_renderpass_initialize(RHI_renderpass_desc_t *desc, RHI_renderpass_t *renderpass)
{
    return(0);
}

void RHI_context_t::
backend_image_create(RHI_image_create_info_t *create_info, RHI_image_t *image)
{
}
void RHI_context_t::
backend_image_destroy(RHI_image_t *image)
{
}

void RHI_context_t::
backend_image_update_contents(RHI_image_t *image)
{
}
void RHI_context_t::
backend_acquire_image_sampler(RHI_image_t *image)
{
}

void RHI_context_t::
backend_shader_create(RHI_shader_t *shader, string_t shader_source)
{
}

backend_command_buffer_t RHI_context_t::
backend_get_command_buffer(RHI_command_list_t *command_list)
{
    return(0);
}
