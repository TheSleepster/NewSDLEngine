/* ========================================================================
   $File: s_RHI_core.cpp $
   $Date: February 22 2026 05:13 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_base.h>
#include <c_types.h>
#include <c_global_context.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_hash_table.h>
#include <c_log.h>

#include <vk_backend_core.h>
#include <s_asset_manager.h>
#include <s_RHI_image.h>
#include <s_RHI_core.h>

/*
=============
s_RHI_context_init
=============
*/

void
RHI_context_init(RHI_context_t *RHI_context, backend_render_context_t *render_context)
{
    RHI_context->RHI_arena       = c_arena_create(MB(100));
    RHI_context->transient_arena = c_arena_create(MB(100));

    RHI_context->command_lists   = c_arena_push_array(&RHI_context->RHI_arena, RHI_command_list_t, RHI_MAX_COMMAND_LISTS);
    RHI_context->constant_buffer_hash = c_hash_table_create<RHI_uniform_constant_buffer_t>(RHI_MAX_CONSTANT_BUFFERS);

    RHI_context->backend_render_context = render_context;
}

/*
=============
RHI_handle_window_resize
=============
*/

void
RHI_handle_window_resize(RHI_context_t *RHI_context, vec2_t window_size)
{
    RHI_context->backend_handle_window_resize(window_size);

    RHI_context->window_size.x = window_size.x;
    RHI_context->window_size.y = window_size.y;

    RHI_context->last_window_size_generation     = RHI_context->current_window_size_generation;
    RHI_context->current_window_size_generation += 1;

    for(u32 renderpass_index = 0;
        renderpass_index < RHI_context->renderpass_count;
        ++renderpass_index)
    {
        RHI_renderpass_t *renderpass = RHI_context->renderpasses + renderpass_index;
        if(renderpass->resize_with_window)
        {
            for(u32 color_attachment_index = 0;
                color_attachment_index < renderpass->color_attachment_count;
                ++color_attachment_index)
            {
                RHI_renderpass_attachment_t *attachment = renderpass->color_attachments + color_attachment_index;

                RHI_image_create_info_t *info = &attachment->image->create_info;
                info->width  = RHI_context->window_size.x;
                info->height = RHI_context->window_size.y;

                Assert(attachment->image->backend_image.is_valid == true);
                RHI_image_destroy(RHI_context, attachment->image);
                *attachment->image = RHI_image_create(RHI_context, info);
            }

            if(renderpass->has_depth_stencil_attachment)
            {
                RHI_image_create_info_t *info = &renderpass->depth_stencil_attachment.image->create_info;
                info->width  = RHI_context->window_size.x;
                info->height = RHI_context->window_size.y;

                Assert(renderpass->depth_stencil_attachment.image->backend_image.is_valid == true);
                RHI_image_destroy(RHI_context, renderpass->depth_stencil_attachment.image);
                *renderpass->depth_stencil_attachment.image = RHI_image_create(RHI_context, info);
            }

            renderpass->create_info.render_width  = RHI_context->window_size.x;
            renderpass->create_info.render_height = RHI_context->window_size.y;
            if(renderpass->create_info.color_attachment_count > 0)
            {
                RHI_resize_renderpass(RHI_context, renderpass);
                log_info("renderpass with ID: '%u' rebuilt with the current window size...\n", renderpass->ID);
            }
        }
    }
}

void
RHI_execute_backend_commands(RHI_context_t *RHI_context)
{
    RHI_context->backend_render_frame();
}

internal_api void 
RHI_renderpass_key(RHI_renderpass_key_t *key, RHI_renderpass_t *renderpass, RHI_renderpass_desc_t *renderpass_desc)
{
    u32 attachment_count = 0;
    for(u32 attachment_index = 0;
        attachment_index < renderpass_desc->color_attachment_count;
        ++attachment_index)
    {
        RHI_renderpass_attachment_t *attachment = renderpass_desc->color_attachments + attachment_index;
        key->attachment_formats[attachment_index] = (bitmap_format_t)attachment->image->create_info.format;

        ++attachment_count;
    }

    if(renderpass->has_depth_stencil_attachment)
    {
        key->attachment_formats[attachment_count] = (bitmap_format_t)renderpass_desc->depth_stencil_attachment.image->create_info.format;
    }
}

