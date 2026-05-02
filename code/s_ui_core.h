#if !defined(S_UI_CORE_H)
/* ========================================================================
   $File: s_ui_core.h $
   $Date: April 30 2026 12:08 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_UI_CORE_H
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>
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

    vec4_t       idle_color;
    vec4_t       hovered_color;
    vec4_t       active_color;

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
    // NOTE(Sleepster): Lasts one frame...
    memory_arena_t                    widget_arena;

    // NOTE(Sleepster): Persists between frames... 
    memory_arena_t                    persistent_data_arena;
    HashTable_t(interaction_data_t *) widget_iteractions;

    renderer_state_t                 *renderer;
    asset_manager_t                  *asset_manager;
    uniform_constant_buffer_t        *camera_matrices_buffer;

    asset_handle_t                    widget_shader;
    asset_handle_t                    font_shader;

    vec2_t                            mouse_position;
    u32                               widget_count;
    u64                               frame_count;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *parent_stack[MAX_PARENT_WIDGETS];
    u32                               parent_stack_top;

    u32                               interface_framebuffer;
    vertex_buffer_t                   vertex_buffer;
    render_buffer_t                   index_buffer;
};

void      ui_state_init(ui_state_t *ui_state, asset_manager_t *asset_manager, renderer_state_t *renderer_state, u32 renderpass_ID);
void      ui_state_update_widget_state(ui_state_t *ui_state);
void      ui_state_render_widgets(ui_state_t *ui_state);

true_inline void ui_state_begin_frame(ui_state_t *ui_state);
true_inline void ui_state_end_frame(ui_state_t *ui_state);
true_inline void ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget);
true_inline void ui_widget_pop_parent(ui_state_t *ui_state);

widget_t* ui_widget_create(ui_state_t *ui_state, string_t widget_name);
widget_t* ui_widget_panel(ui_state_t *ui_state, string_t widget_name, vec2_t position, vec4_t background_color);
widget_t* ui_widget_button(ui_state_t *ui_state, string_t widget_name, vec2_t minimum_size, vec4_t idle_color, vec4_t hovered_color, vec4_t active_color);

#endif // S_UI_CORE_H

