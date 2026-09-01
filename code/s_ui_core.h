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

#include <r_immediate_rendering.h>
#include <s_RHI_core.h>

constexpr u32 MAX_PARENT_WIDGETS = 256;
constexpr u32 MAX_WIDGETS        = 1024;
constexpr u32 MAX_WIDGET_LAYERS  = 32;

struct widget_t;

struct widget_state_t
{
    u32          last_interacted_frame;
    bool8        toggled;

    bool8        dragging;
    bool8        input_begin_within_bounds;
    bool8        is_held;
    bool8        just_released;
    bool8        just_clicked;
    bool8        is_double_clicked;
    bool8        is_right_clicked;
    bool8        is_right_held;

    // NOTE(Sleepster): For rendering SECTIONS of a string 
    s32          widget_text_render_start_offset;
    s32          widget_text_render_end_offset;

    // NOTE(Sleepster): Always between 0.0 and 1.0
    float32      slider_value;

    // NOTE(Sleepster): Offset is used for dragged widgets... 
    vec3_t       position;
    vec2_t       offset;
    vec2_t       initial_mouse_position;

    vec2_t       render_size;
    // NOTE(Sleepster): The difference between this and the widget's minimum_render_size
    // is that this is meant to be persistent and accounts for ALL widgets contained within as well as padding
    // Essentially the widget's "minimum_render_size" is meant to be for the layout engine, whereas this is 
    // "the smallest size reasonably possible" for actions that modify the size of the widget (such as resizing)
    vec2_t       absolute_minimum_render_size;
    vec2_t       resize_amount;
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

    UI_SIGNAL_FLAG_LEFT_DOWN             = BIT(10),
    UI_SIGNAL_FLAG_RIGHT_DOWN            = BIT(11),
    UI_SIGNAL_FLAG_MIDDLE_DOWN           = BIT(12),

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

#define ui_hovered(signal)          ((signal).signal_flags & UI_SIGNAL_FLAG_HOVERING)
#define ui_pressed(signal)          ((signal).signal_flags & UI_SIGNAL_FLAG_CLICKED)
#define ui_down(signal)             ((signal).signal_flags & UI_SIGNAL_FLAG_LEFT_DOWN)
#define ui_released(signal)         ((signal).signal_flags & UI_SIGNAL_FLAG_RELEASED)
#define ui_dragging(signal)         ((signal).signal_flags & UI_SIGNAL_FLAG_LEFT_DOWN)
#define ui_right_mouse_down(signal) ((signal).signal_flags & UI_SIGNAL_FLAG_RIGHT_DOWN)

// NOTE(Sleepster): If you change the values here, make sure they're inline with what's in the 
// widget.slh file.
enum widget_flags_t
{
    UI_WIDGET_FLAG_INVALID           = BIT(0),
    UI_WIDGET_FLAG_IDLE_COLOR        = BIT(1),
    UI_WIDGET_FLAG_HOVER_COLOR       = BIT(2),
    UI_WIDGET_FLAG_ACTIVE_COLOR      = BIT(3),
    UI_WIDGET_FLAG_INTERACTABLE      = BIT(4),
    UI_WIDGET_FLAG_MOUSE_CLICKABLE   = BIT(5),
    UI_WIDGET_FLAG_HOVERABLE         = BIT(6),
    UI_WIDGET_FLAG_LEFT_DRAGGABLE    = BIT(7),

    UI_WIDGET_FLAG_DRAW_TEXT         = BIT(8),
    UI_WIDGET_FLAG_DRAW_RECTANGLE    = BIT(9),
    UI_WIDGET_FLAG_DRAW_BACKGROUND   = BIT(10),
    UI_WIDGET_FLAG_DRAW_BORDER       = BIT(11),
    UI_WIDGET_FLAG_MAKE_CIRCULAR     = BIT(12),
    UI_WIDGET_FLAG_FIXED_SIZE        = BIT(13),
    UI_WIDGET_FLAG_HAS_TEXT_CONTENT  = BIT(14),
    UI_WIDGET_FLAG_RESIZEABLE        = BIT(15),
    UI_WIDGET_FLAG_DISPLAY_TEXTURE   = BIT(16),
    UI_WIDGET_FLAG_STANDARD_RECTANGLE_BUTTON = UI_WIDGET_FLAG_IDLE_COLOR|UI_WIDGET_FLAG_HOVER_COLOR|UI_WIDGET_FLAG_ACTIVE_COLOR|UI_WIDGET_FLAG_MOUSE_CLICKABLE|UI_WIDGET_FLAG_HOVERABLE|UI_WIDGET_FLAG_DRAW_RECTANGLE|UI_WIDGET_FLAG_INTERACTABLE
};

enum widget_layout_style_t
{
    UI_WIDGET_LAYOUT_STYLE_VERTICAL,
    UI_WIDGET_LAYOUT_STYLE_HORIZONTAL
};

enum widget_size_kind_t
{
    UI_WIDGET_SIZE_KIND_PIXELS,
    UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT,
};

struct widget_t
{
    u64                ID;
    u32                widget_flags;
    u32                layout_style;
    widget_state_t    *state;

