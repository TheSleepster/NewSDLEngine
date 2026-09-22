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
    (void)window;
}

void RHI_context_t::
backend_handle_window_resize(vec2_t window_size)
{
    (void)window_size;
}
void RHI_context_t::
backend_render_frame(void)
{
    for(u32 command_list_index = 0;
        command_list_index < this->command_list_count;
        ++command_list_index)
    {
        RHI_command_list_t *command_list = this->command_lists + command_list_index;

        command_list->presenting                       = false;
        command_list->active_index_buffer              = null;
        command_list->active_shader_program            = null;
        command_list->active_viewport_command          = null;
        command_list->active_scissor_command           = null;
        command_list->bound_image_count                = 0;
        command_list->image_count                      = 0;
        command_list->bind_material_command_count      = 0;
        command_list->bind_render_target_command_count = 0;
        command_list->bind_shader_command_count        = 0;
        command_list->draw_instance_command_count      = 0;
        command_list->command_count                    = 0;
        command_list->vertex_buffer_count              = 0;

        command_list->active_render_state     = g_pipeline_default_state_key; 
        command_list->active_renderpass       = null;
        command_list->active_index_buffer     = null;
        command_list->active_scissor_command  = null;
        command_list->active_viewport_command = null;
        command_list->active_shader_program   = null;
    }

    this->command_list_count = 0;
    this->present_command    = null;
}

RHI_render_buffer_t RHI_context_t::
backend_buffer_create(RHI_render_buffer_desc_t *buffer_desc)
{
    RHI_render_buffer_t result = {};
    result.type                = buffer_desc->type;
    result.buffer_capacity     = buffer_desc->buffer_capacity;
    result.buffer_element_size = buffer_desc->element_size;
    result.allocation_type     = buffer_desc->allocation_type;

    return(result);
}
void RHI_context_t::
backend_buffer_copy_data(RHI_render_buffer_t *buffer, void *data, u32 size, u32 offset)
{
    (void)buffer;
    (void)data;
    (void)size;
    (void)offset;
}

void RHI_context_t::
backend_buffer_append_data(RHI_render_buffer_t *buffer, void *data, u32 data_size)
{
    (void)buffer;
    (void)data;
    (void)data_size;
}

void* RHI_context_t:: 
backend_constant_buffer_append_data(void *data, u32 data_size, u32 *buffer_offset_out)
{
    (void)data;
    (void)data_size;
    (void)buffer_offset_out;

    return(null);
}

void RHI_context_t::
backend_buffer_reset(RHI_render_buffer_t *buffer)
{
    (void)buffer;
}

u32 RHI_context_t::
backend_renderpass_initialize(RHI_renderpass_desc_t *desc, RHI_renderpass_t *renderpass)
{
    (void)desc;
    (void)renderpass;

    return(0);
}

void RHI_context_t::
backend_image_create(RHI_image_create_info_t *create_info, RHI_image_t *image)
{
    (void)create_info;
    image->backend_image.is_valid = true;
}
void RHI_context_t::
backend_image_destroy(RHI_image_t *image)
{
    image->backend_image.is_valid = false;
}

void RHI_context_t::
backend_image_update_contents(RHI_image_t *image)
{
    (void)image;
}
void RHI_context_t::
backend_acquire_image_sampler(RHI_image_t *image)
{
    (void)image;
}

void RHI_context_t::
backend_shader_create(RHI_shader_t *shader, string_t shader_source)
{
    (void)shader;
    (void)shader_source;
}

backend_command_buffer_t RHI_context_t::
backend_get_command_buffer(RHI_command_list_t *command_list)
{
    (void)command_list;
    return(0);
}
