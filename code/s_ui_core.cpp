/* ========================================================================
   $File: s_ui_core.cpp $
   $Date: April 27 2026 03:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_ui_core.h>
#include <r_immediate_rendering.h>

internal_api void* widget_hash_table_allocate_impl(void *allocator, u32 allocation_size);
void ui_state_update_widget_state(ui_state_t *ui_state);

internal_api void ui_state_update_widget_hierarchy(ui_state_t *ui_state);
internal_api void size_all_widgets(ui_state_t *ui_state);
internal_api void place_all_widgets(ui_state_t *ui_state);
internal_api void render_widget_hierarchy(ui_state_t *ui_state, RHI_command_list_t *command_list, widget_t *first_widget);

/*
=============
find_top_level_in_bounds_widget

Helper for ui_state_poll_input_events
=============
*/

internal_api widget_t* 
find_top_level_in_bounds_widget(ui_state_t *ui_state, widget_t *widget, vec2_t mouse_position)
{
    widget_t *result = null;
    widget_t *first_widget = widget;
    do {
        if(widget->first_child)
        { 
            widget_t *top_level_child = find_top_level_in_bounds_widget(ui_state, widget->first_child, mouse_position);
            if(result && top_level_child)
            {
                if(top_level_child->parent_stack_depth < result->parent_stack_depth)
                {
                    result = top_level_child;
                }
            }
            else if(!result)
            {
                result = top_level_child;
            }
        }

        widget_state_t *state = &(ui_state->widget_states.items[widget->ID]).item;
        state->is_held = false;
        state->just_released = false;
        state->just_clicked = false;
        state->is_double_clicked = false;
        state->is_right_clicked = false;

        bool8 within_widget_bounds = rect2_point_in_rect(state->widget_rect, mouse_position);
        if(within_widget_bounds && (widget->widget_flags & UI_WIDGET_FLAG_INTERACTABLE))
        {
            if(result)
            {
                if(widget->parent_stack_depth < result->parent_stack_depth)
                {
                    result = widget;
                }
            }
            else
            {
                result = widget;
            }
        }

        widget = widget->next_sibling;
    }while(widget != first_widget);

    return(result);
}

/*
=============
ui_state_init
=============
*/

