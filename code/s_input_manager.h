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

/* 
====================================================
INPUT EVENTS
==================================================== 
*/

struct input_controller_t;

enum input_event_type_t
{
    INPUT_EVENT_TYPE_NONE                = 0,
    INPUT_EVENT_TYPE_KEY_DOWN            = BIT(0),
    INPUT_EVENT_TYPE_KEY_UP              = BIT(1),
    INPUT_EVENT_TYPE_MOUSE_DOWN          = BIT(2),
    INPUT_EVENT_TYPE_MOUSE_UP            = BIT(3),
    INPUT_EVENT_TYPE_TEXT_INPUT          = BIT(4),
    INPUT_EVENT_TYPE_GAMEPAD_BUTTON_DOWN = BIT(5),
    INPUT_EVENT_TYPE_GAMEPAD_BUTTON_UP   = BIT(6),
};

struct input_event_t
{
    u32      type;
    bool8    consumed;
    u16      inputID; 
    u32      vkcode; 
    u64      timestamp;
    vec2_t   axis_value;
    string_t text;
};

ENGINE_API input_event_t *s_im_read_next_event_from_controller(input_controller_t *controller);

/* 
====================================================
INPUT STATE 
==================================================== 
*/

enum input_mouse_buttons_t
{
    SDL_SCANCODE_LEFT_MOUSE   = 513,
    SDL_SCANCODE_MIDDLE_MOUSE = 514,
    SDL_SCANCODE_RIGHT_MOUSE  = 515,
    SDL_SCANCODE_X1_MOUSE     = 516,
    SDL_SCANCODE_X2_MOUSE     = 517,
    SDL_MOUSE_BUTTON_COUNT    = 5
};

constexpr u32 INPUT_ARRAY_INPUT_AXIS_OFFSET = SDL_SCANCODE_COUNT + SDL_MOUSE_BUTTON_COUNT;
enum input_axis_value_t
{
    INPUT_ARRAY_INPUT_AXIS_GAMEPAD_LEFTX  = INPUT_ARRAY_INPUT_AXIS_OFFSET + 0,
    INPUT_ARRAY_INPUT_AXIS_GAMEPAD_LEFTY  = INPUT_ARRAY_INPUT_AXIS_OFFSET + 1,
    INPUT_ARRAY_INPUT_AXIS_GAMEPAD_RIGHTX = INPUT_ARRAY_INPUT_AXIS_OFFSET + 2,
    INPUT_ARRAY_INPUT_AXIS_GAMEPAD_RIGHTY = INPUT_ARRAY_INPUT_AXIS_OFFSET + 3,
    INPUT_ARRAY_INPUT_AXIS_LEFT_TRIGGER   = INPUT_ARRAY_INPUT_AXIS_OFFSET + 4,
    INPUT_ARRAY_INPUT_AXIS_RIGHT_TRIGGER  = INPUT_ARRAY_INPUT_AXIS_OFFSET + 5,
    INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT = INPUT_ARRAY_INPUT_AXIS_OFFSET + 6,
    INPUT_ARRAY_INPUT_AXIS_MOUSE_WHEEL    = INPUT_ARRAY_INPUT_AXIS_OFFSET + 7,
    INPUT_ARRAY_INPUT_AXIS_COUNT          = (SDL_GAMEPAD_AXIS_COUNT) + 2,
};

enum input_button_flags_t
{
    INPUT_MANAGER_INPUT_STATE_FLAG_NONE     = 0,
    INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED  = BIT(0),
    INPUT_MANAGER_INPUT_STATE_FLAG_DOWN     = BIT(1),
    INPUT_MANAGER_INPUT_STATE_FLAG_RELEASED = BIT(2),
    INPUT_MANAGER_INPUT_STATE_FLAG_CONSUMED = BIT(3),
    INPUT_MANAGER_INPUT_STATE_FLAG_COUNT,
};

enum keyboard_modifier_flags_t 
{
    KEYBOARD_MODIFIER_NONE   = BIT(0),
    KEYBOARD_MODIFIER_LALT   = BIT(1),
    KEYBOARD_MODIFIER_LCTRL  = BIT(2),
    KEYBOARD_MODIFIER_LSHIFT = BIT(3),
    KEYBOARD_MODIFIER_RALT   = BIT(4),
    KEYBOARD_MODIFIER_RCTRL  = BIT(5),
    KEYBOARD_MODIFIER_RSHIFT = BIT(6),
};

struct input_state_t
{
    u32    inputID;
    u32    vkcode; 
    u32    flags;
    u16    half_transition_count;

    vec2_t last_value;
    vec2_t current_value;
    vec2_t delta_value;
};

#define InputStatePressed(button_flags)  (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED)  != 0)
#define InputStateDown(button_flags)     (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_DOWN)     != 0)
#define InputStateReleased(button_flags) (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_RELEASED) != 0)

ENGINE_API u32            s_im_controller_get_input_state_flags(input_controller_t *controller, u32 inputID);
ENGINE_API input_state_t *s_im_controller_get_input_state(input_controller_t *controller, u32 inputID);
ENGINE_API bool8          s_im_is_input_button_pressed(input_controller_t *controller, u32 inputID);
ENGINE_API bool8          s_im_is_input_button_down(input_controller_t *controller, u32 inputID);
ENGINE_API bool8          s_im_is_input_button_released(input_controller_t *controller, u32 inputID);

/* 
====================================================
CONTROLLERS AND DEVICES
==================================================== 
*/

