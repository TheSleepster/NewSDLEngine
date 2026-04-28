/* ========================================================================
   $File: s_ui_core.cpp $
   $Date: April 27 2026 03:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_memory_arena.h>

#include <s_render_RHI.h>

constexpr u32 MAX_PARENT_WIDGETS = 256;
constexpr u32 MAX_WIDGETS        = 1024;

struct interaction_data_t 
{
    u64    last_interacted_frame;
    bool32 is_clicked;
    bool32 is_hot;
    bool32 is_open;
};

enum widget_flags_t
{
    UI_WIDGET_FLAG_INVALID,
    UI_WIDGET_FLAG_IDLE_COLOR,
    UI_WIDGET_FLAG_HOVER_COLOR,
    UI_WIDGET_FLAG_ACTIVE_COLOR,
    UI_WIDGET_FLAG_HAS_TEXT,
};

struct widget_t
{
    u64          ID;
    string_t     widget_text;
    bool8        toggled;
    
    vec2_t       position;
    vec2_t       render_size;
    vec4_t       render_color;

    vec2_t       minimum_render_size;
    vec2_t       expected_position;

    rectangle2_t widget_rect;

    // NOTE(Sleepster): 
    // If this is a tree... 
    widget_t    *parent;
    widget_t    *first_child;
    widget_t    *last_child;

    // NOTE(Sleepster): 
    // Chaining upon a linked list... 
    widget_t    *next_sibling;
    widget_t    *prev_sibling;
};

struct ui_state_t
{
    memory_arena_t                    widget_arena;
    HashTable_t(interaction_data_t *) widget_iteractions;

    renderer_state_t                 *renderer;
    asset_manager_t                  *asset_manager;
    uniform_constant_buffer_t        *camera_matrices_buffer;

    vec2_t                            mouse_position;
    u32                               widget_count;
    u64                               frame_count;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *parent_stack[MAX_PARENT_WIDGETS];
    u32                               parent_stack_top;

    u32                               interface_framebuffer;
    render_buffer_t                   vertex_buffer;
    render_buffer_t                   index_buffer;

    immediate_vertex_t               *vertices;
    u32                               vertex_count;
};

// TODO(Sleepster): Seeding in for loops...
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
    --ui_state->parent_stack_top;
}

true_inline widget_t*
ui_widget_get_top_parent(ui_state_t *ui_state)
{
    widget_t *result = null;

    if(ui_state->parent_stack_top > 0)
    {
        result = ui_state->parent_stack[ui_state->parent_stack_top];
    }

    return(result);
}

widget_t*
ui_widget_create(ui_state_t *ui_state, string_t widget_name)
{
    widget_t *result = c_arena_push_struct(&ui_state->widget_arena, widget_t);
    ZeroStruct(*result);

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
        }
    }

    return(result);
}

void
draw_rect(ui_state_t *ui_state,
          vec2_t      position,
          vec2_t      render_size,
          vec4_t      render_color)
{
}

void
ui_state_update_widget_state(ui_state_t *ui_state)
{
    widget_t *current_widget = ui_state->first_widget;
    do {
        if(current_widget->first_child)
        {
            // NOTE(Sleepster): Getting the expected size of each of the children 
            widget_t *current_child = current_widget->first_child;
            do {
                // TODO(Sleepster): Calculate this properly... 
                current_child->render_size = current_child->minimum_render_size;
                current_child = current_child->next_sibling;
            }while(current_child != current_widget->first_child);

            // NOTE(Sleepster): Calculating the parent's width and height depending on it's children 
            float32 total_height  = 0.0; 
            float32 largest_width = 0.0;

            current_child = current_widget->first_child;
            do {
                total_height += current_child->render_size.y;
                if(current_child->render_size.x > largest_width)
                {
                    largest_width = current_child->render_size.x;
                }

                current_child = current_child->next_sibling;
            }while(current_child != current_widget->first_child);

            current_widget->render_size = vec2(largest_width, total_height);

            // NOTE(Sleepster): Setting the children's position based off the offset of the parent 
            float32 placement_cursor = 0.0;
            current_child = current_widget->first_child;
            do {
                float32 x_position = current_widget->expected_position.x;
                float32 y_position = current_widget->expected_position.y;

                // TODO(Sleepster): 
                // For now, this just grows the panel downward, but we WILL want to grow to the side perhaps
                current_child->expected_position.x = x_position;
                current_child->expected_position.y = y_position + placement_cursor;
                placement_cursor += current_child->render_size.y;

                current_child = current_child->next_sibling;
            }while(current_child != current_widget->first_child);

            // NOTE(Sleepster): Fill the vertex buffer
            current_child = current_widget->first_child;
            do {
                current_child = current_child->next_sibling;
            }while(current_child != current_widget);
        }
        else
        {
            current_widget->render_size = current_widget->minimum_render_size;
        }

        current_widget = current_widget->next_sibling;
    }while(current_widget != ui_state->first_widget);
}

void
ui_state_render_widgets(ui_state_t *ui_state)
{
    renderer_state_t *renderer_state = ui_state->renderer;
    asset_manager_t  *asset_manager  = ui_state->asset_manager;
    (void)asset_manager;

    render_command_list_t *command_list = s_renderer_get_command_list(renderer_state);

    r_cmd_renderpass_begin(command_list, ui_state->interface_framebuffer);
    r_cmd_bind_vertex_buffer(command_list, &ui_state->vertex_buffer);
    r_cmd_bind_index_buffer(command_list,  &ui_state->index_buffer);

    r_cmd_update_buffer_contents(command_list, 
                                &ui_state->vertex_buffer, 
                                ui_state->vertices, 
                                ui_state->vertex_count * sizeof(immediate_vertex_t));

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

    ui_state->vertex_count = 0;
}

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(widget_hash_table_allocate_impl)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

void
ui_state_init(ui_state_t *ui_state, 
              asset_manager_t *asset_manager, 
              renderer_state_t *renderer_state, 
              u32 renderpass_ID)
{
    ZeroStruct(*ui_state);
    ui_state->widget_arena = c_arena_create(MB(10));
    c_hash_table_init(&ui_state->widget_iteractions, 
                       2097, 
                      &ui_state->widget_arena, 
                       widget_hash_table_allocate_impl,
                       null);

    ui_state->renderer      = renderer_state;
    ui_state->asset_manager = asset_manager;
    ui_state->interface_framebuffer = renderpass_ID;

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
    ui_state->vertex_buffer = s_renderer_vertex_buffer_create(renderer_state, 
                                                              RenderBufferAllocationTypeMapped,
                                                              sizeof(immediate_vertex_t),
                                                              RenderBufferAdvanceRate_PerElement,
                                                              null,
                                                              sizeof(immediate_vertex_t) * MAX_WIDGETS);
    ui_state->index_buffer = s_renderer_index_buffer_create(renderer_state, 
                                                            RenderBufferAllocationTypeGPUOnly,
                                                            sizeof(u32),
                                                            indices,
                                                            sizeof(u32) * (6 * MAX_WIDGETS));

    ui_state->camera_matrices_buffer = s_renderer_get_constant_buffer(renderer_state, STR("CameraMatrices"));
    ui_state->vertices = c_arena_push_array(&ui_state->widget_arena, immediate_vertex_t, MAX_WIDGETS * 4);
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