    string_t           widget_name;
    string_t          *widget_text_buffer;
    bool32             toggled;

    float32            parent_stack_depth;
    u32                font_size;
    
    vec3_t             expected_position;
    vec2_t             minimum_render_size;
    vec2_t             resize_value;
    ivec2_t            size_kind;

    // NOTE(Sleepster): Offset inside the parent, accounting for padding 
    vec2_t             parent_child_spacing;
    vec4_t             parent_padding;
    
    // NOTE(Sleepster): Left, Right, Top, Bottom 
    vec4_t             widget_padding;
    vec4_t             max_widget_padding;

    // NOTE(Sleepster): X and Y spacing... 
    vec2_t             child_spacing;

    rectangle2_t       scissor_rect;

    // TODO(Sleepster): Merge these into a "ui_theme_t" structure?
    vec4_t             idle_color;
    vec4_t             hovered_color;
    vec4_t             active_color;
    vec4_t             border_color;
    float32            smoothness;
    float32            radius;
    float32            border_thickness;

    asset_handle_t    *display_texture;
    vec2_t             display_texture_uvmin;
    vec2_t             display_texture_uvmax;

    // NOTE(Sleepster): 
    // If this is a tree... 
    widget_t          *parent;
    widget_t          *first_child;
    widget_t          *last_child;

    // NOTE(Sleepster): 
    // Chaining upon a linked list... 
    widget_t          *next_sibling;
    widget_t          *prev_sibling;

    immediate_widget_data_t *widget_instance_data; 
};

enum ui_keyboard_flags_t
{
    UI_KEYBOARD_FLAG_NONE  = BIT(0),
    UI_KEYBOARD_FLAG_LCTRL = BIT(1)
};

struct ui_state_t
{
    // NOTE(Sleepster): Lasts one frame...
    memory_arena_t                    widget_arena;
    memory_arena_t                    polling_arena;
    u32                               section_count;
    u32                               keyboard_flags;

    bool8                             frame_begun;
    bool8                             frame_ended;
    // NOTE(Sleepster): For collecting and handling input to things like textboxes. 
    bool8                             input_focused;
    bool8                             left_mouse_clicked_this_frame;

    u64                               last_hot_ID;
    u64                               last_active_ID;
    widget_t                         *hot_widget;
    widget_t                         *active_widget;
    widget_t                         *last_clicked_widget;

    input_binding_state_t             left_mouse;
    input_binding_state_t             right_mouse;

    array_t<input_event_t, 100>       ui_events;
    u32                               ui_event_count;

    vec2_t                            mouse_position;
    vec2_t                            mouse_delta;

    // NOTE(Sleepster): Persists between frames... 
    memory_arena_t                    persistent_data_arena;
    hash_table_t<widget_state_t>      widget_states;

    RHI_context_t                    *RHI_context;
    asset_manager_t                  *asset_manager;
    input_manager_t                  *input_manager;

    camera_matrices_t                 current_camera;
    input_controller_t               *ui_controller;

    // TODO(Sleepster): Merge these into a "ui_theme_t" structure?
    asset_handle_t                    widget_shader;
    asset_handle_t                    default_font;
    u32                               default_font_size;
    vec4_t                            default_font_color;
    vec4_t                            default_widget_idle_color;
    vec4_t                            default_widget_hover_color;
    vec4_t                            default_widget_active_color;
    vec4_t                            default_widget_border_color;
    float32                           default_widget_SDF_smoothness;
    float32                           default_widget_border_thickness;

    vec2_t                            max_widget_size;
    vec4_t                            active_widget_padding;
    u32                               widget_item_count;
    u64                               frame_count;
    u64                               ui_seed;

    widget_t                         *first_widget;
    widget_t                         *last_widget;

    widget_t                         *parent_stack[MAX_PARENT_WIDGETS];
    u32                               parent_stack_top;
    u32                               interface_framebuffer;

    RHI_vertex_buffer_t               vertex_buffer;
    RHI_index_buffer_t                index_buffer;

    immediate_widget_data_t          *widget_instances;
    u32                               widget_instance_count;

