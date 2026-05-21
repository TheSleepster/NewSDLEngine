/* ========================================================================
   $File: old_widget_sizing_code.cpp $
   $Date: May 19 2026 04:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */


#if 0 
struct widget_child_size_data_t
{
    u32    largest_width;
    u32    tallest_height;
    u32    total_width;
    u32    total_height;
    vec2_t placement_cursor;
};

internal_api widget_child_size_data_t 
get_hierarchy_size_data(ui_state_t *ui_state, widget_t *parent, widget_t *first_widget)
{
    widget_child_size_data_t result = {};
    if(first_widget)
    {
        widget_t *current_widget = first_widget;
        do {
            current_widget->state->render_size = vec2(current_widget->minimum_render_size.x, 
                                                      current_widget->minimum_render_size.y);
            if(current_widget->first_child)
            {
                widget_child_size_data_t child_data = get_hierarchy_size_data(ui_state, 
                                                                              current_widget,
                                                                              current_widget->first_child);
                result.total_width  += child_data.total_width;
                result.total_height += child_data.total_height;

                if(result.largest_width < child_data.largest_width)
                {
                    result.largest_width = child_data.largest_width;
                }

                if(result.tallest_height < child_data.tallest_height)
                {
                    result.tallest_height = child_data.tallest_height;
                }

                current_widget->state->render_size = vec2(child_data.largest_width, child_data.tallest_height);
                if(current_widget->state->render_size.x < current_widget->minimum_render_size.x ||
                   current_widget->state->render_size.y < current_widget->minimum_render_size.y)
                {
                    current_widget->state->render_size = current_widget->minimum_render_size;
                }
            }
            u32 layout_style = parent->layout_style;

            float32 true_width  = 0.0f; 
            float32 true_height = 0.0f;
            if(layout_style == WIDGET_LAYOUT_STYLE_HORIZONTAL)
            {
                true_width  = current_widget->state->render_size.x;
                true_height = current_widget->state->render_size.y; 

                result.total_width  += true_width;
            }
            else
            {
                true_width  = current_widget->state->render_size.x;
                true_height = current_widget->state->render_size.y;

                result.total_height += true_height;
            }

            if(result.largest_width < true_width)
            {
                result.largest_width = true_width;
            }

            if(result.tallest_height < true_height)
            {
                result.tallest_height = true_height;
            }

            current_widget = current_widget->next_sibling;
        }while(current_widget != first_widget);
    }

    return(result);
}

internal_api void
place_widgets_in_hierarchy(ui_state_t *ui_state,
                           u32         layout_style, 
                           widget_t   *parent, 
                           widget_t   *first_widget, 
                           widget_t   *last_widget, 
                           vec2_t     *parent_cursor)
{
    widget_t *current_widget = first_widget;
    do {
        vec2_t widget_position = *parent_cursor;

        current_widget->expected_position.xy = vec2_add(widget_position, vec2(current_widget->offset_from_parent, 0.0f));
        current_widget->expected_position.z  = current_widget->parent_stack_depth;

        float32 parent_spacing = 0.0f;
        vec2_t  parent_padding = vec2_zero();
        if(parent)
        {
            parent_spacing = parent->child_spacing;
            parent_padding = parent->padding;
        }

        vec2_t  advance;
        if(layout_style == WIDGET_LAYOUT_STYLE_VERTICAL)
        {
            float32 y_advance = (current_widget->state->render_size.y + parent_spacing) * -1.0f;
            advance = vec2(0.0, y_advance);
        }
        else
        {
            float32 x_advance = (current_widget->state->render_size.x + parent_spacing);
            advance = vec2(x_advance, 0.0f);
        }

        vec2_t parent_top_left = current_widget->expected_position.xy + current_widget->state->offset;

        // NOTE(Sleepster): We have to place the parent before we can place the children, otherwise we get weird popping 
        current_widget->state->position.xy = vec2(parent_top_left.x,
                                                  parent_top_left.y - current_widget->state->render_size.y);
        current_widget->state->position.z  = current_widget->parent_stack_depth;
        current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy, 
                                                          current_widget->state->render_size);

        if(current_widget->first_child)
        {
            vec2_t parent_relative_cursor = vec2(current_widget->state->position.x,
                                                 current_widget->state->position.y + current_widget->state->render_size.y);
            place_widgets_in_hierarchy(ui_state,
                                       current_widget->layout_style, 
                                       current_widget,
                                       current_widget->first_child, 
                                       current_widget->last_child, 
                                      &parent_relative_cursor);
        }
        *parent_cursor = vec2_add(*parent_cursor, advance);

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
                                                                     current_widget,
                                                                     current_widget->first_child);

        // NOTE(Sleepster): Place the children in relative locations to that of the parent 
        current_widget->expected_position = current_widget->state->position;
        vec2_t placement_cursor = vec2(current_widget->expected_position.x,
                                       current_widget->expected_position.y + current_widget->state->render_size.y);
        if(current_widget->layout_style == WIDGET_LAYOUT_STYLE_VERTICAL)
        {
            current_widget->state->render_size = vec2(size_data.largest_width, 
                                                      size_data.total_height);
        }
        else
        {
            current_widget->state->render_size = vec2(size_data.total_width, size_data.total_height);
        }

        place_widgets_in_hierarchy(ui_state,
                                   current_widget->layout_style, 
                                   null,
                                   current_widget, 
                                   null, 
                                  &placement_cursor);

        current_widget = current_widget->next_sibling;
    }while(current_widget != ui_state->first_widget);
}
#endif




