/* ========================================================================
   $File: backup_renderer.cpp $
   $Date: March 05 2026 11:54 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */


        // NOTE(Sleepster): Test blit image 
        image_create_info_t image_info = {};
        image_info.image_type       = IMAGE_TYPE_ColorAttachment;
        image_info.format           = BMF_RGBA32;
        image_info.width            = 1920;
        image_info.height           = 1080;

        image_t color_buffer = s_renderer_image_create(&renderer_state, &image_info);

        render_target_attachment_info_t color_attachment = {};
        color_attachment.attachment      = &color_buffer;
#if 1
        color_attachment.attachment_type = IMAGE_TYPE_ColorAttachment;
#else 
        color_attachment.attachment_type = RTAT_WriteAttachment;
#endif

        color_attachment.initial_layout  = IMAGE_TYPE_Undefined;
        color_attachment.final_layout    = IMAGE_TYPE_ColorAttachment;
        color_attachment.load_operation  = RTALO_Clear;
        color_attachment.store_operation = RTASO_Store;

        color_attachment.clear_value.clear_color.float_color = vec4(0.4, 0.6, 1.0, 1.0);

        render_target_create_info_t target_info = {};
        target_info.attachments        = &color_attachment;
        target_info.attachment_count   = 1;
        target_info.width              = 1920;
        target_info.height             = 1080;
        target_info.resize_with_window = true;
        render_target_t *test_target = s_renderer_render_target_create(&renderer_state, &target_info);

        // NOTE(Sleepster): Game Texture 
        image_create_info_t game_texture_info = {};
        game_texture_info.image_type = IMAGE_TYPE_ColorAttachment;
        game_texture_info.format     = BMF_RGBA32;
        game_texture_info.width      = 320;
        game_texture_info.height     = 180;

        image_t game_texture = s_renderer_image_create(&renderer_state, &game_texture_info);

        render_target_attachment_info_t game_buffer_attachment = {};
        game_buffer_attachment.attachment      = &game_texture;
        game_buffer_attachment.attachment_type = IMAGE_TYPE_ColorAttachment;
        game_buffer_attachment.initial_layout  = IMAGE_TYPE_Undefined;
        game_buffer_attachment.final_layout    = IMAGE_TYPE_ColorAttachment;
        game_buffer_attachment.load_operation  = RTALO_Clear;
        game_buffer_attachment.store_operation = RTASO_Store;

        game_buffer_attachment.clear_value.clear_color.float_color = vec4(1.0, 0.0, 0.0, 1.0);

        render_target_create_info_t game_target_info = {};
        game_target_info.resize_with_window = false;
        game_target_info.attachments        = &game_buffer_attachment;
        game_target_info.attachment_count   = 1;
        game_target_info.width              = 320;
        game_target_info.height             = 180;

        render_target_t *game_target = s_renderer_render_target_create(&renderer_state, &game_target_info);
        (void)game_target;



            render_command_list_t *command_list = s_renderer_get_command_list(&renderer_state);
            r_cmd_bind_render_target(command_list, test_target);

            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(1, 1, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);
#if 0
            render_command_blit_info_t blit_info = {
                .source             = game_target,
                .destination        = test_target,
                .source_offset      = {0, 0},
                .source_size        = vec2(game_target->create_info.width, game_target->create_info.height),
                .destination_offset = vec2(0, 0),
                .destination_size   = vec2(game_target->create_info.width, game_target->create_info.height),
            };
            r_cmd_blit_render_target(command_list, &blit_info);
#endif

            r_cmd_begin_render_group(command_list);
            r_cmd_draw_rectangle(command_list, vec2(100, 100), vec2(20, 20), vec4(0, 0, 1, 1), 0.0f);
            r_cmd_end_render_group(command_list);

            r_cmd_present(command_list);

//////////////////////////////
/////////////////
/// NEW 

// NOTE(Sleepster):
// Realistically, we don't need these first two functions, we just have them in case you want
// to be able to do more when it comes to initialization like using DynamicArrays or memory
// arenas.

void
s_renderer_frame_graph_desc_init(render_frame_graph_desc_t *desc)
{
    ZeroStruct(*desc);
}

void
s_renderer_renderpass_desc_init(renderpass_desc_t *desc)
{
    ZeroStruct(*desc);
}

void
s_renderer_renderpass_append_color_image(renderpass_desc_t           *renderpass, 
                                         image_t                     *image, 
                                         renderpass_attachment_type_t type, 
                                         clear_value_t                clear_value)
{
    renderpass_attachment_t *attachment = renderpass->color_attachments + renderpass->color_attachment_count++;
    attachment->image       = image;
    attachment->type        = type;
    attachment->clear_value = clear_value;
}

void
s_renderer_renderpas_attach_depth_image(renderpass_desc_t           *renderpass,
                                        image_t                     *image,
                                        renderpass_attachment_type_t type,
                                        clear_value_t                clear_value)
{
    renderpass_attachment_t *attachment = &renderpass->depth_attachment;
    ++renderpass->total_attachment_count;
    
    attachment->image       = image;
    attachment->type        = type;
    attachment->clear_value = clear_value;
}

void
s_renderer_renderpas_attach_stencil_image(renderpass_desc_t           *renderpass,
                                          image_t                     *image,
                                          renderpass_attachment_type_t type,
                                          clear_value_t                clear_value)
{
    renderpass_attachment_t *attachment = &renderpass->stencil_attachment;
    ++renderpass->total_attachment_count;
    
    attachment->image       = image;
    attachment->type        = type;
    attachment->clear_value = clear_value;
}

