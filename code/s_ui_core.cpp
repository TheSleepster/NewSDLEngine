/* ========================================================================
   $File: s_ui_core.cpp $
   $Date: April 27 2026 03:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_ui_core.h>
#include <r_immediate_rendering.h>

true_inline void
ui_state_begin_frame(ui_state_t *ui_state)
{
    ui_state->widget_count     = 0;
    ui_state->ui_seed          = 0;
    ui_state->parent_stack_top = 1;

    ui_state->current_font_size  = ui_state->default_font_size;
    ui_state->current_font_color = ui_state->default_font_color;

    c_arena_reset(&ui_state->widget_arena);
}

true_inline void
ui_state_end_frame(ui_state_t *ui_state, render_command_list_t *command_list)
{
    ui_state_update_widget_state(ui_state);
    ui_state_render_widgets(ui_state, command_list);
}

struct widget_child_size_data_t
{
    u32    largest_width;
    u32    tallest_height;
    u32    total_width;
    u32    total_height;
    vec2_t placement_cursor;
};

internal_api widget_child_size_data_t 
get_hierarchy_size_data(ui_state_t *ui_state, widget_t *first_widget)
{
    widget_child_size_data_t result = {};
    if(first_widget)
    {

        widget_t *current_widget = first_widget;
        do {
            current_widget->state->render_size = current_widget->minimum_render_size;
            if(current_widget->first_child)
            {
                result = get_hierarchy_size_data(ui_state, 
                                                 current_widget->first_child);
            }

            result.total_width  += current_widget->state->render_size.x;
            result.total_height += current_widget->state->render_size.y;
            if(result.largest_width < current_widget->state->render_size.x)
            {
                result.largest_width = current_widget->state->render_size.x;
            }

            if(result.tallest_height < current_widget->state->render_size.y)
            {
                result.tallest_height = current_widget->state->render_size.y;
            }

            current_widget = current_widget->next_sibling;
        }while(current_widget != first_widget);
    }

    return(result);
}

internal_api void
place_widgets_in_hierarchy(u32 layout_style, widget_t *first_widget, widget_t *last_widget, vec2_t *parent_cursor)
{
    widget_t *current_widget = first_widget;
    do {
        current_widget->expected_position.xy = *parent_cursor;
        current_widget->expected_position.z  =  current_widget->parent_stack_depth;

        vec2_t advance;
        if(layout_style == WIDGET_LAYOUT_STYLE_VERTICAL)
        {
            advance = vec2(0.0f, -current_widget->state->render_size.y);
        }
        else
        {
            advance = vec2(current_widget->state->render_size.x, 0.0f);
        }

        *parent_cursor = vec2_add(*parent_cursor, advance);

        if(current_widget->first_child)
        {
            vec2_t parent_relative_cursor = current_widget->expected_position.xy;
            place_widgets_in_hierarchy(current_widget->layout_style, current_widget->first_child, current_widget->last_child, &parent_relative_cursor);
        }

        current_widget->state->position = vec3(current_widget->expected_position.x,
                                               current_widget->expected_position.y - current_widget->state->render_size.y,
                                               current_widget->expected_position.z);

        current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy, 
                                                          current_widget->state->render_size);

        if(last_widget) current_widget = current_widget->next_sibling;
        else            current_widget = null;
    }while(current_widget != first_widget && current_widget);
}

internal_api void
ui_state_update_widget_hierarchy(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    do {
        // NOTE(Sleepster): Set the size of the top-most parent here 
        widget_child_size_data_t size_data = get_hierarchy_size_data(ui_state, 
                                                                     current_widget->first_child);
        if(current_widget->layout_style == WIDGET_LAYOUT_STYLE_VERTICAL)
        {
            current_widget->state->render_size = vec2(size_data.largest_width, 
                                                      size_data.total_height);
        }
        else
        {
            current_widget->state->render_size = vec2(size_data.total_width, size_data.tallest_height);
        }

        // NOTE(Sleepster): Place the children in relative locations to that of the parent 
        vec2_t placement_cursor = vec2_zero();
        place_widgets_in_hierarchy(current_widget->layout_style, current_widget, null, &placement_cursor);

        current_widget = current_widget->next_sibling;
    }while(current_widget != ui_state->first_widget);
}

void
ui_state_update_widget_state(ui_state_t *ui_state)
{
    renderpass_t *renderpass = ui_state->renderer->renderpasses + ui_state->interface_framebuffer;
    
    float32 half_width  = renderpass->render_width  * 0.5f;
    float32 half_height = renderpass->render_height * 0.5f;

    ui_state->current_camera = {
        .view_matrix       = mat4_identity(),
        .projection_matrix = mat4_RHDX_ortho(-half_width, half_width, -half_height, half_height, 0.0f, 1.0f)
    };

    ui_state->mouse_position = s_im_transform_mouse_data(ui_state->ui_controller, 
                                                         vec2(renderpass->render_width, renderpass->render_height), 
                                                         ui_state->current_camera.view_matrix,
                                                         ui_state->current_camera.projection_matrix);
    ui_state_update_widget_hierarchy(ui_state);
}
#if 0
        if(current_widget->widget_flags & UI_WIDGET_FLAG_HAS_TEXT)
        {
            immediate_text(command_list, 
                          &ui_state->vertex_buffer, 
                           ui_state->asset_manager,
                          &ui_state->default_font,
                           current_widget->widget_text,
                           current_widget->state->position, 
                           current_widget->state->render_color,
                           vec2(2.0, 0.0),
                           ui_state->default_font_size);
        }
#endif

internal_api void
render_widget_hierarchy(ui_state_t *ui_state, render_command_list_t *command_list, widget_t *first_widget)
{
    widget_t *current_widget = first_widget;
    do {
        if(current_widget->widget_flags & UI_WIDGET_FLAG_DRAW_RECTANGLE)
        {
            immediate_rect(command_list,
                           &ui_state->vertex_buffer,
                           current_widget->state->position, 
                           current_widget->state->render_size,
                           current_widget->state->render_color,
                           vec2(0, 0));
        }

        if(current_widget->first_child)
        {
            render_widget_hierarchy(ui_state, command_list, current_widget->first_child);
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != first_widget);
}

void
ui_state_render_widgets(ui_state_t *ui_state, render_command_list_t *command_list)
{
    renderer_state_t *renderer_state = ui_state->renderer;
    asset_manager_t  *asset_manager  = ui_state->asset_manager;
    (void)asset_manager;

    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        render_widget_hierarchy(ui_state, command_list, current_widget);

        r_cmd_bind_vertex_buffer(command_list, &ui_state->vertex_buffer);
        r_cmd_bind_index_buffer(command_list,  &ui_state->index_buffer);

        // NOTE(Sleepster): First, draw the normal rectangle widgets
        {
            r_cmd_use_shader_program(command_list, ui_state->widget_shader);
            r_cmd_update_buffer_contents(command_list, &ui_state->vertex_buffer);

            s32 window_width  = Max(renderer_state->window_size.x, 10);
            s32 window_height = Max(renderer_state->window_size.y, 10);

            r_cmd_update_constant_buffer(command_list, ui_state->camera_matrices_buffer, &ui_state->current_camera, sizeof(camera_matrices_t));

            r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

            r_cmd_draw_indexed(command_list, ui_state->widget_count * 6, 0, 1, 0);
            s_renderer_buffer_reset(ui_state->renderer, &ui_state->vertex_buffer);
        }
    }
    else
    {
        log_warning("Called ui_state_render_widgets on an empty ui_state_t... there are no widgets attached!!!\n");
    }
}

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(widget_hash_table_allocate_impl)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

void
ui_state_init(ui_state_t       *ui_state, 
              input_manager_t  *input_manager,
              asset_manager_t  *asset_manager, 
              renderer_state_t *renderer_state, 
              u32               renderpass_ID)
{
    ZeroStruct(*ui_state);
    ui_state->widget_arena = c_arena_create(MB(10));
    ui_state->persistent_data_arena = c_arena_create(MB(100));
    c_hash_table_init(&ui_state->widget_states, 
                       2096, 
                      &ui_state->persistent_data_arena, 
                       widget_hash_table_allocate_impl,
                       null);

    ui_state->renderer              = renderer_state;
    ui_state->asset_manager         = asset_manager;
    ui_state->interface_framebuffer = renderpass_ID;
    ui_state->parent_stack_top      = 1;
    ui_state->hot_widget_ID         = INVALID_ID;
    ui_state->active_widget_ID      = INVALID_ID;

    ui_state->default_font_color = vec4_create(1.0);
    ui_state->default_font_size  = 16;

    ui_state->input_manager = input_manager;
    ui_state->ui_controller = s_im_get_primary_controller(input_manager);

    ui_state->widget_shader = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_widget"));
    ui_state->default_font  = s_asset_manager_acquire_asset_handle(asset_manager, STR("LiberationMono_Regular"));

    u32 *indices = c_arena_push_array(&renderer_state->transient_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
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
    ui_state->vertex_buffer = s_renderer_vertex_buffer_create(renderer_state, 
                                                              RenderBufferAllocationTypeMapped,
                                                              RenderBufferAdvanceRate_PerElement,
                                                              (byte*)vertices,
                                                              sizeof(immediate_vertex_t),
                                                              VERTEX_BUFFER_SIZE);
    ui_state->index_buffer = s_renderer_index_buffer_create(renderer_state, 
                                                            RenderBufferAllocationTypeGPUOnly,
                                                            sizeof(u32),
                                                            indices,
                                                            sizeof(u32) * (6 * MAX_WIDGETS));

    ui_state->camera_matrices_buffer = s_renderer_get_constant_buffer(renderer_state, STR("CameraMatrices"));
}

// NOTE(Sleepster): Widget functions

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
ui_widget_set_parent_layout(ui_state_t *ui_state, u32 layout_style)
{
    widget_t *parent = ui_widget_get_top_parent(ui_state);
    parent->layout_style = layout_style;
}

true_inline void
ui_widget_set_font_color(ui_state_t *ui_state, vec4_t color)
{
    ui_state->current_font_color = color;
}

true_inline void
ui_widget_set_current_font_size(ui_state_t *ui_state, u32 font_size)
{
    ui_state->current_font_size = font_size;
}

true_inline void
ui_widget_seed(ui_state_t *ui_state, u64 index)
{
    u64 hash = c_fnv_hash_value((byte*)&index, sizeof(u64));
    ui_state->ui_seed = hash;
}

true_inline u64
ui_widget_hash(ui_state_t *ui_state, widget_t *widget)
{
    u64 result = 0;
    result = (c_fnv_hash_value(widget->widget_text.data, widget->widget_text.count));
    if(ui_state->ui_seed != 0)
    {
        result = c_combine_hashes(result, ui_state->ui_seed);
    }

    result %= ui_state->widget_states.header.max_entries;
    return(result);
}


internal_api true_inline float32
ui_widget_determine_depth(ui_state_t *ui_state)
{
    float32 result = 0.0f;
    float32 layer_depth_value = 1.0f - 2.0f * ((float32)ui_state->parent_stack_top / (float32)(MAX_WIDGET_LAYERS - 1));

    result = layer_depth_value;

    return(result);
}

void
ui_widget_append(widget_t **first_node, widget_t **last_node, widget_t *widget)
{
    if(*first_node)
    {
        widget_t *old_last = *last_node;

        widget->prev_sibling   = old_last;
        widget->next_sibling   = *first_node;
        old_last->next_sibling = widget;

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

    ++ui_state->widget_count;

    result->widget_text        = widget_name;
    result->widget_flags       = widget_flags;
    result->ID                 = ui_widget_hash(ui_state, result);
    result->parent_stack_depth = ui_widget_determine_depth(ui_state);
    result->state              = ui_state->widget_states.data + result->ID;

    // NOTE(Sleepster): NEXT AND PREVIOUS ARE BROKEN... 
    widget_t *parent = ui_widget_get_top_parent(ui_state);
    if(parent != null)
    {
        ui_widget_append(&parent->first_child, &parent->last_child, result);
    }
    else
    {
        ui_widget_append(&ui_state->first_widget, &ui_state->last_widget, result);
    }

    return(result);
}

ui_signal_t
ui_widget_get_signals(ui_state_t *ui_state, widget_t *widget)
{
    ui_signal_t result = {};
    result.widget = widget;

    widget_state_t *widget_state = ui_state->widget_states.data + widget->ID;
    Assert(widget_state);

    bool8 is_hot = rect2_point_in_rect(widget_state->widget_rect, ui_state->mouse_position);
    if(is_hot)
    {
        action_button_t *left_mouse  = s_im_get_key_state(ui_state->ui_controller, SDL_LEFT_MOUSE);
        action_button_t *right_mouse = s_im_get_key_state(ui_state->ui_controller, SDL_RIGHT_MOUSE);

        ui_state->hot_widget_ID = widget->ID;

        result.signal_flags |= UI_SIGNAL_FLAG_HOVERING;

        bool8 is_active         = left_mouse->is_down || left_mouse->is_pressed;
        bool8 is_held           = left_mouse->is_down;
        bool8 just_released     = left_mouse->is_released;
        bool8 just_clicked      = left_mouse->is_pressed;
        bool8 is_double_clicked = left_mouse->half_transition_counter  >= 2;

        bool8 is_right_clicked = right_mouse->is_pressed;
        if(is_active)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DRAGGING; 
        }

        if(is_held)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DRAGGING; 
        }

        if(just_released)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_RELEASED; 
        }

        if(just_clicked)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_CLICKED; 
        }

        if(is_double_clicked)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_LEFT_DOUBLE_CLICKED;
        }

        if(is_right_clicked)
        {
            result.signal_flags |= UI_SIGNAL_FLAG_RIGHT_CLICKED;
        }
    }

    return(result);
}

ui_signal_t
ui_widget_panel(ui_state_t *ui_state, string_t widget_name, vec2_t position, vec4_t background_color)
{
    widget_t *widget = ui_widget_create(ui_state, widget_name, UI_WIDGET_FLAG_IDLE_COLOR|UI_WIDGET_FLAG_DRAW_RECTANGLE);
    widget->expected_position   = vec2_expand_vec3(position, widget->parent_stack_depth);
    widget->state->render_color = background_color;
    widget->toggled             = widget->state->toggled;

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    return(result);
}

ui_signal_t
ui_widget_button(ui_state_t *ui_state, 
                 string_t    widget_name, 
                 vec2_t      minimum_size, 
                 vec4_t      idle_color, 
                 vec4_t      hovered_color, 
                 vec4_t      active_color)
{
    widget_t *widget = ui_widget_create(ui_state, 
                                        widget_name, 
                                        UI_WIDGET_FLAG_STANDARD_RECTANGLE_BUTTON);
    widget->minimum_render_size = minimum_size;
    widget->idle_color          = idle_color;
    widget->hovered_color       = hovered_color;
    widget->active_color        = active_color;
    widget->state->render_color = idle_color;

    ui_signal_t result = ui_widget_get_signals(ui_state, widget);
    if(ui_hovered(result))
    {
        widget->state->render_color = hovered_color;
    }
    
    if(ui_down(result))
    {
        widget->state->render_color = widget->active_color;
    }

    return(result);
}

ui_signal_t
ui_widget_text(ui_state_t *ui_state, string_t widget_text, vec4_t text_color)
{
    widget_t *widget = ui_widget_create(ui_state, widget_text, UI_WIDGET_FLAG_DRAW_TEXT|UI_WIDGET_FLAG_IDLE_COLOR);
    widget->state->render_color = text_color;
    widget->minimum_render_size = s_asset_font_get_string_size(ui_state->asset_manager, 
                                                               widget_text, 
                                                              &ui_state->default_font, 
                                                               ui_state->current_font_size);
    ui_signal_t result = ui_widget_get_signals(ui_state, widget);

    return(result);
}

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
                ui_widget_set_parent_layout(WIDGET_LAYOUT_STYLE_HORIZONTAL);
                ui_widget_label();
            }
            ui_widget_pop_parent();

            if(ui_clicked(button_signal))
            {
                window_expanded = true;
            }

            if(dropdown_allowed)
            {
                ui_state_layout(WIDGET_LAYOUT_STYLE_VERTICAL);

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

