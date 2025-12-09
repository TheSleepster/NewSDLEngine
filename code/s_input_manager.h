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
#define SDL_SCANCODE_MAX (SDL_SCANCODE_COUNT + 5)

typedef enum controller_type
{
    IM_CONTROLLER_INVALID,
    IM_CONTROLLER_GAMEPAD,
    IM_CONTROLLER_KEYBOARD,
    IM_CONTROLLER_COUNT,
}controller_type_t;

typedef enum input_mouse_buttons
{
    SDL_LEFT_MOUSE         = 513,
    SDL_RIGHT_MOUSE        = 514,
    SDL_MIDDLE_MOUSE       = 515,
    SDL_X1_MOUSE           = 516,
    SDL_X2_MOUSE           = 517,
    SDL_MOUSE_BUTTON_COUNT = 5
}input_mouse_buttons_t;

typedef struct action_button
{
    bool8 is_down;
    bool8 is_released;
    bool8 is_pressed;

    u8    half_transition_counter;
}action_button_t;

typedef struct keyboard_controller_data
{
    action_button_t input[SDL_SCANCODE_MAX + SDL_MOUSE_BUTTON_COUNT];

    vec2_t          current_mouse_pos;
    vec2_t          last_mouse_pos;
    vec2_t          mouse_delta;

    bool8           is_shift_key_down;
    bool8           is_control_key_down;
    bool8           is_alt_key_down;
}keyboard_controller_data_t;

typedef struct analog_button
{
    s16 deadzone;
    s16 value;
}analog_button_t;

typedef struct gamepad_controller_data
{
    SDL_Gamepad        *gamepad_data;
    SDL_Joystick       *stick_data;
    u32                 gamepad_id;

    bool8               has_rumble;
    s32                 rumble_value;

    action_button_t     digital_buttons[SDL_GAMEPAD_BUTTON_COUNT];
    analog_button_t     analog_buttons[SDL_GAMEPAD_AXIS_COUNT];
}gamepad_controller_data_t;

typedef struct input_controller
{
    bool8             is_valid;
    bool8             is_analog;
    controller_type_t type;
    union
    {
        keyboard_controller_data_t keyboard;
        gamepad_controller_data_t  gamepad;
    };
}input_controller_t;

typedef enum game_action_binding_type
{
    GAB_Invalid,
    GAB_Button,
    GAB_Axis,
    GAB_Count
}game_action_binding_type_t;

typedef struct game_action_binding
{
    u32 type;
    u32 binding_id;
}game_action_binding_t;

// TODO(Sleepster): AXIS ASSIGNMENT (things like controller left stick assignment) 
typedef struct game_action
{
    game_action_binding_t keyboard;
    game_action_binding_t gamepad;

    string_t              name;
}game_action_t;

// NOTE(Sleepster): Hey, the keyboard connected to the PC is ALWAYS the primary device 
typedef struct input_manager
{
    keyboard_controller_data_t keyboard_data;
    gamepad_controller_data_t  gamepad_data;
    
    u32                        primary_controller_index;
    u32                        active_controller_index;
    u32                        connected_controller_count;
    input_controller_t         controllers[MAX_INPUT_CONTROLLERS];

    DynArray(game_action_t)   game_actions;
}input_manager_t;

/*===========================================
  =============== GENERAL API ===============
  ===========================================*/
void                s_im_init_input_manager(input_manager_t *input_manager);
void                s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager);
void                s_im_reset_controller_states(input_manager_t *input_manager);
void                s_im_initialize_keyboard_controller(input_manager_t *input_manager, s32 index);
input_controller_t* s_im_get_primary_controller(input_manager_t *input_manager);
input_controller_t* s_im_get_controller_at_index(input_manager_t *input_manager, s32 index);
input_controller_t* s_im_get_active_controller(input_manager_t *input_manager);
bool8               s_im_is_shift_key_down(input_controller_t *controller);
bool8               s_im_is_control_key_down(input_controller_t *controller);
bool8               s_im_is_alt_key_down(input_controller_t *controller);

/*==============================================
  =============== KEYBOARD INPUT ===============
  ==============================================*/
vec2_t s_im_transform_mouse_data(input_controller_t *controller, mat4_t view_matrix, mat4_t projection_matrix);

bool8  s_im_is_keyboard_key_pressed(input_controller_t *controller, s32 key_index);
bool8  s_im_is_keyboard_key_down(input_controller_t *controller, s32 key_index);
bool8  s_im_is_keyboard_key_released(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_press(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_down(input_controller_t *controller, s32 key_index);
void   s_im_consume_keyboard_key_release(input_controller_t *controller, s32 key_index);

inline action_button_t* s_im_get_key_state(input_controller_t *controller, s32 key_index);

/*=============================================
  =============== GAMEPAD INPUT ===============
  =============================================*/
bool8  s_im_is_gamepad_button_pressed(input_controller_t *controller, s32 button_index);
bool8  s_im_is_gamepad_button_down(input_controller_t *controller, s32 button_index);
bool8  s_im_is_gamepad_button_released(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_press(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_down(input_controller_t *controller, s32 button_index);
void   s_im_consume_gamepad_button_release(input_controller_t *controller, s32 button_index);

inline action_button_t* s_im_get_button_state(input_controller_t *controller, s32 button_index);
/*===========================================
  ============= GAME ACTION API =============
  ===========================================*/

#endif // S_INPUT_MANAGER_H