u32
s_renderer_frame_graph_attach_renderpass(render_frame_graph_desc_t *frame_graph, renderpass_desc_t *renderpass_desc)
{
    u32 ID = frame_graph->renderpass_count;
    renderpass_desc_t *new_desc = frame_graph->renderpass_descs + ID;

    frame_graph->renderpass_count++;
    memcpy(new_desc, renderpass_desc, sizeof(renderpass_desc_t));

    return(ID);
}

internal_api VkImageLayout
get_vulkan_layout_from_attachment(renderpass_attachment_t *attachment)
{
    image_t *image = attachment->image;

    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(image->create_info.format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32:
        {
            result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }break;
        case BMF_D32_SFLOAT_S8_UINT:
        case BMF_D24_SFLOAT_S8:
        case BMF_D32_SFLOAT:
        {
            // NOTE(Sleepster): D32 is still a depth stencil anyway 
            result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }break;
    }

    return(result);
}

internal_api true_inline VkAttachmentLoadOp
get_load_operation(renderpass_attachment_type_t type)
{
    VkAttachmentLoadOp result = VK_ATTACHMENT_LOAD_OP_CLEAR;
    if(type == RenderpassAttachmentRead || type == RenderpassAttachmentReadWrite)
    {
        result = VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    return(result);
}


// NOTE(Sleepster): This is pointless right now, but we may want to expand the abilities here.
internal_api true_inline VkAttachmentStoreOp
get_store_operation(renderpass_attachment_type_t type)
{
    VkAttachmentStoreOp result = VK_ATTACHMENT_STORE_OP_STORE;
    return(result);
}

internal_api true_inline VkImageLayout
get_attachment_type(renderpass_attachment_t *attachment)
{
    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(attachment->image->create_info.format)
    {
        case BMF_R8:
        case BMF_B8:
        case BMF_G8:
        case BMF_RGB24:
        case BMF_RGBA32:
        {
            result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }break;
        case BMF_D32_SFLOAT_S8_UINT:
        case BMF_D24_SFLOAT_S8:
        case BMF_D32_SFLOAT:
        {
            result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }break;
    }

    return(result);
}

internal_api void
construct_frame_graph_renderpass(vulkan_context_t *vulkan_context, frame_graph_renderpass_t *renderpass, renderpass_desc_t *desc, u32 ID)
{
    renderpass->ID               = ID;
    renderpass->width            = desc->attachments->image->create_info.width;
    renderpass->height           = desc->attachments->image->create_info.height;
    renderpass->attachment_count = desc->attachment_count;

    image_t             image_attachments[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkImageLayout       initial_layouts[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkImageLayout       final_layouts[MAX_RENDER_TARGET_ATTACHMENTS]     = {};
    VkAttachmentLoadOp  load_operations[MAX_RENDER_TARGET_ATTACHMENTS]   = {};
    VkAttachmentStoreOp store_operations[MAX_RENDER_TARGET_ATTACHMENTS]  = {};
    VkImageLayout       attachment_types[MAX_RENDER_TARGET_ATTACHMENTS]  = {};

    for(u32 attachment_index = 0;
        attachment_index < desc->attachment_count;
        ++attachment_index)
    {
        renderpass_attachment_t *attachment_data = desc->attachments + attachment_index;
        image_t                 *image           = image_attachments + attachment_index;

        initial_layouts[attachment_index]  = VK_IMAGE_LAYOUT_UNDEFINED;
        final_layouts[attachment_index]    = get_vulkan_layout_from_attachment(attachment_data);
        load_operations[attachment_index]  = get_load_operation(attachment_data->type);
        store_operations[attachment_index] = get_store_operation(attachment_data->type);
        attachment_types[attachment_index] = get_attachment_type(attachment_data);

        renderpass->attachment_clear_values[attachment_index] = attachment_data->clear_value;
        memcpy(image, attachment_data->image, sizeof(image_t));
    }

    renderpass->renderpass_handle = vk_backend_renderpass_create(vulkan_context,
                                                                 image_attachments,
                                                                 desc->attachment_count,
                                                                 initial_layouts,
                                                                 final_layouts,
                                                                 load_operations,
                                                                 store_operations,
                                                                 attachment_types);

    renderpass->framebuffer_handle = vk_backend_framebuffer_create(vulkan_context,
                                                                   renderpass->renderpass_handle,
                                                                   image_attachments,
                                                                   desc->attachment_count,
                                                                   image_attachments[0].create_info.width,
                                                                   image_attachments[0].create_info.height);
}

// TODO(Sleepster): 
// Realistically when we do this for real, we'll want to construct a dependancy tree for each of the
// attachments to know when and where the attachments are actually being used in ways that require their
// store / load operations to be differ.
render_frame_graph_t
s_renderer_frame_graph_construct(renderer_state_t *renderer_state, render_frame_graph_desc_t *frame_graph_desc)
{
    render_frame_graph_t result = {};

    vulkan_context_t *vulkan_context = (vulkan_context_t*)renderer_state->render_context;
    for(u32 renderpass_desc_index = 0;
        renderpass_desc_index < frame_graph_desc->renderpass_count;
        ++renderpass_desc_index)
    {
        renderpass_desc_t        *renderpass_desc = frame_graph_desc->renderpass_descs + renderpass_desc_index;
        frame_graph_renderpass_t *renderpass      = result.renderpasses                + renderpass_desc_index;

        construct_frame_graph_renderpass(vulkan_context, renderpass, renderpass_desc, renderpass_desc_index);
    }

    return(result);
}
