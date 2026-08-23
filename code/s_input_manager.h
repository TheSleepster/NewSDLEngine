#if !defined(S_INPUT_MANAGER_H)
/* ========================================================================
   $File: s_input_manager.h $
   $Date: December 06 2025 09:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_INPUT_MANAGER_H
#include <SDL3/SDL.h>

#include <c_base.h>
#include <c_types.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_log.h>
#include <c_math.h>

extern vec2_t g_window_size;

#define MAX_INPUT_CONTROLLERS 4
#define MAX_KEYBOARD_BUTTONS (SDL_SCANCODE_COUNT + 5)
#define MAX_BUFFERED_INPUTS  32

// TEXT INPUT INFORMATION

typedef enum text_input_action_event_type 
{
    TEXT_INPUT_EVENT_NONE,
    TEXT_INPUT_EVENT_PRESSED  = BIT(1),
    TEXT_INPUT_EVENT_DOWN     = BIT(2),
    TEXT_INPUT_EVENT_RELEASED = BIT(3),
}text_input_action_type_t;

typedef enum text_input_modifier_flags 
{
    TEXT_INPUT_MODIFIER_NONE  = BIT(0),
    TEXT_INPUT_MODIFIER_SHIFT = BIT(1),
    TEXT_INPUT_MODIFIER_CTRL  = BIT(2),
    TEXT_INPUT_MODIFIER_ALT   = BIT(3),
}text_input_modifier_flags;

typedef enum text_input_event_type
{
    // NOTE(Sleepster): For messages like WM_CHAR for a character string that has been typed,
    // Already in UTF8 encoding.
    TEXT_INPUT_EVENT_TYPE_CHARACTER_STREAM = BIT(0),
    // NOTE(Sleepster): For events like CTRL being pressed, Shift down, etc. 
    TEXT_INPUT_EVENT_TYPE_INPUT_EVENT      = BIT(1),
}text_input_event_type_t;

typedef struct text_input_event
{
    u32       type;
    u32       input_event_type;
    u32       modifier_flags;
    u32       scancode;
    u32       keycode;

    const u8 *input_stream;
}text_input_event_t;

// TEXT INPUT INFORMATION

typedef enum action_button_flags
{
    INPUT_MANAGER_ACTION_BUTTON_FLAG_NONE     = 0,
    INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED  = BIT(1),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN     = BIT(2),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED = BIT(3),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_CONSUMED = BIT(4),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_COUNT,
}action_button_flags_t;

typedef enum input_mouse_buttons
{
    SDL_LEFT_MOUSE         = 513,
    SDL_MIDDLE_MOUSE       = 514,
    SDL_RIGHT_MOUSE        = 515,
    SDL_X1_MOUSE           = 516,
    SDL_X2_MOUSE           = 517,
    SDL_MOUSE_BUTTON_COUNT = 5
}input_mouse_buttons_t;

typedef struct action_button
{
    // NOTE(Sleepster): UTF32 keycode 
    u32   keycode;
    u32   scancode;
    u32   flags;
    s16   analog_value;
    u8    half_transition_counter;
}action_button_t;

typedef struct keyboard_controller_data
{
    action_button_t  input[MAX_KEYBOARD_BUTTONS + SDL_MOUSE_BUTTON_COUNT];

    vec2_t           current_mouse_pos;
    vec2_t           last_mouse_pos;
    vec2_t           mouse_delta;

    bool8            is_shift_key_down;
    bool8            is_control_key_down;
    bool8            is_alt_key_down;
}keyboard_controller_data_t;

typedef struct gamepad_controller_data
{
    SDL_Gamepad    *gamepad_data;
    SDL_Joystick   *stick_data;
    u32             gamepad_id;

    bool8           has_rumble;
    s32             rumble_value;
    float32         stick_deadzone;

    action_button_t buttons[SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT];
}gamepad_controller_data_t;

typedef enum controller_type
{
    IM_CONTROLLER_INVALID,
    IM_CONTROLLER_GAMEPAD,
    IM_CONTROLLER_KEYBOARD,
    IM_CONTROLLER_COUNT,
}controller_type_t;

typedef struct input_controller
{
    bool8             is_valid;
    bool8             is_analog;
    controller_type_t type;
    u32               ID;
    
    action_button_t *transient_action_inputs[MAX_BUFFERED_INPUTS];
    u32              action_inputs_this_frame;

    text_input_event_t transient_text_inputs[MAX_BUFFERED_INPUTS];
    u32                text_inputs_this_frame;

    union {
        keyboard_controller_data_t keyboard;
        gamepad_controller_data_t  gamepad;
    };
}input_controller_t;

// GAME ACTIONS

// NOTE(Sleepster):
// For Axis2Ds we assume that for controller map bindings:
//
// index 0: Up + Down
// index 1: Left + Right
//
// And for keyboards:
// index 0: Up
// index 1: Down
// index 2: Left
// index 3: Right
//
// While for Axis1Ds:
//
// controller:
//  index 0: Left + Right
//  
// keyboard:
//  index 0: Left
//  index 1: Right
//
// and buttons:
//
// index 0: button

constexpr s32 MAX_GAME_ACTION_BINDINGS = 4;
constexpr s32 MAX_GAME_ACTION_MAPPINGS = 4;

constexpr float32 INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE = 0.20f; 

typedef enum game_action_mapping_type
{
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_INVALID,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS1D,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS2D,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_COUNT
}game_action_mapping_type_t;

typedef enum game_action_binding_type
{
    INPUT_MANAGER_BINDING_TYPE_BUTTON,
    INPUT_MANAGER_BINDING_TYPE_JOYSTICK,
}game_action_binding_type_t;

typedef struct game_action_binding
{
    u32 binding_id;
    u32 binding_type;
}game_action_binding_t;

typedef struct game_action_mapping
{
    game_action_binding_t bindings[MAX_GAME_ACTION_BINDINGS];
    s32                   binding_count;
    u32                   controller_type;
}game_action_mapping_t;

typedef struct game_action
{
    u32                   action_binding_type;
    game_action_mapping_t mappings[MAX_GAME_ACTION_MAPPINGS];
    s32                   mapping_count;

    string_t              name;
    // TODO(Sleepster): Maybe we want to cache the values related to the input axis and such?
    // vec2_t  axis2D_value;
    // float32 axis1D_value;
    // s32     button_flags;
}game_action_t;

// GAME ACTIONS

// NOTE(Sleepster): Hey, the keyboard connected to the PC is ALWAYS the primary device 
typedef struct input_manager
{
    keyboard_controller_data_t keyboard_data;
    gamepad_controller_data_t  gamepad_data;
    
    u32                        primary_controller_index;
    u32                        active_controller_index;
    u32                        connected_controller_count;
    input_controller_t         controllers[MAX_INPUT_CONTROLLERS];

    dynarray_t<game_action_t>  game_actions;
}input_manager_t;

/*===========================================
  =============== GENERAL API ===============
  ===========================================*/