/*
=============
RHI_build_renderpass
=============
*/

u32 
RHI_build_renderpass(RHI_context_t *RHI_context, RHI_renderpass_desc_t *renderpass_desc)
{
    u32 result = INVALID_ID;
    RHI_renderpass_t *renderpass = RHI_context->renderpasses + RHI_context->renderpass_count;
    Assert(renderpass);
    
    renderpass->create_info            = *renderpass_desc;
    renderpass->render_width           =  renderpass_desc->render_width;
    renderpass->render_height          =  renderpass_desc->render_height;
    renderpass->color_attachment_count =  renderpass_desc->color_attachment_count;
    renderpass->resize_with_window     =  renderpass_desc->resize_with_window;
    renderpass->ID                     =  RHI_context->renderpass_count++;

    result = renderpass->ID;

    Assert(RHI_context->renderpass_count <= 100);
    Assert(renderpass_desc->color_attachment_count > 0);
    Assert(renderpass_desc->color_attachment_count <= RHI_MAX_RENDER_TARGET_ATTACHMENTS);
    
    // NOTE(Sleepster): Copy color attachments 
    memcpy(renderpass->color_attachments, 
           renderpass_desc->color_attachments, 
           renderpass_desc->color_attachment_count * sizeof(RHI_renderpass_attachment_t));

    // NOTE(Sleepster): Copy depth_stencil attachment 
    memcpy(&renderpass->depth_stencil_attachment, 
           &renderpass_desc->depth_stencil_attachment, 
            sizeof(RHI_renderpass_attachment_t));

    u32 attachment_count = RHI_context->backend_renderpass_initialize(renderpass_desc, renderpass);
    RHI_renderpass_key(&renderpass->renderpass_key, renderpass, renderpass_desc);

    renderpass->total_attachment_count = attachment_count;

    return(result);
}

true_inline void
RHI_resize_renderpass(RHI_context_t *RHI_context, RHI_renderpass_t *renderpass)
{
    RHI_context->backend_renderpass_initialize(&renderpass->create_info, renderpass);

    renderpass->render_width  = renderpass->create_info.render_width;
    renderpass->render_height = renderpass->create_info.render_height;
}

/////////////////////////
// VERTEX AND INDEX BUFFERS 
/////////////////////////

/*
=============
RHI_vertex_buffer_create
=============
*/

true_inline RHI_vertex_buffer_t
RHI_vertex_buffer_create(RHI_context_t                   *RHI_context, 
                         RHI_render_buffer_memory_type_t  memory_type, 
                         RHI_render_buffer_advance_rate_t rate, 
                         byte                            *vertex_buffer_data,
                         u32                              vertex_size, 
                         u32                              max_vertices)
{
    RHI_vertex_buffer_t result;
    RHI_render_buffer_desc_t buffer_desc = {
        .type            = RHI_RENDER_BUFFER_TYPE_VERTEX_BUFFER,
        .allocation_type = memory_type,
        .buffer_capacity = vertex_size * max_vertices,
        .element_size    = vertex_size,
        .initial_data    = 0 
    };

    result.buffer_data  = RHI_render_buffer_create(RHI_context, &buffer_desc);
    result.vertex_count = 0;
    result.max_vertices = max_vertices;
    result.vertex_data  = vertex_buffer_data;
    result.advance_rate = (u32)rate;

    return(result);
}

/*
=============
RHI_index_buffer_create
=============
*/

