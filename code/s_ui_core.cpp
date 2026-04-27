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
};

struct widget_t
{
    u64      ID;
    string_t text;

    vec2_t position;
    vec2_t render_size;
    vec4_t render_color;

    // NOTE(Sleepster): 
    // If this is a tree... 
    widget_t *parent;
    widget_t *first_child;
    widget_t *last_child;

    // NOTE(Sleepster): 
    // Chaining upon a linked list... 
    widget_t *next_sibling;
    widget_t *prev_sibling;
};

struct ui_state_t
{
    memory_arena_t                    widget_arena;
    HashTable_t(interaction_data_t *) widget_iteractions;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *active_parent;
};

widget_t*
ui_widget_create(ui_state_t *ui_state)
{
    widget_t *result = c_arena_push_struct(&ui_state->widget_arena, widget_t);
    ZeroStruct(*result);

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

u64
ui_widget_hash(widget_t *widget)
{
    u64 result = 0;

    Expect(false, "This function is not implemented...\n");

    return(result);
}

void
ui_state_init(ui_state_t *ui_state)
{
    ZeroStruct(*ui_state);
    ui_state->widget_arena = c_arena_create(MB(10));
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