void                s_im_init_input_manager(input_manager_t *input_manager);
void                s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager);
void                s_im_reset_controller_states(input_manager_t *input_manager);
void                s_im_initialize_keyboard_controller(input_controller_t *controller, u32 ID);
input_controller_t* s_im_get_primary_controller(input_manager_t *input_manager);
input_controller_t* s_im_get_controller_at_index(input_manager_t *input_manager, s32 index);
input_controller_t* s_im_get_active_controller(input_manager_t *input_manager);
void                s_im_set_active_controller(input_manager_t *input_manager, u32 controller_index);
void                s_im_set_primary_controller(input_manager_t *input_manager, u32 controller_index);
bool8               s_im_is_shift_key_down(input_controller_t *controller);
bool8               s_im_is_control_key_down(input_controller_t *controller);
bool8               s_im_is_alt_key_down(input_controller_t *controller);

/*==============================================
  =============== KEYBOARD INPUT ===============
  ==============================================*/
vec2_t s_im_transform_mouse_data(input_controller_t *controller, vec2_t surface_size, mat4_t view_matrix, mat4_t projection_matrix);

bool8  s_im_is_keyboard_key_pressed(input_controller_t *controller, s32 key_index);
bool8  s_im_is_keyboard_key_down(input_controller_t *controller, s32 key_index);
bool8  s_im_is_keyboard_key_released(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_press(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_down(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_release(input_controller_t *controller, s32 key_index);

action_button_t* s_im_get_key_state(input_controller_t *controller, s32 key_index);

/*=============================================
  =============== GAMEPAD INPUT ===============
  =============================================*/
bool8  s_im_is_gamepad_button_pressed(input_controller_t *controller, s32 button_index);
bool8  s_im_is_gamepad_button_down(input_controller_t *controller, s32 button_index);
bool8  s_im_is_gamepad_button_released(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_press(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_down(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_release(input_controller_t *controller, s32 button_index);

action_button_t* s_im_gamepad_get_button_state(input_controller_t *controller, s32 button_index);

/*===========================================
  ============= GAME ACTION API =============
  ===========================================*/
game_action_t *s_im_game_action_create(input_manager_t *input_manager, string_t action_name, game_action_mapping_type_t mapping_type);
void           s_im_game_action_add_mapping(game_action_t *action, game_action_mapping_t *mapping);
void           s_im_game_action_reset_mappings(game_action_t *action);
s32            s_im_game_action_read_button_state(input_controller_t *controller, game_action_t *action);
float32        s_im_game_action_read_axis1D_value(input_controller_t *controller, game_action_t *action);
vec2_t         s_im_game_action_read_axis2D_value(input_controller_t *controller, game_action_t *action);

#define GameActionPressed(flags)  ((flags) & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED)
#define GameActionDown(flags)     ((flags) & INPUT_MANAGER_ACTION_BUTTON_DOWN)
#define GameActionReleased(flags) ((flags) & INPUT_MANAGER_ACTION_BUTTON_RELEASED)

#endif // S_INPUT_MANAGER_H