true_inline RHI_index_buffer_t
RHI_index_buffer_create(RHI_context_t *RHI_context, RHI_render_buffer_memory_type_t memory_type, u32 element_size, void *data, u32 size)
{
    RHI_index_buffer_t result = {};

    RHI_render_buffer_desc_t buffer_desc = {
        .type = RHI_RENDER_BUFFER_TYPE_INDEX_BUFFER,
        .allocation_type = memory_type,
        .buffer_capacity = size,
        .element_size    = element_size,
        .initial_data    = data
    };
    result.buffer_data = RHI_render_buffer_create(RHI_context, &buffer_desc);
    return(result);
}

/*
=============
RHI_render_buffer_create
=============
*/

RHI_render_buffer_t
RHI_render_buffer_create(RHI_context_t            *RHI_context, 
                         RHI_render_buffer_desc_t *buffer_desc)
{
    RHI_render_buffer_t result = {};
    result = RHI_context->backend_buffer_create(buffer_desc);
    if(buffer_desc->initial_data != null && buffer_desc->buffer_capacity > 0)
    {
        RHI_context->backend_buffer_copy_data(&result, buffer_desc->initial_data, buffer_desc->buffer_capacity, 0);
    }

    return(result);
}

/*
=============
RHI_render_buffer_copy_data
=============
*/

true_inline void
RHI_render_buffer_copy_data(RHI_context_t *RHI_context, RHI_render_buffer_t *buffer, void *data, u32 size, u32 offset)
{
    RHI_context->backend_buffer_copy_data(buffer, data, size, offset);
}


/*
=============
RHI_buffer_reset
=============
*/

true_inline void
RHI_buffer_reset(RHI_context_t *RHI_context, RHI_render_buffer_t *buffer)
{
    buffer->buffer_elements_used = 0;
    buffer->working_offset       = 0;

    RHI_context->backend_buffer_reset(buffer);
}

/*
=============
RHI_buffer_reset
=============
*/

true_inline void
RHI_buffer_reset(RHI_context_t *RHI_context, RHI_vertex_buffer_t *buffer)
{
    buffer->vertex_count = 0;
    RHI_buffer_reset(RHI_context, &buffer->buffer_data);
}

/*
=============
RHI_buffer_reset
=============
*/

true_inline void
RHI_buffer_reset(RHI_context_t *RHI_context, RHI_index_buffer_t *buffer)
{
    buffer->index_offset = 0;
    RHI_buffer_reset(RHI_context, &buffer->buffer_data);
}

/////////////////////////
// UNIFORM BUFFERS 
/////////////////////////

// NOTE(Sleepster): 
// This currently can't fail... even if you mess up the uniform buffer name... 
// perhaps that's a bad thing in all honesty...
//
// We could perhaps do what we do for assets and shader parameters where we preload
// the name of the item into the hash table so that if you ask for a constant buffer that literally
// doesn't exist and can't exist, it's a reasonable error.
RHI_uniform_constant_buffer_t*
RHI_get_constant_buffer(RHI_context_t *RHI_context, string_t uniform_name)
{
    RHI_uniform_constant_buffer_t *result = null;
    result = c_hash_table_get_element_ptr(&RHI_context->constant_buffer_hash, uniform_name);
    if(result)
    {
        result->uniform_hash_index  = c_hash_table_hash_key(uniform_name);
        result->uniform_hash_index %= RHI_MAX_CONSTANT_BUFFERS;
    }
    else
    {
        log_fatal("Idk how, but there's no place for this constant_buffer...\n");
        InvalidCodePath;
    }

    return(result);
}

/////////////////////////
// TEXTURE UTILITIES 
/////////////////////////

/*
=============
RHI_is_texture_bound
=============
*/

s32
RHI_is_texture_bound(RHI_command_list_t *command_list, texture2D_t *texture)
{
    s32 result = -1;
    for(u32 bound_textures_index = 0;
        bound_textures_index < command_list->bound_image_count;
        ++bound_textures_index)
    {
        u32 image_id = command_list->image_ids_to_bind[bound_textures_index];
        if(image_id == texture->gpu_data.ID)
        {
            result = bound_textures_index;
            break;
        }
    }

    return(result);
}