void
ui_state_init(ui_state_t       *ui_state, 
              input_manager_t  *input_manager,
              asset_manager_t  *asset_manager, 
              RHI_context_t *RHI_context, 
              u32               renderpass_ID)
{
    ZeroStruct(*ui_state);

    ui_state->ui_events.count = MAX_INPUT_EVENTS;
    ui_state->widget_arena    = c_arena_create(MB(10));
    ui_state->polling_arena   = c_arena_create(MB(50));
    ui_state->persistent_data_arena = c_arena_create(MB(100));
    ui_state->widget_states = c_hash_table_create<widget_state_t>(2096, 
                                                                 &ui_state->persistent_data_arena, 
                                                                  widget_hash_table_allocate_impl,
                                                                  null);
    ui_state->RHI_context           = RHI_context;
    ui_state->asset_manager         = asset_manager;
    ui_state->interface_framebuffer = renderpass_ID;
    ui_state->parent_stack_top      = 1;
    ui_state->hot_widget            = null;
    ui_state->active_widget         = null;

    // NOTE(Sleepster): Theme stuff 
    ui_state->default_font = s_asset_manager_acquire_asset_handle(asset_manager, STR("LiberationMono_Regular"));

    ui_state->default_widget_idle_color       = vec4(1.0, 1.0, 1.0, 1.0);
    ui_state->default_widget_hover_color      = vec4(1.0, 0.0, 0.0, 1.0);
    ui_state->default_widget_active_color     = vec4(0.0, 1.0, 0.0, 1.0);
    ui_state->default_widget_border_color     = vec4(0.0, 0.0, 0.0, 1.0);
    ui_state->default_font_color              = vec4(0.0, 0.0, 0.0, 1.0);

    ui_state->default_font_size               = 32;
    ui_state->default_widget_SDF_smoothness   = 1.0f;
    ui_state->default_widget_border_thickness = 5;
    // NOTE(Sleepster): Theme stuff 

    ui_state->input_manager = input_manager;
    ui_state->widget_shader = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_widget"));

    u32 *indices = c_arena_push_array(&RHI_context->transient_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
    u32  index_offset = 0;
    for(u32 index = 0;
        index < 60000;
        index += 6)
    {
        indices[index + 0] = index_offset + 0;
        indices[index + 1] = index_offset + 1;
        indices[index + 2] = index_offset + 2;
        indices[index + 3] = index_offset + 2;
        indices[index + 4] = index_offset + 3;
        indices[index + 5] = index_offset + 0;

        index_offset += 4;
    }

    ui_state->interface_framebuffer = renderpass_ID;
    const u32 VERTEX_BUFFER_SIZE = 4 * MAX_WIDGETS;
    immediate_vertex_t *vertices = c_arena_push_array(&ui_state->persistent_data_arena, immediate_vertex_t, VERTEX_BUFFER_SIZE);
    ui_state->vertex_buffer = RHI_vertex_buffer_create(RHI_context, 
                                                       RHI_RENDER_BUFFER_ALLOCATION_TYPE_MAPPED,
                                                       RHI_RENDER_BUFFER_ADVANCE_RATE_PER_ELEMENT,
                                                       (byte*)vertices,
                                                       sizeof(immediate_vertex_t),
                                                       VERTEX_BUFFER_SIZE);
    ui_state->index_buffer = RHI_index_buffer_create(RHI_context, 
                                                     RHI_RENDER_BUFFER_ALLOCATION_TYPE_GPU_ONLY,
                                                     sizeof(u32),
                                                     indices,
                                                     sizeof(u32) * (6 * MAX_WIDGETS));
    const u32 INSTANCE_BUFFER_SIZE = MAX_WIDGETS;
    ui_state->widget_instances       = c_arena_push_array(&ui_state->persistent_data_arena, immediate_widget_data_t, INSTANCE_BUFFER_SIZE);
    ui_state->widget_instance_data   = RHI_get_constant_buffer(RHI_context, STR("WidgetInstanceData"));
    ui_state->camera_matrices_buffer = RHI_get_constant_buffer(RHI_context, STR("CameraMatrices"));
}

/*
=============
ui_state_poll_input_events

This function must be called explicitly to update
the current inputs stored inside of the ui_state.
It must be called after ui_state_get_input_events.

If you call this before that function, then you will
have no events to poll...
=============
*/

void
ui_state_poll_input_events(ui_state_t *ui_state)
{
    if(ui_state->frame_begun)
    {
        input_controller_t *controller = ui_state->ui_controller;
        if(controller)
        {
            s_im_apply_events_to_controller(ui_state->ui_controller, ui_state->ui_events, false);
        }
        else
        {
            log_warning("NO valid UI_controller...\n");
        }
    }

    if(ui_state->ui_controller)
    {
        action_button_t *left_mouse  = s_im_get_controller_action_button(ui_state->ui_controller, SDL_LEFT_MOUSE);
        action_button_t *right_mouse = s_im_get_controller_action_button(ui_state->ui_controller, SDL_RIGHT_MOUSE);

        ui_state->left_mouse = {
            .flags = left_mouse->flags,
            .half_transition_count = left_mouse->half_transition_count,
            .ID = SDL_LEFT_MOUSE
        };

        ui_state->right_mouse = {
            .flags = right_mouse->flags,
            .half_transition_count = right_mouse->half_transition_count,
            .ID = SDL_RIGHT_MOUSE
        };

        if(ui_state->first_widget != null)
        {
            vec2_t current_mouse_position = ui_state->mouse_position;

            // NOTE(Sleepster): Determine top most widget for the hierarchy 
            widget_t *current_widget = ui_state->first_widget;
            if(ui_state->hot_widget)
            {
                widget_state_t *state = &(ui_state->widget_states.items[ui_state->hot_widget->ID]).item;
                bool8 within_bounds = rect2_point_in_rect(state->widget_rect, current_mouse_position);
                if(!within_bounds)
                {
                    ui_state->last_hot_ID = ui_state->hot_widget->ID;
                    ui_state->hot_widget = null;
                }
            }

            bool8 is_held           = (ui_state->left_mouse.flags  & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
            bool8 just_released     = (ui_state->left_mouse.flags  & INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
            bool8 just_clicked      = (ui_state->left_mouse.flags  & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
            bool8 is_right_clicked  = (ui_state->right_mouse.flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
            bool8 is_right_held     = (ui_state->right_mouse.flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
            bool8 is_double_clicked = ui_state->left_mouse.half_transition_count >= 2;

            ui_state->left_mouse_clicked_this_frame = just_clicked;
            widget_t *top_most_widget = find_top_level_in_bounds_widget(ui_state,
                                                                        current_widget, 
                                                                        current_mouse_position);
            if(ui_state->hot_widget)
            {
                ui_state->last_hot_ID = ui_state->hot_widget->ID;
                ui_state->hot_widget  = top_most_widget;
            }
            else
            {
                ui_state->hot_widget  = top_most_widget;
            }

            if(just_clicked || is_right_clicked)
            {
                if(ui_state->active_widget)
                {
                    ui_state->last_active_ID = ui_state->active_widget->ID;
                    ui_state->active_widget  = top_most_widget;
                }
                else
                {
                    ui_state->active_widget  = top_most_widget;
                }
            }

            if(ui_state->active_widget)
            {
                widget_state_t *state = &(ui_state->widget_states.items[ui_state->active_widget->ID]).item;

                state->is_held = is_held;
                state->just_released = just_released;
                state->just_clicked = just_clicked;
                state->is_double_clicked = is_double_clicked;
                state->is_right_clicked = is_right_clicked;
                state->is_right_held = is_right_held;
            }
        }
    }
}

/*
=============
ui_state_get_input_events

This function is what actually GATHERS the input
events that you can then poll for. The reason this
API is seperate is because the ui_state itself stores
input events in case it is updated at a different rate
from that of it's ui_controller.
=============
*/

void
ui_state_get_input_events(ui_state_t *ui_state)
{
    if(ui_state->ui_controller && ui_state->input_focused)
    {
        c_arena_reset(&ui_state->polling_arena);

        input_device_t *device = ui_state->ui_controller->device;
        ui_state->ui_event_count = 0;
        for(s32 event_index = 0;
            event_index < device->events.count;
            ++event_index)
        {
            input_event_t *event = device->events + event_index;
            ui_state->ui_events[ui_state->ui_event_count] = *event;

            event->consumed = true;
            if(event->input_stream.count > 0)
            {
                input_event_t *text_event = &ui_state->ui_events[ui_state->ui_event_count];
                text_event->input_stream = c_string_make_copy(&ui_state->polling_arena, text_event->input_stream);
            }

            ++ui_state->ui_event_count;
        }
    }
}

/*
=============
ui_state_update_widget_state
=============
*/

void
ui_state_update_widget_state(ui_state_t *ui_state)
{
    RHI_renderpass_t *renderpass = ui_state->RHI_context->renderpasses + ui_state->interface_framebuffer;
    ui_state->ui_controller = s_im_get_controller_from_active_device(ui_state->input_manager, ui_state->ui_controller);
    ui_state_poll_input_events(ui_state);
    
    float32 half_width  = renderpass->render_width  * 0.5f;
    float32 half_height = renderpass->render_height * 0.5f;

    ui_state->current_camera = {
        .view_matrix       = mat4_identity(),
        .projection_matrix = mat4_RHDX_ortho(-half_width, half_width, -half_height, half_height, 0.0f, 1.0f)
    };

    vec2_t last_mouse = ui_state->mouse_position;
    ui_state->mouse_position = s_im_transform_mouse_data(ui_state->ui_controller, 
                                                         vec2(renderpass->render_width, renderpass->render_height), 
                                                         ui_state->current_camera.view_matrix,
                                                         ui_state->current_camera.projection_matrix);
    ui_state->mouse_delta     = vec2_subtract(ui_state->mouse_position, last_mouse);
    ui_state->max_widget_size = vec2(renderpass->render_width, renderpass->render_height);

    ui_state_update_widget_hierarchy(ui_state);
}

/*
=============
ui_state_update_widget_hierarchy
=============
*/

internal_api void
ui_state_update_widget_hierarchy(ui_state_t *ui_state)
{
    // NOTE(Sleepster): Get the total size of the hierarchy
    size_all_widgets(ui_state);
    // NOTE(Sleepster): Place the widgets in the hierarchy, honoring the sizing and padding
    place_all_widgets(ui_state);
}

/*
=============
render_widget_hierarchy
=============
*/

void
ui_state_render_widgets(ui_state_t *ui_state, RHI_command_list_t *command_list)
{
    RHI_context_t *RHI_context = ui_state->RHI_context;
    asset_manager_t  *asset_manager  = ui_state->asset_manager;
    (void)asset_manager;

    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        render_widget_hierarchy(ui_state, command_list, current_widget);

        RHI_cmd_bind_vertex_buffer(command_list, &ui_state->vertex_buffer);
        RHI_cmd_bind_index_buffer(command_list,  &ui_state->index_buffer);

        // NOTE(Sleepster): First, draw the normal rectangle widgets
        {
            RHI_pipeline_state_t pipeline_state = command_list->active_render_state;
            pipeline_state.dst_color_blend_mode = RBM_OneMinusSrcAlpha;
            pipeline_state.src_alpha_blend_mode = RBM_One;
            pipeline_state.dst_alpha_blend_mode = RBM_Zero;

            RHI_cmd_set_render_state(command_list, &pipeline_state);
            RHI_cmd_use_shader_program(command_list, ui_state->widget_shader);
            RHI_cmd_update_buffer_contents(command_list, &ui_state->vertex_buffer);

            s32 window_width  = Max(RHI_context->window_size.x, 10);
            s32 window_height = Max(RHI_context->window_size.y, 10);

            RHI_cmd_update_constant_buffer(command_list, ui_state->camera_matrices_buffer, &ui_state->current_camera,   sizeof(camera_matrices_t));
            RHI_cmd_update_constant_buffer(command_list, ui_state->widget_instance_data,    ui_state->widget_instances, sizeof(immediate_widget_data_t) * ui_state->widget_instance_count);

            RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

            RHI_cmd_draw_indexed(command_list, ui_state->widget_item_count * 6, 0, 1, 0);
            RHI_buffer_reset(ui_state->RHI_context, &ui_state->vertex_buffer);
            RHI_buffer_reset(ui_state->RHI_context, &ui_state->index_buffer);
            ui_state->widget_instance_count = 0;
        }
    }
    else
    {
        log_warning("Called ui_state_render_widgets on an empty ui_state_t... there are no widgets attached!!!\n");
    }
}

/*
=============
render_widget_hierarchy
=============
*/

internal_api void
render_widget_hierarchy(ui_state_t *ui_state, RHI_command_list_t *command_list, widget_t *first_widget)
{
    widget_t *current_widget = first_widget;
    do {
        vec2_t half_size = vec2(current_widget->state->render_size.x * 0.5f, 
                                current_widget->state->render_size.y * 0.5f);

        if(current_widget->widget_flags & UI_WIDGET_FLAG_DRAW_RECTANGLE)
        {
            current_widget->widget_instance_data         = ui_state->widget_instances + ui_state->widget_instance_count;
            current_widget->widget_instance_data->iFlags = current_widget->widget_flags;

            // NOTE(Sleepster): We're probably setting a ton of redundant data... TOO BAD!! 
            current_widget->widget_instance_data->iBorderColor     = current_widget->border_color;
            current_widget->widget_instance_data->iBorderThickness = current_widget->border_thickness;
            current_widget->widget_instance_data->iHalfSize        = half_size;
            current_widget->widget_instance_data->iRadius          = current_widget->radius;
            current_widget->widget_instance_data->iSDFSmoothness   = current_widget->smoothness;

            immediate_rect(command_list,
                          &ui_state->vertex_buffer,
                           current_widget->state->position, 
                           current_widget->state->render_size,
                           current_widget->state->render_color,
                           vec2_negate(half_size),
                           half_size,
                           vec2(0, ui_state->widget_instance_count),
                           vec2_zero(),
                           vec2_zero());

            ++ui_state->widget_item_count;
            ++ui_state->widget_instance_count;
        }

        if(current_widget->widget_flags & UI_WIDGET_FLAG_DRAW_BACKGROUND)
        {
            immediate_rect(command_list,
                          &ui_state->vertex_buffer,
                           current_widget->state->position, 
                           current_widget->state->render_size,
                           current_widget->state->render_color,
                           current_widget->state->position.xy,
                           vec2_add(current_widget->state->position.xy, current_widget->state->render_size),
                           vec2(0, 0),
                           vec2_zero(),
                           vec2_zero());

            ++ui_state->widget_item_count;
        }

        if(current_widget->widget_flags & UI_WIDGET_FLAG_DRAW_BACKGROUND)
        {
            InvalidCodePath;
        }

        if(current_widget->widget_flags & UI_WIDGET_FLAG_DRAW_TEXT)
        {
            vec3_t text_render_position = vec3(current_widget->state->position.x + current_widget->widget_padding.left,
                                               current_widget->state->position.y + current_widget->widget_padding.bottom,
                                               current_widget->state->position.z + -0.01); // some Epsilon
            if((current_widget->widget_flags & UI_WIDGET_FLAG_HAS_TEXT_CONTENT) == 0)
            {
                immediate_text(command_list, 
                              &ui_state->vertex_buffer, 
                               ui_state->asset_manager,
                              &ui_state->default_font,
                               current_widget->widget_name,
                               text_render_position, 
                               ui_state->default_font_color,
                               2.0,
                               current_widget->font_size);

                ui_state->widget_item_count += current_widget->widget_name.count;
            }
            else
            {
                string_t widget_text = {
                    .data  = (u8*)(current_widget->widget_text_buffer->data + current_widget->state->widget_text_render_start_offset),
                    .count = current_widget->state->widget_text_render_end_offset,
                };
                immediate_text(command_list, 
                              &ui_state->vertex_buffer, 
                               ui_state->asset_manager,
                              &ui_state->default_font,
                               widget_text,
                               text_render_position, 
                               ui_state->default_font_color,
                               2.0,
                               current_widget->font_size);

                ui_state->widget_item_count += widget_text.count;
            }
        }

        if(current_widget->widget_flags & UI_WIDGET_FLAG_DISPLAY_TEXTURE)
        {
            if(current_widget->display_texture->texture)
            {
                RHI_cmd_bind_texture_from_handle(command_list, current_widget->display_texture);

                s32 texture_index = RHI_is_texture_bound(command_list, current_widget->display_texture->texture);
                immediate_quad_ex(command_list,
                                  &ui_state->vertex_buffer,
                                  current_widget->state->position, 
                                  current_widget->state->render_size,
                                  current_widget->state->render_color,
                                  current_widget->display_texture_uvmin,
                                  current_widget->display_texture_uvmax,
                                  vec2(1.0, texture_index),
                                  vec2(current_widget->display_texture->texture->gpu_data.create_info.sampler_info.filtering, 0.0),
                                  vec2_zero(),
                                  current_widget->display_texture->texture);
                ++ui_state->widget_item_count;
            }
        }

        if(current_widget->first_child)
        {
            render_widget_hierarchy(ui_state, command_list, current_widget->first_child);
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != first_widget);
}

/*
=============
ui_state_begin_frame
=============
*/

void
ui_state_begin_frame(ui_state_t *ui_state)
{
    Expect(ui_state->frame_begun == false, "Attempted to call 'ui_state_begin_frame()' on this ui_state_t, however the frame has already begun for this ui_state_t...\n");

    ui_state->widget_item_count      = 0;
    ui_state->ui_seed                = 0;
    ui_state->parent_stack_top       = 1;
    ui_state->default_font_size      = 32;
    ui_state->active_widget_padding  = vec4_zero();
    ui_state->left_mouse_clicked_this_frame = false;
    ui_state->frame_begun = true;
    ui_state->frame_ended = false;

    c_arena_reset(&ui_state->widget_arena);
}

/*
=============
ui_state_end_frame
=============
*/

true_inline void
ui_state_end_frame(ui_state_t *ui_state, RHI_command_list_t *command_list)
{
    Expect(ui_state->frame_begun == true, "Attempted to call 'ui_state_end_frame()' but 'ui_state_begin_frame()' was never called on this ui_state_t...\n");
    Expect(ui_state->frame_ended == false, "Attempted to call 'ui_state_end_frame()' but this has already been called on this ui_state_t this frame...\n");

    ui_state_update_widget_state(ui_state);
    ui_state_render_widgets(ui_state, command_list);
    ui_state->frame_ended = true;
    ui_state->frame_begun = false;
    s_im_clear_controller_transient_state(ui_state->ui_controller);
    if(ui_state->input_focused)
    {
        s_im_clear_device_events(ui_state->ui_controller->device);
    }

    ++ui_state->frame_count;
}

/*
================================
UI_STATE WIDGET PROPERTIES
================================
*/

true_inline void
ui_state_begin_row(ui_state_t *ui_state, widget_t *parent)
{
    ui_widget_set_layout(parent, UI_WIDGET_LAYOUT_STYLE_HORIZONTAL);
    ui_widget_push_parent(ui_state, parent);
}

true_inline void
ui_state_end_row(ui_state_t *ui_state)
{
    ui_widget_pop_parent(ui_state);
}

true_inline void
ui_state_begin_column(ui_state_t *ui_state, widget_t *parent)
{
    ui_widget_set_layout(parent, UI_WIDGET_LAYOUT_STYLE_VERTICAL);
    ui_widget_push_parent(ui_state, parent);
}

true_inline void
ui_state_end_column(ui_state_t *ui_state)
{
    ui_widget_pop_parent(ui_state);
}

true_inline void
ui_state_set_active_padding(ui_state_t *ui_state, vec4_t padding)
{
    ui_state->active_widget_padding = padding;
}

/*
================================
UI_STATE WIDGET PROPERTIES
================================
*/

internal_api void
place_widgets_in_hierarchy(widget_t *first_widget, vec2_t *placement_cursor, u32 layout_style)
{
    widget_t *current_widget = first_widget;
    do {
        current_widget->expected_position.xy = *placement_cursor;
        current_widget->expected_position.z  =  current_widget->parent_stack_depth;

        current_widget->state->position = vec3(current_widget->expected_position.x + current_widget->state->offset.x,
                                               current_widget->expected_position.y + current_widget->state->offset.y - current_widget->state->render_size.y,
                                               current_widget->parent_stack_depth);

        current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy,
                                                          current_widget->state->render_size);
        if(current_widget->first_child)
        {
            widget_t *nested_first_child = current_widget->first_child;
            vec2_t child_cursor = vec2(current_widget->state->position.x + nested_first_child->parent_padding.x,
                                       current_widget->state->position.y + current_widget->state->render_size.y - nested_first_child->parent_padding.z);

            place_widgets_in_hierarchy(current_widget->first_child,
                                      &child_cursor,
                                       current_widget->layout_style);
        }

        bool8 has_next_sibling = (current_widget->next_sibling != first_widget);
        if(layout_style == UI_WIDGET_LAYOUT_STYLE_VERTICAL)
        {
            placement_cursor->y -= current_widget->state->render_size.y;
            if(has_next_sibling) 
            {
                placement_cursor->y -= current_widget->parent_child_spacing.y;
            }
        }
        else
        {
            placement_cursor->x += current_widget->state->render_size.x;
            if(has_next_sibling) 
            {
                placement_cursor->x += current_widget->parent_child_spacing.x;
            }
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != first_widget);
}


internal_api void
place_all_widgets(ui_state_t *ui_state)
{
    RHI_renderpass_t *renderpass = ui_state->RHI_context->renderpasses + ui_state->interface_framebuffer;
    float32 half_width  = renderpass->render_width  * 0.5f;
    float32 half_height = renderpass->render_height * 0.5f;

    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        do {
            current_widget->state->position.xy = vec2(current_widget->expected_position.x + current_widget->state->offset.x,
                                                      current_widget->expected_position.y + current_widget->state->offset.y - current_widget->state->render_size.y);
            current_widget->state->position.z  = current_widget->parent_stack_depth;

            // NOTE(Sleepster): Clamp it so that it fits within the window. 
            current_widget->state->position.x = Clamp(current_widget->state->position.x, (-half_width  + 1), (half_width  - current_widget->state->render_size.x) - 1);
            current_widget->state->position.y = Clamp(current_widget->state->position.y, (-half_height + 1), (half_height - current_widget->state->render_size.y) - 1);

            current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy,
                                                              current_widget->state->render_size);

            if(current_widget->first_child)
            {
                vec2_t placement_cursor = vec2(current_widget->state->position.x + current_widget->max_widget_padding.left,
                                               (current_widget->state->position.y + current_widget->state->render_size.y) - current_widget->max_widget_padding.top);

                place_widgets_in_hierarchy(current_widget->first_child,
                                           &placement_cursor,
                                           current_widget->layout_style);
            }

            current_widget = current_widget->next_sibling;
        }while(current_widget != ui_state->first_widget);
    }
}

internal_api vec2_t
determine_hierarchy_size(ui_state_t *ui_state, widget_t *widget)
{
    vec2_t result = {};
    if((widget->widget_flags & UI_WIDGET_FLAG_FIXED_SIZE) == 0)
    {
        // NOTE(Sleepster): Dynamically sized widget
        vec2_t resize_factor = {1.0f, -1.0f};

        result = widget->minimum_render_size;
        // NOTE(Sleepster): Measure Children First
        if(widget->first_child)
        {
            vec2_t total_children_sizing = vec2_zero();
            widget_t *current_widget = widget->first_child;
            do {
                vec2_t current_subhierarchy_size = determine_hierarchy_size(ui_state, current_widget);
                for(u32 axis = 0;
                    axis < ArrayCount(widget->size_kind.elements);
                    ++axis)
                {
                    widget_size_kind_t size_kind = (widget_size_kind_t)current_widget->size_kind.elements[axis];
                    if(size_kind != UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT)
                    {
                        bool8 has_next_sibling = (current_widget->next_sibling != widget->first_child);
                        if(widget->layout_style == UI_WIDGET_LAYOUT_STYLE_HORIZONTAL)
                        {
                            if(axis == 0)
                            {
                                total_children_sizing.elements[axis] += current_subhierarchy_size.elements[axis];
                            }
                            else if(axis == 1)
                            {
                                total_children_sizing.elements[axis] = Max(total_children_sizing.elements[axis], 
                                                                           current_subhierarchy_size.elements[axis]);
                            }

                            if(has_next_sibling) total_children_sizing.elements[axis] += widget->child_spacing.elements[axis];
                        }
                        else
                        {
                            if(axis == 0)
                            {
                                total_children_sizing.elements[axis] = Max(total_children_sizing.elements[axis], 
                                                                           current_subhierarchy_size.elements[axis]);
                            }
                            else if(axis == 1)
                            {
                                total_children_sizing.elements[axis] += current_subhierarchy_size.elements[axis];
                            }

                            if(has_next_sibling) total_children_sizing.elements[axis] += widget->child_spacing.elements[axis];
                        }
                    }
                }

                current_widget = current_widget->next_sibling;
            }while(current_widget != widget->first_child);

            result.x = Max(result.x, total_children_sizing.x);
            result.y = Max(result.y, total_children_sizing.y);
        } 

        // NOTE(Sleepster): Compute the size for each axis
        for(u32 axis = 0;
            axis < ArrayCount(widget->size_kind.elements);
            ++axis)
        {
            float32 padding = widget->max_widget_padding.elements[axis * 2] + widget->max_widget_padding.elements[axis * 2 + 1];

            widget_size_kind_t size_kind = (widget_size_kind_t)widget->size_kind.elements[axis];
            if(size_kind == UI_WIDGET_SIZE_KIND_PIXELS)
            {
                widget->state->render_size.elements[axis] = (result.elements[axis] + padding);

                // NOTE(Sleepster): Set what the size of the widget is before any operations that may change this number 
                widget->state->absolute_minimum_render_size.elements[axis] = widget->state->render_size.elements[axis];
                if(widget->widget_flags & UI_WIDGET_FLAG_RESIZEABLE)
                {
                    float32 *resize_amount = &widget->state->resize_amount.elements[axis];
                    float32 *current_size  = &widget->state->render_size.elements[axis];

                    *resize_amount = widget->resize_value.elements[axis] + *resize_amount;
                    *current_size  = widget->state->render_size.elements[axis] + (*resize_amount * resize_factor.elements[axis]);

                    // NOTE(Sleepster): Clamp the value 
                    if(*current_size < widget->state->absolute_minimum_render_size.elements[axis])
                    {
                        float32 difference = widget->state->absolute_minimum_render_size.elements[axis] - *current_size;
                        widget->state->resize_amount.elements[axis] += (difference * resize_factor.elements[axis]);
                    }

                    *current_size = Max(*current_size, widget->state->absolute_minimum_render_size.elements[axis]);

                    if(*current_size > ui_state->max_widget_size.elements[axis])
                    {
                        float32 difference = *current_size - ui_state->max_widget_size.elements[axis];
                        *current_size = ui_state->max_widget_size.elements[axis];

                        *current_size += (difference * resize_factor.elements[axis]);
                    }
                }
            }
            else if(size_kind == UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT)
            {
                Expect(widget->parent, "Widget size is set too 'UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT', but this widget does not HAVE a parent...\n");
                widget->minimum_render_size.elements[axis] = Clamp(widget->minimum_render_size.elements[axis], 0.0, 1.0);

                float32 render_size = widget->parent->state->render_size.elements[axis] - (padding);
                widget->state->render_size.elements[axis] = render_size * widget->minimum_render_size.elements[axis];
            }
            else
            {
                InvalidCodePath;
            }
        }
    }
    else
    {
        // NOTE(Sleepster): If this is a statically sized widget 
        widget->state->render_size = vec2(widget->minimum_render_size.x + widget->max_widget_padding.left + widget->max_widget_padding.right,
                                          widget->minimum_render_size.y + widget->max_widget_padding.top  + widget->max_widget_padding.bottom);

        widget->state->absolute_minimum_render_size = widget->state->render_size;

        // NOTE(Sleepster): Even if our size doesn't change, we must still do the sizing of our children 
        if(widget->first_child)
        {
            widget_t *current_widget = widget->first_child;
            do {
                determine_hierarchy_size(ui_state, current_widget);
                current_widget = current_widget->next_sibling;
            }while(current_widget != widget->first_child);
        }
    }

    result = widget->state->render_size;

    return(result);
}

internal_api void
size_all_widgets(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        do {
            determine_hierarchy_size(ui_state, current_widget);
            current_widget = current_widget->next_sibling;
        }while(current_widget != ui_state->first_widget);
    }
}

internal_api void*
widget_hash_table_allocate_impl(void *allocator, u32 allocation_size)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

/*
================================
UI_STATE WIDGET PROPERTIES
================================
*/

true_inline void
ui_widget_set_flags(widget_t *widget, u32 flags)
{
    widget->widget_flags |= flags;
}

true_inline void
ui_widget_set_default_font_color(ui_state_t *ui_state, vec4_t color)
{
    ui_state->default_font_color = color;
}

true_inline void
ui_widget_set_default_widget_idle_color(ui_state_t *ui_state, vec4_t color)
{
    ui_state->default_widget_idle_color = color;
}

true_inline void
ui_widget_set_default_widget_hover_color(ui_state_t *ui_state, vec4_t color)
{
    ui_state->default_widget_hover_color = color;
}

true_inline void
ui_widget_set_default_widget_active_color(ui_state_t *ui_state, vec4_t color)
{
    ui_state->default_widget_active_color = color;
}

true_inline void
ui_widget_set_default_font_size(ui_state_t *ui_state, u32 font_size)
{
    ui_state->default_font_size = font_size;
}

true_inline void
ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget)
{
    Assert(ui_state->parent_stack_top + 1 <= MAX_PARENT_WIDGETS);
    ui_state->parent_stack[ui_state->parent_stack_top++] = widget;
}

true_inline void
ui_widget_pop_parent(ui_state_t *ui_state)
{
    Assert(ui_state->parent_stack_top > 1);
    --ui_state->parent_stack_top;
}

true_inline widget_t*
ui_widget_get_top_parent(ui_state_t *ui_state)
{
    widget_t *result = null;

    if(ui_state->parent_stack_top > 1)
    {
        result = ui_state->parent_stack[ui_state->parent_stack_top - 1];
    }

    return(result);
}

true_inline void
ui_widget_set_layout(widget_t *widget, u32 layout_style)
{
    widget->layout_style = layout_style;
}

true_inline void
ui_state_set_parent_layout(ui_state_t *ui_state, u32 layout_style)
{
    widget_t *parent = ui_widget_get_top_parent(ui_state);
    parent->layout_style = layout_style;
}

true_inline void
ui_widget_set_padding(widget_t *widget, vec4_t padding) 
{
    widget->widget_padding = padding;

    bool8 padding_changed = false;
    if(padding.left > widget->max_widget_padding.left)
    {
        widget->max_widget_padding.left = padding.left;
        padding_changed = true;
    }

    if(padding.right > widget->max_widget_padding.right)
    {
        widget->max_widget_padding.right = padding.right;
        padding_changed = true;
    }

    if(padding.top > widget->max_widget_padding.top)
    {
        widget->max_widget_padding.top = padding.top;
        padding_changed = true;
    }

    if(padding.bottom > widget->max_widget_padding.bottom)
    {
        widget->max_widget_padding.bottom = padding.bottom;
        padding_changed = true;
    }

    if(padding_changed)
    {
        vec2_t new_size   = vec2(widget->max_widget_padding.left + widget->max_widget_padding.right, widget->max_widget_padding.top + widget->max_widget_padding.bottom);
        vec2_t size_delta = vec2_subtract(new_size, widget->minimum_render_size);

        widget->minimum_render_size += size_delta;
    }
}

true_inline void
ui_widget_seed(ui_state_t *ui_state, u64 index)
{
    ui_state->ui_seed = index;
}

true_inline u64
ui_widget_hash(ui_state_t *ui_state, widget_t *widget)
{
    u64 result = 0;

    result = (c_hash_table_hash_key(widget->widget_name));
    if(ui_state->ui_seed != 0)
    {
        result = c_hash_table_combine_hashes(result, ui_state->ui_seed);
    }

    result %= ui_state->widget_states.max_entries;
    return(result);
}


internal_api true_inline float32
ui_widget_determine_depth(ui_state_t *ui_state)
{
    float32 result = 1.0f - 2.0f * ((float32)ui_state->parent_stack_top / (float32)(MAX_WIDGET_LAYERS - 1));
    return(result);
}

void
ui_widget_append(widget_t **first_node, widget_t **last_node, widget_t *widget)
{
    if(*first_node)
    {
        widget_t *old_last = *last_node;

        widget->prev_sibling   =  old_last;
        widget->next_sibling   = *first_node;
        old_last->next_sibling =  widget;

         *last_node = widget;
        (*first_node)->prev_sibling = widget;
    }
    else
    {
        *first_node = widget;
        *last_node  = widget;

        (*first_node)->next_sibling = widget;
        (*last_node)->prev_sibling  = widget;
    }
}

widget_t*
ui_widget_create(ui_state_t *ui_state, string_t widget_name, u32 widget_flags)
{
    widget_t *result = c_arena_push_struct(&ui_state->widget_arena, widget_t);
    ZeroStruct(*result);
    if(widget_flags & UI_WIDGET_FLAG_MOUSE_CLICKABLE || 
       widget_flags & UI_WIDGET_FLAG_HOVERABLE       ||
       widget_flags & UI_WIDGET_FLAG_LEFT_DRAGGABLE)
    {
        if((widget_flags & UI_WIDGET_FLAG_INTERACTABLE) == 0)
        {
            widget_flags |= UI_WIDGET_FLAG_INTERACTABLE;
        }
    }


    result->widget_name         = widget_name;
    result->widget_flags        = widget_flags;
    result->ID                  = ui_widget_hash(ui_state, result);
    result->parent_stack_depth  = ui_widget_determine_depth(ui_state);
    result->state               = &ui_state->widget_states.items[result->ID].item;
    result->widget_padding      = ui_state->active_widget_padding;

    result->smoothness          = ui_state->default_widget_SDF_smoothness;
    result->border_thickness    = ui_state->default_widget_border_thickness;
    result->border_color        = ui_state->default_widget_border_color;
    result->font_size           = ui_state->default_font_size;
    result->radius              = result->state->render_size.x;
    result->expected_position.z = result->parent_stack_depth;

    // NOTE(Sleepster): NEXT AND PREVIOUS ARE BROKEN... 
    widget_t *parent = ui_widget_get_top_parent(ui_state);
    if(parent != null)
    {
        ui_widget_append(&parent->first_child, &parent->last_child, result);
        result->layout_style         = parent->layout_style;
        result->parent_child_spacing = parent->child_spacing;
        result->parent_padding       = parent->widget_padding;
        result->parent               = parent;
    }
    else
    {
        ui_widget_append(&ui_state->first_widget, &ui_state->last_widget, result);
    }

    return(result);
}

internal_api void
widget_do_button(ui_state_t *ui_state, ui_signal_t *signal)
{
    widget_t *widget = signal->widget;
    if(signal->signal_flags & UI_SIGNAL_FLAG_HOVERING)
    {
        widget->state->last_interacted_frame = ui_state->frame_count;
        widget->state->render_color = widget->hovered_color;

        if(signal->signal_flags & UI_SIGNAL_FLAG_CLICKED)
        {
            widget->state->input_begin_within_bounds = true;
        }

        if(widget->state->input_begin_within_bounds && 
          (signal->signal_flags & UI_SIGNAL_FLAG_LEFT_DOWN))
        {
            widget->state->render_color = widget->active_color;
        }
    }

    if((signal->signal_flags & UI_SIGNAL_FLAG_LEFT_DOWN) == 0)
    {
        widget->state->input_begin_within_bounds = false;
    }
}

internal_api void
widget_do_toggle_button(ui_state_t *ui_state, ui_signal_t *signal)
{
    widget_do_button(ui_state, signal);
    if(signal->signal_flags & UI_SIGNAL_FLAG_CLICKED)
    {
        signal->widget->state->toggled = !signal->widget->state->toggled;
    }
    signal->widget->toggled = signal->widget->state->toggled;

    if(signal->widget->toggled)
    {
        signal->widget->state->render_color = signal->widget->active_color;
    }
}

internal_api void
widget_do_draggable(ui_state_t *ui_state, ui_signal_t *signal)
{
    widget_t *widget = signal->widget;
    if(signal->signal_flags & UI_SIGNAL_FLAG_HOVERING)
    {
        widget->state->last_interacted_frame = ui_state->frame_count;
        if(signal->signal_flags & UI_SIGNAL_FLAG_CLICKED)
        {
            widget->state->initial_mouse_position    = ui_state->mouse_position;
            widget->state->input_begin_within_bounds = true;
        }
    }

    if(widget->state->input_begin_within_bounds)
    {
        if(signal->signal_flags & UI_SIGNAL_FLAG_LEFT_DOWN)
        {
            vec2_t mouse_delta     = vec2_subtract(ui_state->mouse_position, widget->state->initial_mouse_position);
            widget->state->offset  = vec2_add(widget->state->offset, mouse_delta);

            widget->state->initial_mouse_position = ui_state->mouse_position;
        }
        else if(signal->signal_flags & UI_SIGNAL_FLAG_RELEASED)
        {
            widget->state->input_begin_within_bounds = false;
        }
    }
}

ui_signal_t
ui_widget_get_signals(ui_state_t *ui_state, widget_t *widget)
{
    ui_signal_t result = {};
    result.widget = widget;
    if(ui_state->hot_widget == widget && (ui_state->hot_widget->widget_flags & UI_WIDGET_FLAG_INTERACTABLE))
    {
        widget_state_t *widget_state = &ui_state->widget_states.items[widget->ID].item;

        if((widget->widget_flags & UI_WIDGET_FLAG_HOVERABLE))
        {
            result.signal_flags |= UI_SIGNAL_FLAG_HOVERING;
        }

        if((widget->widget_flags & UI_WIDGET_FLAG_MOUSE_CLICKABLE))
        {
            if(widget_state->just_clicked)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_CLICKED; 
            }

            if(widget_state->is_held)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DOWN;
            }

            if(widget_state->just_released)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_RELEASED;
            }

            if(widget_state->is_double_clicked)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DOUBLE_CLICKED;
            }
        }
        else if((widget->widget_flags & UI_WIDGET_FLAG_LEFT_DRAGGABLE))
        {
            if(widget_state->just_clicked)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_CLICKED;
            }
            if(widget_state->is_held)
            {
                result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DOWN;
            }
        }

        if(widget_state->is_right_clicked)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_RIGHT_CLICKED;
        }
        
        if(widget_state->is_right_held)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_RIGHT_DOWN;
        }
    }

    return(result);
}

