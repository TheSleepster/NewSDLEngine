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
#include <s_input_manager.h>

#include <s_render_RHI.h>

constexpr u32 MAX_PARENT_WIDGETS = 256;
constexpr u32 MAX_WIDGETS        = 1024;
constexpr u32 MAX_WIDGET_LAYERS  = 32;

struct widget_t;

struct widget_state_t
{
    u32          last_interacted_frame;
    bool32       toggled;

    vec3_t       position;
    vec2_t       render_size;
    vec4_t       render_color;

    rectangle2_t widget_rect;
};

enum ui_signal_flags_t
{
    UI_SIGNAL_FLAG_INVALID               = BIT(0),

    // NOTE(Sleepster): Pressed this frame 
    UI_SIGNAL_FLAG_LEFT_CLICKED          = BIT(1),
    UI_SIGNAL_FLAG_RIGHT_CLICKED         = BIT(2),
    UI_SIGNAL_FLAG_MIDDLE_CLICKED        = BIT(3),

    // NOTE(Sleepster): Released this frame 
    UI_SIGNAL_FLAG_LEFT_RELEASED         = BIT(4),
    UI_SIGNAL_FLAG_RIGHT_RELEASED        = BIT(5),
    UI_SIGNAL_FLAG_MIDDLE_RELEASED       = BIT(6),

    UI_SIGNAL_FLAG_LEFT_DOUBLE_CLICKED   = BIT(7),
    UI_SIGNAL_FLAG_RIGHT_DOUBLE_CLICKED  = BIT(8),
    UI_SIGNAL_FLAG_MIDDLE_DOUBLE_CLICKED = BIT(9),

    UI_SIGNAL_FLAG_LEFT_DRAGGING         = BIT(10),
    UI_SIGNAL_FLAG_RIGHT_DRAGGING        = BIT(11),
    UI_SIGNAL_FLAG_MIDDLE_DRAGGING       = BIT(12),

    UI_SIGNAL_FLAG_OUTSIDE_BOUNDS        = BIT(13),
    UI_SIGNAL_FLAG_HOVERING              = BIT(14),

    UI_SIGNAL_FLAG_CLICKED               = UI_SIGNAL_FLAG_LEFT_CLICKED,
    UI_SIGNAL_FLAG_RELEASED              = UI_SIGNAL_FLAG_LEFT_RELEASED,
    UI_SIGNAL_FLAG_DOUBLE_CLICKED        = UI_SIGNAL_FLAG_LEFT_DOUBLE_CLICKED,
};

struct ui_signal_t 
{
    widget_t       *widget;
    u32             signal_flags;
};

#define ui_hovered(signal)        ((signal).signal_flags & UI_SIGNAL_FLAG_HOVERING)
#define ui_pressed(signal)        ((signal).signal_flags & UI_SIGNAL_FLAG_CLICKED)
#define ui_down(signal)           ((signal).signal_flags & UI_SIGNAL_FLAG_LEFT_DRAGGING)
#define ui_released(signal)       ((signal).signal_flags & UI_SIGNAL_FLAG_RELEASED)

enum widget_flags_t
{
    UI_WIDGET_FLAG_INVALID          = BIT(0),
    UI_WIDGET_FLAG_IDLE_COLOR       = BIT(1),
    UI_WIDGET_FLAG_HOVER_COLOR      = BIT(2),
    UI_WIDGET_FLAG_ACTIVE_COLOR     = BIT(3),
    UI_WIDGET_FLAG_HAS_TEXT         = BIT(4),
    UI_WIDGET_FLAG_MOUSE_CLICKABLE  = BIT(5),
    UI_WIDGET_FLAG_HOVERABLE        = BIT(6),

    UI_WIDGET_FLAG_CLICKABLE_BUTTON = UI_WIDGET_FLAG_HOVERABLE|UI_WIDGET_FLAG_HOVER_COLOR|UI_WIDGET_FLAG_MOUSE_CLICKABLE|UI_WIDGET_FLAG_ACTIVE_COLOR,
    UI_WIDGET_FLAG_LABLED_BUTTON    = UI_WIDGET_FLAG_CLICKABLE_BUTTON|UI_WIDGET_FLAG_HAS_TEXT
};