/*
=============
RHI_find_texture_index
=============
*/

s32
RHI_find_texture_index(RHI_command_list_t *command_list, u64 ID)
{
    s32 result = -1;
    for(u32 bound_textures_index = 0;
        bound_textures_index < command_list->bound_image_count;
        ++bound_textures_index)
    {
        u32 image_id = command_list->image_ids_to_bind[bound_textures_index];
        if(image_id == ID)
        {
            result = bound_textures_index;
            break;
        }
    }

    return(result);
}

/*
=============
RHI_set_texture_filter_mode
=============
*/

void
RHI_set_texture_filter_mode(RHI_context_t *render_state, texture2D_t *texture, u32 filter_mode)
{
    if(texture->gpu_data.create_info.sampler_info.filtering != filter_mode)
    {
        texture->gpu_data.create_info.sampler_info.filtering = (RHI_image_filter_type_t)filter_mode;
        if(filter_mode == RHI_IMAGE_FILTER_TYPE_LINEAR)
        {
            texture->gpu_data.create_info.sampler_info.use_normalized_coordinates = true;
        }
        else if(filter_mode == RHI_IMAGE_FILTER_TYPE_NEAREST)
        {
            texture->gpu_data.create_info.sampler_info.use_normalized_coordinates = false;
        }

        render_state->backend_acquire_image_sampler(&texture->gpu_data);
    }
}


/////////////////////////
// COMMAND LISTS
/////////////////////////

/*
=============
RHI_command_list_init
=============
*/

internal_api void
RHI_command_list_init(RHI_command_list_t *list)
{
    if(list->is_initialized == false)
    {
        list->transient_arena = c_arena_create(MB(10));
        list->command_arena   = c_arena_create(MB(30));

        //list->active_vertex_buffers = c_dynarray_create(render_buffer_t *);
    }
    else
    {
        c_arena_reset(&list->transient_arena);
        c_arena_reset(&list->command_arena);

        ZeroMemory(list->image_ids_to_bind,   sizeof(u32)          * RHI_MAX_SHADER_IMAGE_PARAMS);
        ZeroMemory(list->image_shader_params, sizeof(RHI_image_t*) * RHI_MAX_SHADER_IMAGE_PARAMS);
    }

    Assert(gc);
    Assert(gc->RHI_context);
    asset_catalog_t *texture_catalog = gc->asset_manager->texture_catalog;

    list->commands               = c_arena_push_array(&list->command_arena, RHI_command_t, RHI_MAX_RENDER_COMMANDS);
    list->active_render_state    = g_pipeline_default_state_key;
    list->image_shader_params[0] = &texture_catalog->default_asset.texture->gpu_data;

    //list->backend_command_buffer = RHI_context->backend_get_command_buffer(list);
    list->is_initialized = true;
}

/*
=============
RHI_get_command_list
=============
*/

RHI_command_list_t*
RHI_get_command_list(RHI_context_t *RHI_context, RHI_command_list_type_t type)
{
    RHI_command_list_t *result = null;
    result = RHI_context->command_lists + RHI_context->command_list_count++;
    Assert(RHI_context->command_list_count < RHI_MAX_COMMAND_LISTS);

    result->RHI_context       = RHI_context;
    result->command_list_type = type;

    RHI_command_list_init(result);
    Assert(result->is_initialized == true);
    Assert(result->transient_arena.is_initialized == true);
    Assert(result->command_arena.is_initialized   == true);

    return(result);
}

/*
=============
RHI_get_next_command
=============
*/

internal_api true_inline RHI_command_t*
RHI_get_next_command(RHI_command_list_t *command_list)
{
    RHI_command_t *result = null;
    result = command_list->commands + command_list->command_count++;

    return(result);
}

/*
=============
RHI_cmd_renderpass_begin
=============
*/