ui_signal_t
ui_widget_panel(ui_state_t *ui_state, 
                string_t    widget_name, 
                vec2_t      position, 
                vec2_t      minimum_render_size,
                vec2_t      child_spacing, 
                vec4_t      padding, 
                vec4_t      background_color,
                u32         additional_flags)
{
    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_IDLE_COLOR|UI_WIDGET_FLAG_DRAW_RECTANGLE|UI_WIDGET_FLAG_DRAW_BORDER);
    if(additional_flags != 0)
    {
        widget->widget_flags |= additional_flags;
    }

    widget->expected_position    = vec2_expand_vec3(position, widget->parent_stack_depth);
    widget->state->render_color  = background_color;
    widget->toggled              = widget->state->toggled;
    widget->child_spacing        = child_spacing;
    widget->widget_padding       = padding;
    widget->radius               = 0.0f;
    widget->max_widget_padding   = padding;
    widget->minimum_render_size  = vec2_add(minimum_render_size, 
                                            vec2(padding.left + padding.right, padding.top + padding.bottom));

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    if(additional_flags & UI_WIDGET_FLAG_RESIZEABLE)
    {
        if(ui_right_mouse_down(result))
        {
            float32 resize_x = ui_state->mouse_delta.x;
            float32 resize_y = ui_state->mouse_delta.y;

            widget->resize_value = vec2(resize_x, resize_y);
        }
    }

    return(result);
}

