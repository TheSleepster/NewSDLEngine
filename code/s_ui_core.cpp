/* ========================================================================
   $File: s_ui_core.cpp $
   $Date: April 27 2026 03:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_ui_core.h>
#include <r_immediate_rendering.h>

// TODO(Sleepster): Seeding in for() loops...
true_inline u64
ui_widget_hash(ui_state_t *ui_state, widget_t *widget)
{
    u64 result = 0;
    result = (c_fnv_hash_value(widget->widget_text.data, widget->widget_text.count) % ui_state->widget_iteractions.header.max_entries);

    return(result);
}

true_inline void
ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget)
{
    ui_state->parent_stack[ui_state->parent_stack_top++] = widget;
}

true_inline void
ui_widget_pop_parent(ui_state_t *ui_state)
{
    Assert(ui_state->parent_stack_top > 0);
    --ui_state->parent_stack_top;
}

true_inline widget_t*
ui_widget_get_top_parent(ui_state_t *ui_state)
{
    widget_t *result = null;

    if(ui_state->parent_stack_top > 0)
    {
        result = ui_state->parent_stack[ui_state->parent_stack_top - 1];
    }

    return(result);
}

true_inline void
ui_state_begin_frame(ui_state_t *ui_state)
{
    ui_state->widget_count = 0;
    c_arena_reset(&ui_state->widget_arena);
}

true_inline void
ui_state_end_frame(ui_state_t *ui_state)
{
    ui_state_update_widget_state(ui_state);
    ui_state_render_widgets(ui_state);
}

void
ui_state_update_widget_state(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        do {
            if(current_widget->first_child)
            {
                Assert(current_widget->last_child);

                // NOTE(Sleepster): Getting the expected size of each of the children 
                widget_t *current_child = current_widget->first_child;
                Assert(current_child);
                do {
                    // TODO(Sleepster): Calculate this properly... 
                    current_child->render_size = current_child->minimum_render_size;
                    current_child = current_child->next_sibling;
                }while(current_child != current_widget->first_child);

                float32 total_height  = 0.0; 
                float32 largest_width = 0.0;

                // NOTE(Sleepster): Walk the list BACKWARDS to build the parent's widget and height based off it's children. 
                current_child = current_widget->last_child;
                Assert(current_child);
                do {
                    total_height += current_child->render_size.y;
                    if(current_child->render_size.x > largest_width)
                    {
                        largest_width = current_child->render_size.x;
                    }

                    current_child = current_child->prev_sibling;
                }while(current_child != current_widget->last_child);

                current_widget->render_size = vec2(largest_width, total_height);

                // NOTE(Sleepster): Setting the children's position based off the offset of the parent 
                float32 placement_cursor = 0.0;

                current_child = current_widget->first_child;
                Assert(current_child);
                do {
                    float32 x_position = current_widget->expected_position.x;
                    float32 y_position = current_widget->expected_position.y;

                    // TODO(Sleepster): 
                    // For now, this just grows the panel downward, but we WILL want to grow to the side perhaps
                    current_child->expected_position.x = x_position;
                    current_child->expected_position.y = y_position + placement_cursor;
                    placement_cursor += current_child->render_size.y;

                    current_child->position = current_child->expected_position;

                    current_child = current_child->next_sibling;
                }while(current_child != current_widget->first_child);
            }
            else
            {
                current_widget->render_size = current_widget->minimum_render_size;
            }

            current_widget = current_widget->next_sibling;
        }while(current_widget != ui_state->first_widget);
    }
    else
    {
        log_warning("Called ui_state_update_widget_state on an empty ui_state_t... there are no widgets attached!!!\n");
    }
}


// NOTE(Sleepster): 
//
// NONE OF THESE WIDGETS WILL RENDER SINCE ALL OUR SHADERS SPECIFICALLY WANT A TEXTURE...


void
ui_state_render_widgets(ui_state_t *ui_state)
{
    renderer_state_t *renderer_state = ui_state->renderer;
    asset_manager_t  *asset_manager  = ui_state->asset_manager;
    (void)asset_manager;

    render_command_list_t *command_list = s_renderer_get_command_list(renderer_state, RENDER_COMMAND_LIST_TYPE_GRAPHICS);

    widget_t *current_widget = ui_state->first_widget;
    if(current_widget)
    {
        do {
            Assert(current_widget);

            // NOTE(Sleepster): Render the parent. 
            immediate_rect(command_list,
                           &ui_state->vertex_buffer,
                           current_widget->position, 
                           current_widget->render_size,
                           current_widget->render_color);

            // NOTE(Sleepster): Render each of the children. 
            widget_t *current_child = current_widget->first_child;
            if(current_child)
            {
                do {
                    immediate_rect(command_list,
                                   &ui_state->vertex_buffer,
                                   current_child->position, 
                                   current_child->render_size,
                                   current_child->render_color);

                    current_child = current_child->next_sibling;
                }while(current_child != current_widget->first_child);
            }

            current_widget = current_widget->next_sibling;
        }while(current_widget != ui_state->first_widget);

        r_cmd_renderpass_begin(command_list, ui_state->interface_framebuffer);
        r_cmd_bind_vertex_buffer(command_list, &ui_state->vertex_buffer);
        r_cmd_bind_index_buffer(command_list,  &ui_state->index_buffer);
        r_cmd_use_shader_program(command_list, ui_state->widget_shader);

        r_cmd_update_buffer_contents(command_list, &ui_state->vertex_buffer);

        s32 window_width  = Max(renderer_state->window_size.x, 10);
        s32 window_height = Max(renderer_state->window_size.y, 10);

        camera_matrices_t camera_matrix_buffer_data = {
            .view_matrix       = mat4_identity(),
            .projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1)
        };
        r_cmd_update_constant_buffer(command_list, ui_state->camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

        r_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
        r_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

        r_cmd_draw_indexed(command_list, ui_state->widget_count * 6, 0, 1, 0);
        r_cmd_renderpass_end(command_list);

        s_renderer_buffer_reset(ui_state->renderer, &ui_state->vertex_buffer);
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
              asset_manager_t  *asset_manager, 
              renderer_state_t *renderer_state, 
              u32               renderpass_ID)
{
    ZeroStruct(*ui_state);
    ui_state->widget_arena = c_arena_create(MB(10));
    ui_state->persistent_data_arena = c_arena_create(MB(100));
    c_hash_table_init(&ui_state->widget_iteractions, 
                       2097, 
                      &ui_state->persistent_data_arena, 
                       widget_hash_table_allocate_impl,
                       null);

    ui_state->renderer      = renderer_state;
    ui_state->asset_manager = asset_manager;
    ui_state->interface_framebuffer = renderpass_ID;

    ui_state->widget_shader = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_rectangle"));

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
widget_t*
ui_widget_create(ui_state_t *ui_state, string_t widget_name)
{
    widget_t *result = c_arena_push_struct(&ui_state->widget_arena, widget_t);
    ZeroStruct(*result);

    ++ui_state->widget_count;

    result->widget_text = widget_name;
    result->ID          = ui_widget_hash(ui_state, result);
    widget_t *parent    = ui_widget_get_top_parent(ui_state);
    if(parent != null)
    {
        if(parent->first_child)
        {
            widget_t *last_widget = parent->last_child;

            parent->last_child   = result;
            result->prev_sibling = last_widget;

            last_widget->next_sibling = result;
        }
        else
        {
            parent->first_child = result;
            parent->last_child  = result;

            parent->first_child->next_sibling = result;
            parent->last_child->prev_sibling  = result;
        }
    }
    else
    {
        if(ui_state->first_widget)
        {
            widget_t *last_widget = ui_state->last_widget; 

            last_widget->next_sibling = last_widget;
            result->prev_sibling      = last_widget;

            ui_state->last_widget = result;
        }
        else
        {
            ui_state->first_widget = result;
            ui_state->last_widget  = result;

            ui_state->first_widget->next_sibling = result;
            ui_state->last_widget->prev_sibling  = result;
        }
    }

    return(result);
}

widget_t*
ui_widget_panel(ui_state_t *ui_state, string_t widget_name, vec2_t position, vec4_t background_color)
{
    widget_t *widget = ui_widget_create(ui_state, widget_name);
    widget->position     = position;
    widget->render_color = background_color;

    return(widget);
}

widget_t*
ui_widget_button(ui_state_t *ui_state, string_t widget_name, vec2_t minimum_size, vec4_t idle_color, vec4_t hovered_color, vec4_t active_color)
{
    widget_t *widget = ui_widget_create(ui_state, widget_name);
    widget->minimum_render_size = minimum_size;
    widget->idle_color          = idle_color;
    widget->hovered_color       = hovered_color;
    widget->active_color        = active_color;

    widget->render_color        = idle_color;

    return(widget);
}

#if 0
void
ui_widget_draw_demo_layout(ui_state_t *ui_state)
{
    ui_state_init(ui_state);

    widget_t *panel_widget = ui_widget_panel_create();
    interaction_data_t *data = ui_widget_get_interaction_data(ui_state, panel_widget);

    if(data->is_visible)
    {
        ui_widget_parent(panel_widget))
        {
            ui_widget_button();
            ui_widget_textbox();
            ui_widget_button();
            ui_widget_slider();
            ui_widget_checkbox();
        }
    }
}
#endif