void
RHI_cmd_renderpass_begin(RHI_command_list_t *command_list, u32 renderpassID)
{
    Assert(command_list->active_renderpass == null);
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_begin_renderpass_t *begin_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                                  RHI_command_begin_renderpass_t);
    begin_renderpass->ID = renderpassID;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BEGIN_RENDERPASS;
    command->data                = begin_renderpass;

    command_list->active_renderpass = command_list->RHI_context->renderpasses + renderpassID;
}


/*
=============
RHI_cmd_renderpass_end
=============
*/

// TODO(Sleepster): 
// This is probably a problem. Not really sure why we're actually operating on command list here when it could 
// be accessed from multiple threads
void
RHI_cmd_renderpass_end(RHI_command_list_t *command_list)
{
    Assert(command_list->active_renderpass != null);
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_end_renderpass_t *end_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                           RHI_command_end_renderpass_t);
    end_renderpass->ID = command_list->active_renderpass->ID;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_END_RENDERPASS;
    command->data                = end_renderpass;

    command_list->active_renderpass = null;
}

/*
=============
RHI_cmd_use_shader_program
=============
*/

void
RHI_cmd_use_shader_program(RHI_command_list_t *command_list, asset_handle_t asset_handle)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_bind_shader_t *bind_shader = c_arena_push_struct(&command_list->command_arena, 
                                                                 RHI_command_bind_shader_t);

    asset_slot_load_status_t load_status = asset_handle.slot->slot_state; 
    if(load_status != ASLS_Loaded)
    {
        Assert(asset_handle.slot->catalog);
        if(load_status == ASLS_Unloaded)
        {
            // signal for load
        }
        // apply a default texture
        bind_shader->shader = asset_handle.slot->catalog->default_asset;
    }
    else if(load_status == ASLS_Loaded)
    {
        bind_shader->shader = asset_handle;
    }

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BIND_SHADER;
    command->data = bind_shader;
}

/*
=============
RHI_cmd_bind_texture_image
=============
*/

void
RHI_cmd_bind_texture_image(RHI_command_list_t *command_list, texture2D_t *texture)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
    Assert(texture);

    if(texture->gpu_data.ID > 0)
    {
        RHI_command_t *command = RHI_get_next_command(command_list);
        RHI_command_bind_texture_t *bind_texture = c_arena_push_struct(&command_list->command_arena, 
                                                                        RHI_command_bind_texture_t);
        bind_texture->texture = &texture->gpu_data;

        command->header.command_type = RHI_RENDER_COMMAND_TYPE_BIND_TEXTURE;
        command->data = bind_texture;

        command_list->image_ids_to_bind[command_list->bound_image_count++] = bind_texture->texture->ID;
    }
    else
    {
        log_warning("Attempting to bind a texture where it's ID == 0. This may be intentional...\n");
    }
}

/*
=============
RHI_cmd_bind_texture_from_handle
=============
*/

void
RHI_cmd_bind_texture_from_handle(RHI_command_list_t *command_list, asset_handle_t *asset_handle)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
    Assert(asset_handle->slot->type == AT_Bitmap);

    texture2D_t *texture = null;
    asset_slot_load_status_t load_status = asset_handle->slot->slot_state; 
    if(load_status != ASLS_Loaded)
    {
        if(load_status == ASLS_Unloaded)
        {
            // signal for load
        }
        // apply a default texture
        texture = asset_handle->slot->catalog->default_asset.texture;
        Assert(texture->ID != 0);
    }
    else if(load_status == ASLS_Loaded)
    {
        texture = asset_handle->texture;
    }

    RHI_cmd_bind_texture_image(command_list, texture);
}

/*
=============
RHI_cmd_bind_vertex_buffer
=============
*/

void
RHI_cmd_bind_vertex_buffer(RHI_command_list_t *command_list, RHI_render_buffer_t *buffer)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_bind_vertex_buffer_t *bind_vertex_buffer = c_arena_push_struct(&command_list->command_arena, 
                                                                                RHI_command_bind_vertex_buffer_t);
    Assert(buffer->type == RHI_RENDER_BUFFER_TYPE_VERTEX_BUFFER);
    bind_vertex_buffer->vertex_buffer = buffer; 

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BIND_VERTEX_BUFFER;
    command->data = bind_vertex_buffer;
}