true_inline ui_signal_t
ui_widget_draggable_panel(ui_state_t *ui_state,
                          string_t    widget_name,
                          vec2_t      position,
                          vec2_t      minimum_render_size,
                          vec2_t      child_spacing,
                          vec4_t      padding,
                          vec4_t      background_color,
                          u32         additional_flags)
{
    u32 flags = (additional_flags | (UI_WIDGET_FLAG_LEFT_DRAGGABLE|UI_WIDGET_FLAG_HOVERABLE|UI_WIDGET_FLAG_INTERACTABLE));
    ui_signal_t result = ui_widget_panel(ui_state, widget_name, position, minimum_render_size, child_spacing, padding, background_color, flags);

    result = ui_widget_get_signals(ui_state, result.widget);
    widget_do_draggable(ui_state, &result);

    return(result);
}

ui_signal_t
ui_widget_sized_button(ui_state_t *ui_state, 
                       string_t    widget_name, 
                       vec2_t      minimum_size,
                       u32         widget_flags)
{
    widget_t *widget = ui_widget_create(ui_state, 
                                        widget_name, 
                                        UI_WIDGET_FLAG_STANDARD_RECTANGLE_BUTTON|UI_WIDGET_FLAG_MAKE_CIRCULAR);
    widget->minimum_render_size = minimum_size;
    widget->idle_color          = ui_state->default_widget_idle_color;
    widget->hovered_color       = ui_state->default_widget_hover_color;
    widget->active_color        = ui_state->default_widget_active_color;
    widget->state->render_color = widget->idle_color;
    widget->widget_flags       |= widget_flags;

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    widget_do_button(ui_state, &result);

    return(result);
}

