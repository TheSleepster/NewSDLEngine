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

