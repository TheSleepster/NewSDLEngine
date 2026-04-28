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

struct interaction_data_t 
{
    u64    last_interacted_frame;
    bool32 clicked;
    bool32 hovered;
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

    vec2_t       position;
    vec2_t       render_size;
    vec4_t       render_color;

    vec2_t       minimum_render_size;

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

    vec2_t                            mouse_position;
    u64                               frame_count;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *active_parent;
};

// TODO(Sleepster): Seeding in for loops...
true_inline u64
ui_widget_hash(ui_state_t *ui_state, widget_t *widget)
{
    u64 result = 0;
    result = (c_fnv_hash_value(widget->widget_text.data, widget->widget_text.count) % ui_state->widget_iteractions.header.max_entries);

    return(result);
}

widget_t*
ui_widget_create(ui_state_t *ui_state, string_t widget_name)
{
    widget_t *result = c_arena_push_struct(&ui_state->widget_arena, widget_t);
    ZeroStruct(*result);

    result->widget_text = widget_name;
    result->ID          = ui_widget_hash(ui_state, result);
    if(ui_state->active_parent)
    {
        widget_t *parent = ui_state->active_parent;
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
ui_state_update_widget_state(ui_state_t *ui_state)
{
    for(widget_t *current_widget = ui_state->first_widget;
        current_widget;
        current_widget  = current_widget->next_sibling)
    {
        for(widget_t *current_child = current_widget->first_child;
            current_child;
            current_child = current_child->next_sibling)
        {
            current_child->render_size = current_child->minimum_render_size;
        }

        float32 total_height  = 0.0; 
        float32 largest_width = 0.0;
        for(widget_t *current_child = current_widget->first_child;
            current_child;
            current_child  = current_child->next_sibling)
        {
            total_height += current_child->render_size.y;
            if(current_child->render_size.x > largest_width)
            {
                largest_width = current_child->render_size.x;
            }
        }
    }
}

true_inline void
ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget)
{
    ui_state->active_parent = widget;
}

true_inline void
ui_widget_pop_parent(ui_state_t *ui_state)
{
    ui_state->active_parent = null;
}


internal_api
C_HASH_TABLE_ALLOCATE_IMPL(widget_hash_table_allocate_impl)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

void
ui_state_init(ui_state_t *ui_state)
{
    ZeroStruct(*ui_state);
    ui_state->widget_arena = c_arena_create(MB(10));
    c_hash_table_init(&ui_state->widget_iteractions, 
                       2097, 
                      &ui_state->widget_arena, 
                       widget_hash_table_allocate_impl,
                       null);
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