ui_signal_t
ui_widget_text(ui_state_t *ui_state, string_t widget_text)
{
    widget_t *widget  = ui_widget_create(ui_state, widget_text, UI_WIDGET_FLAG_DRAW_TEXT|UI_WIDGET_FLAG_IDLE_COLOR);
    widget->font_size = ui_state->default_font_size;

    float32 max_descender = 0.0f;
    widget->minimum_render_size = s_asset_font_get_string_size(ui_state->asset_manager, 
                                                               widget_text, 
                                                              &ui_state->default_font, 
                                                               ui_state->default_font_size,
                                                               &max_descender);
    widget->state->offset.y = max_descender;

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    return(result);
}

ui_signal_t
ui_widget_labeled_button(ui_state_t *ui_state, string_t widget_text)
{
    float32 max_descender = 0.0f;
    vec2_t text_size = s_asset_font_get_string_size(ui_state->asset_manager, 
                                                    widget_text, 
                                                   &ui_state->default_font, 
                                                    ui_state->default_font_size,
                                                    &max_descender);
    
    vec2_t true_size = vec2(text_size.x + ui_state->active_widget_padding.left + ui_state->active_widget_padding.right,
                            text_size.y + ui_state->active_widget_padding.top  + ui_state->active_widget_padding.bottom);
    ui_signal_t result = ui_widget_sized_button(ui_state, 
                                                widget_text,
                                                true_size,
                                                UI_WIDGET_FLAG_DRAW_TEXT|UI_WIDGET_FLAG_MAKE_CIRCULAR|UI_WIDGET_FLAG_INTERACTABLE);
    result.widget->font_size = ui_state->default_font_size;

    return(result);
}