true_inline void 
RHI_cmd_bind_vertex_buffer(RHI_command_list_t *command_list, RHI_vertex_buffer_t *buffer)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
    RHI_cmd_bind_vertex_buffer(command_list, &buffer->buffer_data);
}

/*
=============
RHI_cmd_bind_index_buffer
=============
*/

void
RHI_cmd_bind_index_buffer(RHI_command_list_t *command_list, RHI_index_buffer_t *buffer)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_bind_index_buffer_t *bind_index_buffer = c_arena_push_struct(&command_list->command_arena, 
                                                                              RHI_command_bind_index_buffer_t);

    Assert(buffer->buffer_data.type == RHI_RENDER_BUFFER_TYPE_INDEX_BUFFER);
    bind_index_buffer->index_buffer = buffer; 

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BIND_INDEX_BUFFER;
    command->data = bind_index_buffer;
}

/*
=============
RHI_cmd_set_viewport
=============
*/

void
RHI_cmd_set_viewport(RHI_command_list_t *command_list, vec2_t offset, vec2_t size)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_set_viewport_t *set_viewport = c_arena_push_struct(&command_list->command_arena, 
                                                                   RHI_command_set_viewport_t);
    set_viewport->size   = vec2(size.x, size.y);
    set_viewport->offset = vec2(offset.x, offset.y);

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_SET_VIEWPORT;
    command->data = set_viewport;
}

/*
=============
RHI_cmd_set_scissor
=============
*/

void
RHI_cmd_set_scissor(RHI_command_list_t *command_list, vec2_t offset, vec2_t size)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_set_scissor_t *set_scissor = c_arena_push_struct(&command_list->command_arena, 
                                                                  RHI_command_set_scissor_t);
    set_scissor->size   = size;
    set_scissor->offset = offset;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_SET_SCISSOR;
    command->data = set_scissor;
}

/*
=============
RHI_cmd_update_push_constants
=============
*/

void
RHI_cmd_update_push_constants(RHI_command_list_t *command_list, u32 offset, u32 size, void *data) 
{
    Assert(size <= 128);

    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_update_push_constant_t *update_constant = c_arena_push_struct(&command_list->command_arena, 
                                                                               RHI_command_update_push_constant_t);
    update_constant->data   = data;
    update_constant->size   = size;
    update_constant->offset = offset;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_UPDATE_PUSH_CONSTANTS;
    command->data = update_constant;
}

/*
=============
RHI_cmd_update_constant_buffer
=============
*/

void
RHI_cmd_update_constant_buffer(RHI_command_list_t *command_list, RHI_uniform_constant_buffer_t *buffer, void *data, u32 data_size)
{
    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_update_uniform_constant_buffer_t *update_buffer_contents = c_arena_push_struct(&command_list->command_arena, 
                                                                                                RHI_command_update_uniform_constant_buffer_t);
    update_buffer_contents->constant_data_size = data_size;

    // TODO(Sleepster): Abstract this... 
    update_buffer_contents->buffer                     = buffer;
    update_buffer_contents->uniform_hash_index         = buffer->uniform_hash_index;
    update_buffer_contents->backend_uniform_buffer_ptr = command_list->RHI_context->backend_constant_buffer_append_data(data, 
                                                                                                                        data_size, 
                                                                                                                       &buffer->offset);
    update_buffer_contents->constant_buffer_offset = buffer->offset;

    buffer->mapped_data = update_buffer_contents->backend_uniform_buffer_ptr;
    buffer->size        = data_size;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_UPDATE_UNIFORM_CONSTANT_BUFFER;
    command->data                = update_buffer_contents;
}

/*
=============
RHI_cmd_update_buffer_contents
=============
*/