constexpr u32 MAX_INPUT_ARRAY_SIZE          = INPUT_ARRAY_INPUT_AXIS_OFFSET + INPUT_ARRAY_INPUT_AXIS_COUNT;
constexpr s32 MAX_INPUT_DEVICES             = 4;
constexpr s32 DEVICE_MAX_INPUT_EVENT_COUNT  = 256;
constexpr s32 MAX_TRACKED_TRANSIENT_BUTTONS = 20;

constexpr float32 DEFAULT_GAMEPAD_DEADZONE = 0.20f; 

struct input_device_t;

enum input_device_type_t
{
    INPUT_DEVICE_TYPE_INVALID,
    INPUT_DEVICE_TYPE_KEYBOARD,
    INPUT_DEVICE_TYPE_GAMEPAD
};

// NOTE(Sleepster): Using is dumb. I hate C++ but templates don't support typedef... 
using input_event_array_t      = fixed_array_t<input_event_t, DEVICE_MAX_INPUT_EVENT_COUNT>;
using transient_button_array_t = fixed_array_t<input_state_t*, MAX_TRACKED_TRANSIENT_BUTTONS>;
using input_state_array_t      = fixed_array_t<input_state_t, MAX_INPUT_ARRAY_SIZE>;

struct input_controller_t
{
    input_device_t      *owner_device;
    u64                  last_polled_timestamp;

    input_event_array_t  input_events;
    s32                  next_controller_event_to_write;
    s32                  next_controller_event_to_read;
    s32                  next_device_event_to_read;

    input_state_array_t inputs;
    struct {
        transient_button_array_t buttons_pressed;
        transient_button_array_t buttons_released;
        u32                      pressed_button_count;
        u32                      released_button_count;
    }transient_data;
};

using device_input_axis_array_t = fixed_array_t<input_state_t, INPUT_ARRAY_INPUT_AXIS_COUNT>;
struct input_device_t
{
    s32 deviceID;
    s32 type;

    input_event_array_t       input_events;
    s32                       next_event_to_write;

    device_input_axis_array_t input_axis_info_array;
    struct {
        u32 mouseID;
    }keyboard;

    struct {
        SDL_Gamepad *handle;
        float32      deadzone;
        bool8        has_rumble;
    }gamepad;
};

/* 
====================================================
GAME ACTIONS
==================================================== 
*/

enum input_action_type_t
{
    INPUT_ACTION_TYPE_INVALID,
    INPUT_ACTION_TYPE_BUTTON,
    INPUT_ACTION_TYPE_AXIS1D,
    INPUT_ACTION_TYPE_AXIS2D,
};

struct input_action_binding_t
{
    u32 inputID;
    u32 required_modifiers;
};

using input_action_binding_array_t = fixed_array_t<input_action_binding_t, 10>;
struct input_action_mapping_t
{
    s32                          device_type;
    s32                          binding_count;
    input_action_binding_array_t bindings;
};

using input_action_mapping_array_t = fixed_array_t<input_action_mapping_t, 2>;
struct input_action_t
{
    s32     type;

    u32     button_flags;
    float32 axis1D_value;
    vec2_t  axis2D_value;

    u32                          mapping_count;
    input_action_mapping_array_t mappings;
};

ENGINE_API input_action_t *s_im_input_action_create(input_manager_t *input_manager, s32 type);
ENGINE_API void            s_im_input_action_add_mapping(input_action_t *action, input_action_mapping_t *mapping);
ENGINE_API void            s_im_input_action_update_state(input_manager_t *input_manager, input_controller_t *controller);

#define InputActionPressed(action)  InputStatePressed((action)->button_flags)
#define InputActionDown(action)     InputStateDown((action)->button_flags)
#define InputActionReleased(action) InputStateReleased((action)->button_flags)

/* 
====================================================
INPUT MANAGER
==================================================== 
*/

constexpr s32 MAX_INPUT_ACTIONS     = 10;
constexpr s32 MAX_INPUT_CONTROLLERS = 20;

using input_device_array_t     = fixed_array_t<input_device_t, MAX_INPUT_DEVICES>;
using input_action_array_t     = fixed_array_t<input_action_t, MAX_INPUT_ACTIONS>;
using input_controller_array_t = fixed_array_t<input_controller_t, MAX_INPUT_CONTROLLERS>;

struct input_manager_t 
{
    bool8                    initialized;
    input_device_t          *primary_device;

    input_device_array_t     devices;
    s32                      connected_device_count;

    s32                      input_action_count;
    input_action_array_t     input_actions;

    s32                      used_controller_count;
    input_controller_array_t controllers;
};
ENGINE_API void s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager);

ENGINE_API input_device_t     *s_im_find_first_device_of_type(input_manager_t *input_manager, s32 type);
ENGINE_API input_controller_t *s_im_init_input_controller(input_manager_t *input_manager);
ENGINE_API input_event_t      *s_im_read_next_event_from_controller(input_controller_t *controller);
ENGINE_API void                s_im_controller_update_state(input_manager_t *input_manager, input_controller_t *controller, bool8 consume);
ENGINE_API u32                 s_im_observe_current_device_events(input_controller_t *controller, array_view_t<input_event_t> event_array);


ENGINE_API vec2_t
s_im_transform_mouse_data(input_controller_t *controller,
                          vec2_t              surface_size,    
                          mat4_t              view_matrix,
                          mat4_t              projection_matrix);

#endif // S_INPUT_MANAGER_H