ui_signal_t
ui_widget_toggle_box(ui_state_t *ui_state, string_t widget_text, vec2_t size)
{
    widget_t *widget = ui_widget_create(ui_state, 
                                        widget_text, 
                                        UI_WIDGET_FLAG_STANDARD_RECTANGLE_BUTTON|UI_WIDGET_FLAG_MAKE_CIRCULAR|UI_WIDGET_FLAG_INTERACTABLE);
    widget->minimum_render_size = size;
    widget->idle_color          = ui_state->default_widget_idle_color;
    widget->hovered_color       = ui_state->default_widget_hover_color;
    widget->active_color        = ui_state->default_widget_active_color;
    widget->state->render_color = widget->idle_color;

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    widget_do_toggle_button(ui_state, &result);

    return(result);
}

// TODO(Sleepster): color? 
ui_signal_t
ui_widget_rectangle(ui_state_t *ui_state, string_t widget_name, vec2_t size, ivec2_t size_kind)
{
    ui_signal_t result = {};
    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_IDLE_COLOR|UI_WIDGET_FLAG_DRAW_RECTANGLE|UI_WIDGET_FLAG_MAKE_CIRCULAR);

    result = ui_widget_get_signals(ui_state, widget);

    widget->minimum_render_size = size;
    widget->size_kind           = size_kind;
    widget->state->render_color = ui_state->default_widget_idle_color;

    return(result);
}