// NOTE(Sleepster): NEW SECTION
#if 0
internal_api vec2_t
size_widget_hierarchy(widget_t *current_widget)
{
    vec2_t result = vec2_zero();

    float32 padding_x = current_widget->widget_padding.left + current_widget->widget_padding.right;
    float32 padding_y = current_widget->widget_padding.top  + current_widget->widget_padding.bottom;

    vec2_t widget_size = current_widget->minimum_render_size;
    if(current_widget->first_child)
    {
        vec2_t hierarchy_size = vec2_zero();
        u32    node_count     = 0;

        widget_t *current_child = current_widget->first_child;
        do {
            vec2_t child_size = size_widget_hierarchy(current_child);

            if(current_widget->layout_style == WIDGET_LAYOUT_STYLE_VERTICAL)
            {
                hierarchy_size.x  = Max(hierarchy_size.x, child_size.x);
                hierarchy_size.y += child_size.y;
            }
            else
            {
                hierarchy_size.x += child_size.x;
                hierarchy_size.y  = Max(hierarchy_size.y, child_size.y);
            }

            ++node_count;
            current_child = current_child->next_sibling;
        }while(current_child != current_widget->first_child);

        if(node_count > 1)
        {
            if(current_widget->layout_style == WIDGET_LAYOUT_STYLE_VERTICAL) 
            {
                hierarchy_size.y += (float32)(node_count) * current_widget->child_spacing.y;
            }
            else 
            {
                hierarchy_size.x += (float32)(node_count) * current_widget->child_spacing.x;
            }
        }

        widget_size.x = Max(widget_size.x, hierarchy_size.x);
        widget_size.y = Max(widget_size.y, hierarchy_size.y);
    }

    current_widget->state->render_size = vec2(widget_size.x + padding_x, widget_size.y + padding_y);

    result = current_widget->state->render_size;
    return(result);
}

internal_api void
size_all_widgets(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    do {
        size_widget_hierarchy(current_widget);
        current_widget = current_widget->next_sibling;
    }while(current_widget != ui_state->first_widget);
}

internal_api void
place_widgets_in_hierarchy(widget_t *first_widget, vec2_t *placement_cursor, u32 layout_style)
{
    widget_t *current_widget = first_widget;

    do { 
        vec2_t widget_cursor = *placement_cursor;

        current_widget->expected_position  = vec2_expand_vec3(widget_cursor, current_widget->parent_stack_depth);
        current_widget->state->position    = vec3(current_widget->expected_position.x + current_widget->state->offset.x, 
                                                  current_widget->expected_position.y + current_widget->state->offset.y, 
                                                  current_widget->parent_stack_depth);
        current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy, current_widget->state->render_size);

        if(layout_style == WIDGET_LAYOUT_STYLE_HORIZONTAL)
        {
            placement_cursor->x += current_widget->state->render_size.x + current_widget->parent_child_spacing.x;
        }
        else
        {
            placement_cursor->y -= (current_widget->state->render_size.y + current_widget->parent_child_spacing.y);
        }

        if(current_widget->first_child)
        {
            vec2_t parent_relative_cursor = vec2(current_widget->state->position.x,
                                                 current_widget->state->position.y);
            place_widgets_in_hierarchy(current_widget->first_child, &parent_relative_cursor, current_widget->layout_style);
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != first_widget);
}

internal_api void
place_all_widgets(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    do {
        current_widget->state->position = vec3(current_widget->expected_position.x + current_widget->state->offset.x, 
                                              (current_widget->expected_position.y + current_widget->state->offset.y) - current_widget->state->render_size.y, 
                                               current_widget->parent_stack_depth);
        current_widget->state->widget_rect = rect2_create(current_widget->state->position.xy, current_widget->state->render_size);

        vec2_t placement_cursor = vec2(current_widget->state->position.x  + current_widget->max_left_padding,
                                       (current_widget->state->position.y + current_widget->state->render_size.y) - (current_widget->max_top_padding + current_widget->max_bottom_padding));
        if(current_widget->first_child)
        {
            place_widgets_in_hierarchy(current_widget->first_child, &placement_cursor, current_widget->layout_style);
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != ui_state->first_widget);
}
#endif