enum widget_layout_style_t
{
    WIDGET_LAYOUT_STYLE_VERTICAL,
    WIDGET_LAYOUT_STYLE_HORIZONTAL
};

struct widget_t
{
    u64             ID;
    u32             widget_flags;
    u32             layout_style;
    widget_state_t *state;

    string_t        widget_text;
    bool32          toggled;
    float32         parent_stack_depth;
    u32             font_size;
    
    vec3_t          expected_position;
    vec2_t          minimum_render_size;

    // TODO(Sleepster): Merge these into a "ui_theme_t" structure?
    vec4_t          idle_color;
    vec4_t          hovered_color;
    vec4_t          active_color;
#if 0
    u32             max_child_width;
    u32             max_child_height;
    u32             total_child_width;
    u32             total_child_height;
#endif

    // NOTE(Sleepster): 
    // If this is a tree... 
    widget_t       *parent;
    widget_t       *first_child;
    widget_t       *last_child;

    // NOTE(Sleepster): 
    // Chaining upon a linked list... 
    widget_t       *next_sibling;
    widget_t       *prev_sibling;
};

struct ui_state_t
{
    // NOTE(Sleepster): Lasts one frame...
    memory_arena_t                    widget_arena;

    u64                               hot_widget_ID;
    u64                               active_widget_ID;

    // NOTE(Sleepster): Persists between frames... 
    memory_arena_t                    persistent_data_arena;
    HashTable_t(widget_state_t)       widget_states;

    renderer_state_t                 *renderer;
    asset_manager_t                  *asset_manager;
    input_manager_t                  *input_manager;
    uniform_constant_buffer_t        *camera_matrices_buffer;

    camera_matrices_t                 current_camera;
    input_controller_t               *ui_controller;

    // TODO(Sleepster): Merge these into a "ui_theme_t" structure?
    asset_handle_t                    widget_shader;
    asset_handle_t                    font_shader;
    asset_handle_t                    default_font;
    u32                               default_font_size;
    vec4_t                            default_font_color;

    u32                               current_font_size;
    vec4_t                            current_font_color;

    vec2_t                            mouse_position;
    u32                               widget_count;
    u64                               frame_count;
    u64                               ui_seed;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *parent_stack[MAX_PARENT_WIDGETS];
    u32                               parent_stack_top;

    u32                               interface_framebuffer;
    vertex_buffer_t                   vertex_buffer;
    render_buffer_t                   index_buffer;
};

void      ui_state_init(ui_state_t *ui_state, input_manager_t *input_manager, asset_manager_t *asset_manager, renderer_state_t *renderer_state, u32 renderpass_ID);
void      ui_state_update_widget_state(ui_state_t *ui_state);
void      ui_state_render_widgets(ui_state_t *ui_state, render_command_list_t *command_list);

true_inline void ui_state_begin_frame(ui_state_t *ui_state);
true_inline void ui_state_end_frame(ui_state_t *ui_state, render_command_list_t *command_list);
true_inline void ui_widget_set_parent_layout(ui_state_t *ui_state, u32 layout_style);
true_inline void ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget);
true_inline void ui_widget_pop_parent(ui_state_t *ui_state);
true_inline void ui_widget_seed(ui_state_t *ui_state, u64 index);
true_inline void ui_widget_set_font_color(ui_state_t *ui_state, vec4_t color);
true_inline void ui_widget_set_current_font_size(ui_state_t *ui_state, u32 font_size);

widget_t*   ui_widget_create(ui_state_t *ui_state, string_t widget_name, u32 widget_flags);
ui_signal_t ui_widget_panel(ui_state_t *ui_state,  string_t widget_name, vec2_t position, vec4_t background_color);
ui_signal_t ui_widget_button(ui_state_t *ui_state, string_t widget_name, vec2_t minimum_size, vec4_t idle_color, vec4_t hovered_color, vec4_t active_color);
ui_signal_t ui_widget_text(ui_state_t *ui_state, string_t widget_text, vec4_t text_color);

#endif // S_UI_CORE_H