void 
ui_widget_spacer(ui_state_t *ui_state, string_t widget_name, vec2_t spacing_size, ivec2_t size_kind)
{
    widget_t *widget = ui_widget_create(ui_state, widget_name, 0);
    widget->minimum_render_size = spacing_size;
    widget->size_kind           = size_kind;
    widget->state->render_color = ui_state->default_widget_idle_color;
}

void
ui_widget_divider(ui_state_t *ui_state, string_t widget_name, vec2_t size, ivec2_t size_kind)
{
    ui_widget_rectangle(ui_state, widget_name, size, size_kind);
}

ui_signal_t
ui_widget_float_slider_bar(ui_state_t *ui_state, string_t widget_name, u32 bar_width, u32 bar_height, float32 button_scale_factor)
{
    ui_signal_t result;

    vec4_t active_padding = ui_state->active_widget_padding;
    ui_state_set_active_padding(ui_state, vec4_zero());

    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_DRAW_RECTANGLE|UI_WIDGET_FLAG_MAKE_CIRCULAR|UI_WIDGET_FLAG_FIXED_SIZE);
    widget->minimum_render_size = vec2(bar_width, bar_height);
    widget->state->render_color = ui_state->default_widget_idle_color;

    ui_signal_t slider_state = ui_widget_get_signals(ui_state, widget);
    ui_row(ui_state, widget)
    {
        ui_widget_seed(ui_state, widget->ID);
        vec2_t slider_box_size = vec2((float32)bar_width * 0.1f, (float32)bar_height * button_scale_factor);

        string_t box_name = c_string_concat(&gc->transient_arena, STR("SLIDER_BOX_"), widget_name);
        ui_signal_t slider_button = ui_widget_sized_button(ui_state, 
                                                           box_name, 
                                                           slider_box_size, 
                                                           UI_WIDGET_FLAG_LEFT_DRAGGABLE|UI_WIDGET_FLAG_MAKE_CIRCULAR);
        slider_button.widget->state->render_color = vec4(0.0, 0.0, 0.0, 1.0);

        widget_t *box_button = slider_button.widget;
        widget_t *slider_bar = slider_state.widget;

        widget_do_draggable(ui_state, &slider_button);
        if(box_button->state->input_begin_within_bounds || (ui_state->last_clicked_widget == box_button))
        {
            float32 move_x    = (ui_state->mouse_delta.x * 1.1) / (float32)bar_width;
            float32 new_value = slider_bar->state->slider_value + move_x;

            slider_bar->state->slider_value = Clamp(new_value, 0.0f, 1.0f);
        }

        float32 remaining_width = (float32)bar_width - box_button->minimum_render_size.x;
        box_button->state->offset.x = (slider_bar->state->slider_value * remaining_width);
        box_button->state->offset.y = (slider_box_size.y - ((float32)bar_height * (button_scale_factor * 1.3f))) * -1.0f;
    }

    ui_widget_seed(ui_state, 0);
    ui_state_set_active_padding(ui_state, active_padding);
    return(result);
}

