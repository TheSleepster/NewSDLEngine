#if !defined(S_INPUT_MANAGER_H)
/* ========================================================================
   $File: s_input_manager.h $
   $Date: December 06 2025 09:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_INPUT_MANAGER_H
#include <SDL3/SDL.h>
#include <c_dynarray.h>
#include <stdio.h>

struct input_event_t;

// NOTE(Sleepster): Game Actions 
constexpr s32 MAX_GAME_ACTION_BINDINGS = 4;
constexpr s32 MAX_GAME_ACTION_MAPPINGS = 4;

enum game_action_mapping_type_t
{
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_INVALID,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS1D,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS2D,
    INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_COUNT
};

enum game_action_binding_type_t
{
    INPUT_MANAGER_BINDING_TYPE_BUTTON,
    INPUT_MANAGER_BINDING_TYPE_JOYSTICK,
};

struct input_binding_state_t
{
    s32 flags;
    s32 half_transition_count;
    s32 ID;
};

struct game_action_binding_t
{
    s32 bindingID;
    s32 binding_type;
};

struct game_action_mapping_t
{
    game_action_binding_t bindings[MAX_GAME_ACTION_BINDINGS];
    s32                   binding_count;
    s32                   controller_type;
};

struct game_action_t
{
    u32                   action_binding_type;
    game_action_mapping_t mappings[MAX_GAME_ACTION_MAPPINGS];
    s32                   mapping_count;

    string_t              name;
    vec2_t                axis2D_value;
    float32               axis1D_value;
    s32                   button_flags;
};


// NOTE(Sleepster): Input Controllers 
constexpr u32     MAX_KEYBOARD_BUTTONS                   = SDL_SCANCODE_COUNT + 5;
constexpr float32 INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE = 0.20f; 

enum action_button_flags_t
{
    INPUT_MANAGER_ACTION_BUTTON_FLAG_NONE     = 0,
    INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED  = BIT(1),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN     = BIT(2),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED = BIT(3),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_CONSUMED = BIT(4),
    INPUT_MANAGER_ACTION_BUTTON_FLAG_COUNT,
};

enum input_mouse_buttons_t
{
    SDL_LEFT_MOUSE         = 513,
    SDL_MIDDLE_MOUSE       = 514,
    SDL_RIGHT_MOUSE        = 515,
    SDL_X1_MOUSE           = 516,
    SDL_X2_MOUSE           = 517,
    SDL_MOUSE_BUTTON_COUNT = 5
};

enum keyboard_modifier_flags 
{
    KEYBOARD_MODIFIER_NONE   = BIT(0),
    KEYBOARD_MODIFIER_LALT   = BIT(1),
    KEYBOARD_MODIFIER_LCTRL  = BIT(2),
    KEYBOARD_MODIFIER_LSHIFT = BIT(3),
    KEYBOARD_MODIFIER_RALT   = BIT(4),
    KEYBOARD_MODIFIER_RCTRL  = BIT(5),
    KEYBOARD_MODIFIER_RSHIFT = BIT(6),
};

struct action_button_t
{
    // NOTE(Sleepster): UTF32 keycode 
    u32    keycode;
    u32    scancode;
    u32    flags;
    // NOTE(Sleepster): 1D analog values only occupy the .x member. 
    vec2_t analog_value;
    s16    half_transition_count;
};

struct keyboard_controller_data_t
{
    action_button_t  input[MAX_KEYBOARD_BUTTONS];
    u32              modifier_flags;

    vec2_t           current_mouse_pos;
    vec2_t           last_mouse_pos;
    vec2_t           mouse_delta;

    vec2_t           mouse_wheel_delta;
    vec2_t           current_mouse_wheel;
};

struct gamepad_controller_data_t
{
    SDL_Gamepad    *gamepad_data;
    u32             gamepadID;

    bool8           has_rumble;
    s32             rumble_value;
    float32         stick_deadzone;

    action_button_t buttons[SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT];
};

// NOTE(Sleepster): Input Events 
constexpr s32 MAX_INPUT_EVENTS = 100;
constexpr u32 MAX_INPUT_CONTROLLERS = 4;

enum input_event_type_t
{
    INPUT_EVENT_TYPE_PRESSED,
    INPUT_EVENT_TYPE_DOWN,
    INPUT_EVENT_TYPE_RELEASED,
    INPUT_EVENT_TYPE_AXIS_MOVED,
    INPUT_EVENT_TYPE_TEXT_INPUT,
};

enum input_controller_type_t
{
    INPUT_CONTROLLER_TYPE_INVALID,
    INPUT_CONTROLLER_TYPE_KEYBOARD,
    INPUT_CONTROLLER_TYPE_GAMEPAD,
};

enum input_axis_t
{
    // NOTE(Sleepster): Gamepad 
    INPUT_AXIS_GAMEPAD_LEFT_X,
    INPUT_AXIS_GAMEPAD_LEFT_Y,
    INPUT_AXIS_GAMEPAD_RIGHT_X,
    INPUT_AXIS_GAMEPAD_RIGHT_Y,
    INPUT_AXIS_GAMEPAD_LEFT_TRIGGER,
    INPUT_AXIS_GAMEPAD_RIGHT_TRIGGER,

    // NOTE(Sleepster): Mouse 
    INPUT_AXIS_MOUSE,
    INPUT_AXIS_MOUSE_WHEEL,
};

// NOTE(Sleepster): -1 on any of the ID's means that it is "Invalid" 
struct input_event_t
{
    s32      type;
    s32      input_type;
    bool32   consumed;

    s32      inputID;      // key / gamepad button
    s32      controllerID; // owner controller
    u64      timestampMS;

    string_t input_stream;
    vec2_t   axis_value;
};

// NOTE(Sleepster): I don't want this here... but the C++ compiler is too stupid to see it in the "Input Controllers" section 
struct input_controller_t
{
    s32 type;
    s32 ID;
    s32 controller_index;

    s32                                      event_count;
    array_t<input_event_t, MAX_INPUT_EVENTS> events;
    union {
        gamepad_controller_data_t  gamepad;
        keyboard_controller_data_t keyboard;
    };
};

// TODO(Sleepster): 
struct input_manager_t 
{
    s32 event_count;
    s32 connected_controller_count;
    s32 active_controller_index;

    dynarray_t<game_action_t>                          game_actions;
    array_t<input_controller_t, MAX_INPUT_CONTROLLERS> controllers;
    array_t<input_event_t,      MAX_INPUT_EVENTS>      events;
};

#define GameActionPressed(action)  ((action->button_flags) & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED)
#define GameActionDown(action)     ((action->button_flags) & INPUT_MANAGER_ACTION_BUTTON_DOWN)
#define GameActionReleased(action) ((action->button_flags) & INPUT_MANAGER_ACTION_BUTTON_RELEASED)

// INTERFACE
void s_im_init_input_manager(input_manager_t *input_manager);
void s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager);
void s_im_clear_controller_events(input_controller_t *controller);

input_controller_t* s_im_find_controller_by_ID(input_manager_t *input_manager, s32 ID, s32 *index_out);
input_controller_t* s_im_find_first_gamepad_controller(input_manager_t *input_manager, s32 *index_out);
input_controller_t* s_im_find_first_keyboard_controller(input_manager_t *input_manager, s32 *index_out);
vec2_t              s_im_transform_mouse_data(input_controller_t *controller, vec2_t surface_size, mat4_t view_matrix, mat4_t projection_matrix);

input_binding_state_t s_im_get_button_binding_state(input_controller_t *controller, game_action_binding_t *binding);
float32               s_im_get_axis_value(input_controller_t *controller, game_action_binding_t *binding);

game_action_t* s_im_game_action_create(input_manager_t *input_manager, string_t action_name, game_action_mapping_type_t mapping_type);
void           s_im_game_action_add_mapping(game_action_t *action, game_action_mapping_t *mapping);
void           s_im_game_action_reset_mappings(game_action_t *action);
void           s_im_game_action_process_button_state(input_controller_t *controller, game_action_t *action);
void           s_im_game_action_process_axis1D_state(input_controller_t *controller, game_action_t *action);
void           s_im_game_action_process_axis2D_state(input_controller_t *controller, game_action_t *action);

void s_im_update_game_action_states(input_manager_t *input_manager, input_controller_t *controller);
action_button_t* s_im_get_controller_action_button(input_controller_t *controller, s32 inputID);

#endif // S_INPUT_MANAGER_H