void
RHI_cmd_update_buffer_contents(RHI_command_list_t *command_list, RHI_render_buffer_t *buffer, void *data, u32 data_size)
{
    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_update_render_buffer_contents_t *update_buffer_contents = c_arena_push_struct(&command_list->command_arena, 
                                                                                              RHI_command_update_render_buffer_contents_t);
    update_buffer_contents->data_size = data_size;
    update_buffer_contents->offset    = buffer->buffer_elements_used;
    update_buffer_contents->buffer    = buffer;

    buffer->buffer_elements_used += (data_size / buffer->buffer_element_size);
    command_list->RHI_context->backend_buffer_append_data(buffer, data, data_size);

    command->data = update_buffer_contents; 
    command->header.command_type = RHI_RENDER_COMMAND_TYPE_UPDATE_BUFFER_CONTENTS;
}

/*
=============
RHI_cmd_update_buffer_contents
=============
*/

// TODO(Sleepster): I don't feel good about this... this feels like a good way to blow something up and not know
true_inline void
RHI_cmd_update_buffer_contents(RHI_command_list_t *command_list, RHI_vertex_buffer_t *buffer)
{
    RHI_cmd_update_buffer_contents(command_list, 
                                  &buffer->buffer_data, 
                                   buffer->vertex_data, 
                                   buffer->vertex_count * buffer->buffer_data.buffer_element_size);
}

/*
=============
RHI_cmd_update_buffer_contents
=============
*/

// TODO(Sleepster): I don't feel good about this... this feels like a good way to blow something up and not know x2
true_inline void
RHI_cmd_update_buffer_contents(RHI_command_list_t *command_list, RHI_index_buffer_t *buffer)
{
    RHI_cmd_update_buffer_contents(command_list, 
                                  &buffer->buffer_data, 
                                   buffer->index_data, 
                                   buffer->index_count * buffer->buffer_data.buffer_element_size);
}

/*
=============
RHI_cmd_set_render_state
=============
*/

void
RHI_cmd_set_render_state(RHI_command_list_t *command_list, RHI_pipeline_state_t *render_pipeline_state)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command = RHI_get_next_command(command_list);
    RHI_command_set_pipeline_state_t *set_render_state = c_arena_push_struct(&command_list->command_arena, 
                                                                             RHI_command_set_pipeline_state_t);
    set_render_state->pipeline_state = *render_pipeline_state;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_SET_RENDER_STATE;
    command->data = set_render_state;
}


/*
=============
RHI_cmd_reset_render_state
=============
*/

void
RHI_cmd_reset_render_state(RHI_command_list_t *command_list)
{
    RHI_command_t *command  = RHI_get_next_command(command_list);
    command->header.command_type = RHI_RENDER_COMMAND_TYPE_RESET_RENDER_STATE;
}

/*
=============
RHI_cmd_draw
=============
*/

void
RHI_cmd_draw(RHI_command_list_t *command_list, 
             u32                 vertex_count, 
             u32                 vertex_offset, 
             u32                 instance_count, 
             u32                 first_instance)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command   = RHI_get_next_command(command_list);
    RHI_command_draw_t *draw = c_arena_push_struct(&command_list->command_arena, 
                                                    RHI_command_draw_t);
    draw->vertices_to_draw = vertex_count;
    draw->vertex_offset    = vertex_offset;
    draw->instance_count   = instance_count;
    draw->first_instance   = first_instance;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_DRAW;
    command->data = draw;

    command_list->bound_image_count = 0;
}

/*
=============
RHI_cmd_draw_indexed
=============
*/

void
RHI_cmd_draw_indexed(RHI_command_list_t *command_list, 
                     u32                 index_count, 
                     u32                 index_offset, 
                     u32                 instance_count, 
                     u32                 first_instance)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command   = RHI_get_next_command(command_list);
    RHI_command_draw_t *draw = c_arena_push_struct(&command_list->command_arena, 
                                                    RHI_command_draw_t);
    draw->indices_to_draw  = index_count;
    draw->index_offset     = index_offset;
    draw->instance_count   = instance_count;
    draw->first_instance   = first_instance;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_DRAW_INDEXED;
    command->data = draw;

    command_list->bound_image_count = 0;
}