// TODO(Sleepster): Function to fill the contents of the text buffer
ui_signal_t
ui_widget_textbox(ui_state_t *ui_state, string_t widget_name, string_t *widget_text_content, vec2_t size)
{
    ui_signal_t result = {};
    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_DRAW_RECTANGLE|
                                                               UI_WIDGET_FLAG_MOUSE_CLICKABLE|
                                                               UI_WIDGET_FLAG_INTERACTABLE|
                                                               UI_WIDGET_FLAG_DRAW_TEXT|
                                                               UI_WIDGET_FLAG_HAS_TEXT_CONTENT);
    widget->minimum_render_size = size;
    widget->state->render_color = ui_state->default_widget_idle_color;
    widget->widget_text_buffer  = widget_text_content;

    result = ui_widget_get_signals(ui_state, widget);

    widget_state_t *widget_state = &ui_state->widget_states.items[widget->ID].item;
    if(ui_pressed(result))
    {
        widget_state->toggled = true;
    }
    else if(ui_state->left_mouse_clicked_this_frame)
    {
        widget_state->toggled = false;
    }

    if(widget_state->toggled && ui_state->input_focused)
    {
        for(u32 event_index = 0;
            event_index < ui_state->ui_event_count;
            ++event_index)
        {
            input_event_t *event = ui_state->ui_events + event_index;
            if(event->input_stream.count > 0)
            {
                if(!event->consumed)
                {
                    event->consumed = true;
                    for(s32 stream_index = 0;
                        stream_index < event->input_stream.count;
                        ++stream_index)
                    {
                        s32 index = widget->widget_text_buffer->count + stream_index;
                        widget->widget_text_buffer->data[index] = event->input_stream.data[stream_index];
                    }

                    widget->widget_text_buffer->count += event->input_stream.count;
                }
            }
            else
            {
            }
        }
#if 0
        // NOTE(Sleepster): Handle text input
        for(s32 event_index = 0;
            event_index < ui_state->ui_controller->event_count;
            ++event_index)
        {
            // NOTE(Sleepster): Eventually render a range of characters 
            // (such as indices 10 - 32 for items that run off the end, a way of fitting text into a small box like other apps)
            // rather than the whole string 
            input_event_t *input = ui_state->ui_controller->events + event_index;

            // NOTE(Sleepster): Input stream, like WM_CHAR events 
            if()
            {
                // NOTE(Sleepster): Safe to use here since SDL GUARANTEES that this will be null terminated 
                s32 input_length = strlen((const char *)input->input_stream);
                if(input_length > 0)
                {
                    for(s32 stream_index = 0;
                        stream_index < input_length;
                        ++stream_index)
                    {
                        s32 index = widget->widget_text_buffer->count + stream_index;
                        widget->widget_text_buffer->data[index] = input->input_stream[stream_index];
                    }

                    widget->widget_text_buffer->count += input_length;
                }
            }
            else
            {
                // NOTE(Sleepster): Special non-character items like backspace, delete, etc. 
                if(input->keycode == 0x08)
                {
                    if((input->modifier_flags & TEXT_INPUT_MODIFIER_CTRL) == 0)
                    {
                        widget->widget_text_buffer->data[widget->widget_text_buffer->count] = '\0';
                        widget->widget_text_buffer->count = Max(widget->widget_text_buffer->count - 1, 0);
                    }
                    else
                    {
                        for(s32 index = -4;
                            index != 0;
                            ++index)
                        {
                            s32 char_index = Max(widget->widget_text_buffer->count + index, 0);
                            widget->widget_text_buffer->data[char_index] = '\0';
                        }
                        widget->widget_text_buffer->count = Max(widget->widget_text_buffer->count - 4, 0);
                    }
                }
            }
            input->consumed = true;
        }
#endif

        widget_state->widget_text_render_start_offset = 0;
        widget_state->widget_text_render_end_offset   = widget->widget_text_buffer->count;

        vec2_t offset = {};
        if(widget->widget_text_buffer->count > 0)
        {
            vec2_t string_size = s_asset_font_get_string_size(ui_state->asset_manager, 
                                                             *widget->widget_text_buffer,
                                                              &ui_state->default_font,
                                                              widget->font_size,
                                                              null);
            offset = string_size;
        }
        
        float32 internal_size_left = (size.x - (widget->widget_padding.left + widget->widget_padding.right));
        float32 internal_size_used = offset.x;
        if(internal_size_used > internal_size_left)
        {
            u32 current_byte_offset = 0;
            while(internal_size_used > internal_size_left)
            {
                byte *character = widget->widget_text_buffer->data + current_byte_offset;
                dynamic_render_font_varient_t *varient = s_asset_font_acquire_font_at_size(ui_state->asset_manager,
                                                                                          &ui_state->default_font,
                                                                                           widget->font_size);
                glyph_metric_t *glyph = s_asset_font_fetch_glyph(ui_state->asset_manager, varient, character);
                byte buffer[4] = {};
                
                u32 UTF32_character = s_UTF8_convert_UTF32(character);
                string_t conversion_buffer = {
                    .data  = buffer,
                    .count = sizeof(buffer),
                };

                // NOTE(Sleepster): It's really weird we have to do this, but I'm not sure we can count how many
                // used bytes are actually in the UTF32 character
                s_UTF32_convert_to_UTF8(&conversion_buffer, UTF32_character);

                internal_size_used -= glyph->advance;
                widget_state->widget_text_render_start_offset += 1;
                widget_state->widget_text_render_end_offset   -= 1;

                current_byte_offset += conversion_buffer.count;
            }
        }

        // NOTE(Sleepster): Cursor 
        ui_row(ui_state, widget)
        {
            ui_widget_seed(ui_state, widget->ID);

            string_t name = c_string_concat(&gc->transient_arena, STR("TEXT_BOX_CURSOR_"), widget_name);
            ui_signal_t signal = ui_widget_rectangle(ui_state, name, vec2(12, size.y), {UI_WIDGET_SIZE_KIND_PIXELS, UI_WIDGET_SIZE_KIND_PIXELS});

            widget_t *rect = signal.widget;
            rect->state->render_color = vec4(0.0, 1.0, 0.0, 1.0);
            rect->state->offset = vec2(internal_size_used, widget->widget_padding.bottom);

            ui_widget_seed(ui_state, 0);
        }
    }

    return(result);
}

ui_signal_t
ui_widget_texture(ui_state_t     *ui_state, 
                  string_t        widget_name, 
                  vec2_t          size, 
                  asset_handle_t *texture, 
                  vec2_t          uv_min,
                  vec2_t          uv_max,
                  ivec2_t         size_kind,
                  u32             additional_flags)
{
    ui_signal_t result = {};

    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_DISPLAY_TEXTURE);
    widget->minimum_render_size   = size;
    widget->size_kind             = size_kind;
    widget->display_texture       = texture;
    widget->display_texture_uvmin = uv_min;
    widget->display_texture_uvmax = uv_max;
    widget->state->render_color   = vec4(1.0, 1.0, 1.0, 1.0);

    return(result);
}

#if 0
            if((!(input->keycode < 0x20 || input->keycode > 0x7F) &&  // C0 control codes + delete
                !(input->keycode >= 0x80 && input->keycode <= 0x9F))) // C1 control codes
            {
            }
#endif

#if 0
void
ui_widget_draw_demo_layout(ui_state_t *ui_state)
{
    ui_state_init(ui_state);

    widget_t *panel_widget = ui_widget_panel_create();
    ui_signal_t *data = ui_widget_get_signal_data(ui_state, panel_widget);

    if(data->is_visible)
    {
        ui_widget_parent(panel_widget))
        {
            signal_t button_signal = ui_widget_button();
            ui_widget_parent(signal->widget)
            {
                ui_widget_set_parent_layout(UI_WIDGET_LAYOUT_STYLE_HORIZONTAL);
                ui_widget_label();
            }
            ui_widget_pop_parent();

            if(ui_clicked(button_signal))
            {
                window_expanded = true;
            }

            if(dropdown_allowed)
            {
                ui_state_layout(UI_WIDGET_LAYOUT_STYLE_VERTICAL);

                signal_t start_button = ui_widget_labeled_button("start");
                signal_t play_button  = ui_widget_labeled_button("play");
                signal_t save_button  = ui_widget_labeled_button("save");
                signal_t quit_button  = ui_widget_labeled_button("quit");

                if(ui_clicked(start_button))
                {
                    // do thing...
                }
                // ...
            }
        }
        ui_widget_pop_parent();
    }
}
#endif