    RHI_uniform_constant_buffer_t    *widget_instance_data;
    RHI_uniform_constant_buffer_t    *camera_matrices_buffer;
};

void             ui_state_init(ui_state_t *ui_state, input_manager_t *input_manager, asset_manager_t *asset_manager, RHI_context_t *RHI_context, u32 renderpass_ID);
void             ui_state_poll_input_events(ui_state_t *ui_state);

true_inline void ui_state_begin_frame(ui_state_t *ui_state);
true_inline void ui_state_end_frame(ui_state_t *ui_state, RHI_command_list_t *command_list);

void      ui_state_update_widget_state(ui_state_t *ui_state);
void      ui_state_render_widgets(ui_state_t *ui_state, RHI_command_list_t *command_list);

true_inline void ui_state_set_default_widget_idle_color(ui_state_t *ui_state, vec4_t color);
true_inline void ui_state_set_default_widget_hover_color(ui_state_t *ui_state, vec4_t color);
true_inline void ui_state_set_default_widget_active_color(ui_state_t *ui_state, vec4_t color);
true_inline void ui_widget_set_default_font_color(ui_state_t *ui_state, vec4_t color);
true_inline void ui_widget_set_default_font_size(ui_state_t *ui_state, u32 font_size);
true_inline void ui_state_set_active_padding(ui_state_t *ui_state, vec4_t padding);
true_inline void ui_widget_set_flags(widget_t *widget, u32 flags);

true_inline void ui_state_set_parent_layout(ui_state_t *ui_state, u32 layout_style);

true_inline void ui_widget_set_layout(widget_t *widget, u32 layout_style);
true_inline void ui_widget_push_parent(ui_state_t *ui_state, widget_t *widget);
true_inline void ui_widget_pop_parent(ui_state_t *ui_state);
true_inline void ui_widget_set_padding(widget_t *widget, vec4_t padding);
true_inline void ui_widget_seed(ui_state_t *ui_state, u64 index);

widget_t*   ui_widget_create(ui_state_t *ui_state, string_t widget_name, u32 widget_flags);
ui_signal_t ui_widget_panel(ui_state_t *ui_state, string_t widget_name, vec2_t position, vec2_t minimum_render_size, vec2_t child_spacing, vec4_t padding, vec4_t background_color, u32 additional_flags);
ui_signal_t ui_widget_draggable_panel(ui_state_t *ui_state, string_t widget_name, vec2_t position, vec2_t minimum_render_size, vec2_t child_spacing, vec4_t padding, vec4_t background_color, u32 additional_flags);
ui_signal_t ui_widget_sized_button(ui_state_t *ui_state, string_t widget_name, vec2_t minimum_size, u32 widget_flags);
ui_signal_t ui_widget_text(ui_state_t *ui_state, string_t widget_text);
ui_signal_t ui_widget_labeled_button(ui_state_t *ui_state, string_t widget_text);
ui_signal_t ui_widget_float_slider_bar(ui_state_t *ui_state, string_t widget_name, u32 bar_width, u32 bar_height, float32 button_scale_factor);
void        ui_widget_spacer(ui_state_t *ui_state, string_t widget_name, vec2_t spacing_size, ivec2_t size_kind);
ui_signal_t ui_widget_rectangle(ui_state_t *ui_state, string_t widget_name, vec2_t size, ivec2_t size_kind);
void        ui_widget_divider(ui_state_t *ui_state, string_t widget_name, vec2_t size, ivec2_t size_kind);
ui_signal_t ui_widget_textbox(ui_state_t *ui_state, string_t widget_name, string_t *widget_text_content, vec2_t size);
ui_signal_t ui_widget_texture(ui_state_t *ui_state, string_t widget_name, vec2_t size, asset_handle_t *texture, vec2_t uv_min, vec2_t uv_max, ivec2_t size_kind, u32 additional_flags);

true_inline void ui_state_begin_row(ui_state_t *ui_state, widget_t *parent);
true_inline void ui_state_end_row(ui_state_t *ui_state);
true_inline void ui_state_begin_column(ui_state_t *ui_state, widget_t *parent);
true_inline void ui_state_end_column(ui_state_t *ui_state);

#define ui_parent(state, widget)      DeferLoop(ui_widget_push_parent((state), (widget)), ui_widget_pop_parent((state)))
#define ui_row(state, parent)         DeferLoop(ui_state_begin_row((state), (parent)), ui_state_end_row((state)))
#define ui_column(state, parent)      DeferLoop(ui_state_begin_column((state), (parent)), ui_state_end_column((state)))
#define ui_frame(state, command_list) DeferLoop(ui_state_begin_frame((state)), ui_state_end_frame((state), (command_list)))

#endif // S_UI_CORE_H