/*
=============
RHI_cmd_blit_image
=============
*/

void
RHI_cmd_blit_image(RHI_command_list_t *command_list, 
                   RHI_image_t        *source_image, 
                   RHI_image_t        *dest_image, 
                   vec2_t              source_offset, 
                   vec2_t              source_blit_size, 
                   vec2_t              dest_offset, 
                   vec2_t              dest_blit_size)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command   = RHI_get_next_command(command_list);
    RHI_command_blit_image_t *blit_image = c_arena_push_struct(&command_list->command_arena, 
                                                               RHI_command_blit_image_t);
    blit_image->source_image  = source_image;
    blit_image->dest_image    = dest_image;
    blit_image->source_offset = source_offset;
    blit_image->source_size   = source_blit_size;
    blit_image->dest_offset   = dest_offset;
    blit_image->dest_size     = dest_blit_size;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BLIT_IMAGE;
    command->data = blit_image;
}


/*
=============
RHI_cmd_blit_renderpass
=============
*/

void
RHI_cmd_blit_renderpass(RHI_command_list_t *command_list, u32 source_ID, u32 destination_ID)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command   = RHI_get_next_command(command_list);
    RHI_command_blit_renderpass_t *blit_renderpass = c_arena_push_struct(&command_list->command_arena, 
                                                                          RHI_command_blit_renderpass_t);
    
    RHI_renderpass_t *source      = command_list->RHI_context->renderpasses + source_ID;
    RHI_renderpass_t *destination = command_list->RHI_context->renderpasses + destination_ID;

    Expect(source->total_attachment_count == destination->total_attachment_count, 
           "Renderpasses must have the same attachment count in order to command a blit...\n");

    Expect(source->has_depth_stencil_attachment == destination->has_depth_stencil_attachment, 
           "Renderpasses must both have the same KINDS of renderpass attachments, in this case one of them is missing a depth attachment...\n");

    blit_renderpass->source      = source;
    blit_renderpass->destination = destination;
    command->header.command_type = RHI_RENDER_COMMAND_TYPE_BLIT_RENDERPASS;

    command->data = blit_renderpass;
}

/*
=============
RHI_cmd_clear_image
=============
*/

// TODO(Sleepster): This function
void
RHI_cmd_clear_image()
{
    Expect(false, "This function is not implemented...\n");
}

/*
=============
RHI_cmd_dispatch_compute
=============
*/

void
RHI_cmd_dispatch_compute(RHI_command_list_t *command_list, u32 invoke_x, u32 invoke_y, u32 invoke_z)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_COMPUTE);

    RHI_command_t *command   = RHI_get_next_command(command_list);
    RHI_command_dispatch_compute_t *dispath_compute = c_arena_push_struct(&command_list->command_arena, 
                                                                           RHI_command_dispatch_compute_t);
    dispath_compute->invoke_x = invoke_x;
    dispath_compute->invoke_y = invoke_y;
    dispath_compute->invoke_z = invoke_z;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_DISPATCH_COMPUTE;
    command->data                = dispath_compute;
}

/*
=============
RHI_cmd_present
=============
*/

// NOTE(Sleepster): 
// The cake is a lie. This does not actually present, but rather blits to the swapchain image,
// which is later presented when appropriate
//
// Also, when you call this command any commands after this is called, are an error.
void
RHI_cmd_present(RHI_command_list_t *command_list, RHI_image_t *presentation_source)
{
    Assert(command_list->command_list_type == RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    RHI_command_t *command  = RHI_get_next_command(command_list);
    RHI_command_present_frame_t *present = c_arena_push_struct(&command_list->command_arena, 
                                                                RHI_command_present_frame_t);
    present->presentation_source = presentation_source;

    command->header.command_type = RHI_RENDER_COMMAND_TYPE_PRESENT_FRAME;
    command->data = present;
}
