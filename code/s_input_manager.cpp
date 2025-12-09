/* ========================================================================
   $File: s_input_manager.cpp $
   $Date: December 06 2025 09:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_input_manager.h>

void
s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager)
{
    switch(event->type)
    {
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            input_controller_t new_controller = {};
            new_controller.is_analog = true;
            new_controller.is_valid  = true;
            new_controller.type      = IM_CONTROLLER_GAMEPAD;
            if(SDL_IsGamepad(event->gdevice.which))
            {
                new_controller.gamepad.gamepad_id = event->gdevice.which;
                    
                new_controller.gamepad.gamepad_data = SDL_OpenGamepad(new_controller.gamepad.gamepad_id);
                new_controller.gamepad.stick_data   = SDL_GetGamepadJoystick(new_controller.gamepad.gamepad_data);
                new_controller.gamepad.has_rumble   = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0xffff, 0xffff, 100000);

                if(input_manager->connected_controller_count < MAX_INPUT_CONTROLLERS)
                {
                    log_info("Controller '%s' connected...\n", SDL_GetGamepadName(new_controller.gamepad.gamepad_data));

                    input_manager->controllers[input_manager->connected_controller_count] = new_controller;
                    input_manager->primary_controller_index    = input_manager->connected_controller_count;
                    input_manager->connected_controller_count += 1;
                }
                else
                {
                    log_info("Unable to connect gamepad device... Maximum controller count of: '%d' has been reached...\n", MAX_INPUT_CONTROLLERS);
                }
            }
        }break;
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            u32 controller_id = event->gdevice.which;
            for(u32 controller_index = 0;
                controller_index < MAX_INPUT_CONTROLLERS;
                ++controller_index)
            {
                input_controller_t *controller = input_manager->controllers + controller_index;
                if(controller->type == IM_CONTROLLER_GAMEPAD &&
                   controller->gamepad.gamepad_id == controller_id)
                {
                    if(controller_index == input_manager->primary_controller_index)
                    {
                        input_manager->primary_controller_index = 0;
                    }
                    if(controller_index == input_manager->active_controller_index)
                    {
                        input_manager->active_controller_index = 0;
                    }

                    SDL_CloseGamepad(controller->gamepad.gamepad_data);
                    SDL_CloseJoystick(controller->gamepad.stick_data);

                    ZeroStruct(*controller);
                    input_manager->connected_controller_count -= 1;

                    break;
                }
            }
        }break;
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
        {
            input_controller_t *controller = input_manager->controllers;
            Assert(controller->type == IM_CONTROLLER_KEYBOARD);

            u32 key_index = event->key.scancode;

            action_button_t *action_key = controller->keyboard.input + key_index;
            action_key->is_pressed      = (event->key.down && !event->key.repeat);
            action_key->is_down         =  event->key.down;
            action_key->is_released     = (event->key.down == false);

            controller->keyboard.is_shift_key_down   = (event->key.mod & SDL_KMOD_SHIFT) != 0;
            controller->keyboard.is_control_key_down = (event->key.mod & SDL_KMOD_CTRL)  != 0;
            controller->keyboard.is_alt_key_down     = (event->key.mod & SDL_KMOD_ALT)   != 0;

            action_key->half_transition_counter += 1;
        }break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->type == IM_CONTROLLER_KEYBOARD)
            {
                float32 old_mouse_pos_x = controller->keyboard.current_mouse_pos.x; 
                float32 old_mouse_pos_y = controller->keyboard.current_mouse_pos.y;

                controller->keyboard.current_mouse_pos.x = event->motion.x;
                controller->keyboard.current_mouse_pos.y = event->motion.y;

                controller->keyboard.mouse_delta = vec2_subtract(controller->keyboard.current_mouse_pos, vec2(old_mouse_pos_x, old_mouse_pos_y));
            }
        }break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            Assert(controller->type == IM_CONTROLLER_KEYBOARD);

            u32 key_index = event->button.button + SDL_SCANCODE_COUNT;

            action_button_t *button = controller->keyboard.input + key_index;
            button->is_down     = event->button.down;
            button->is_pressed  = event->button.down  == true;
            button->is_released = event->button.down  == false;

            button->half_transition_counter += event->button.clicks;
        }break;
        case SDL_EVENT_MOUSE_WHEEL: 
        {
            s32 scroll_amount = event->wheel.integer_y;
            log_info("Scrolled...: '%d'\n", scroll_amount);
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            action_button_t *button = controller->gamepad.digital_buttons + event->gbutton.button;
                
            SDL_GamepadButtonEvent button_data = event->gbutton; 
            button->is_pressed  = ((button_data.down == true) && (button->half_transition_counter <= 1));
            button->is_down     = (button_data.down  == true);
            button->is_released = (button_data.down  == false);

            button->half_transition_counter += 1;
        }break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            analog_button_t    *analog_button = controller->gamepad.analog_buttons + event->gaxis.axis;

            analog_button->value = event->gaxis.value;
        }break;
    }
}

void
s_im_reset_controller_states(input_manager_t *input_manager)
{
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = input_manager->controllers + controller_index;
        if(controller->is_valid)
        {
            switch(controller->type)
            {
                case IM_CONTROLLER_KEYBOARD:
                {
                    for(u32 key_index = 0;
                        key_index < SDL_SCANCODE_MAX + SDL_MOUSE_BUTTON_COUNT;
                        ++key_index)
                    {
                        action_button_t *button = controller->keyboard.input + key_index;
                        button->half_transition_counter = 0;
                        button->is_pressed  = false;
                        button->is_released = false;
                    }
                }break;
                case IM_CONTROLLER_GAMEPAD:
                {
                    for(u32 button_index = 0;
                        button_index < SDL_GAMEPAD_BUTTON_COUNT;
                        ++button_index)
                    {
                        action_button *button = controller->gamepad.digital_buttons + button_index;
                        button->half_transition_counter = 0;
                        button->is_pressed  = false;
                        button->is_released = false;
                    }

                    for(u32 analog_button_index = 0;
                        analog_button_index < SDL_GAMEPAD_AXIS_COUNT;
                        ++analog_button_index)
                    {
                        analog_button_t *button = controller->gamepad.analog_buttons + analog_button_index;
                        button->value = 0;
                    }
                }break;
                default: {InvalidCodePath;}break;
            }
        }
    }
}

void
s_im_init_input_manager(input_manager_t *input_manager)
{
    s_im_initialize_keyboard_controller(input_manager, 0);
    input_manager->primary_controller_index = 0;
}

void
s_im_initialize_keyboard_controller(input_manager_t *input_manager, s32 index)
{
    input_controller_t *controller = input_manager->controllers + index;
    ZeroStruct(*controller);

    controller->is_valid  = true;
    controller->is_analog = false;
    controller->type      = IM_CONTROLLER_KEYBOARD;
}


input_controller_t *
s_im_get_primary_controller(input_manager_t *input_manager)
{
    input_controller_t *result = null;
    result = input_manager->controllers + input_manager->primary_controller_index;

    return(result);
}

input_controller_t *
s_im_get_controller_at_index(input_manager_t *input_manager, s32 index)
{
    Assert(index < MAX_INPUT_CONTROLLERS);
    
    input_controller_t *result = null;
    result = input_manager->controllers + index;

    return(result);
}

input_controller_t *
s_im_get_active_controller(input_manager_t *input_manager)
{
    input_controller_t *result = null;
    result = input_manager->controllers + input_manager->active_controller_index;

    return(result);
}
/*==============================================
  =============== KEYBOARD INPUT ===============
  ==============================================*/

vec2_t
s_im_transform_mouse_data(input_controller_t *controller,
                                     mat4_t             view_matrix,
                                     mat4_t             projection_matrix)
{
    vec2_t result = {};

    vec2_t mouse_pos   = controller->keyboard.current_mouse_pos;
    vec2_t window_size = g_window_size;
    vec4_t ndc_pos     = vec4((mouse_pos.x / (window_size.x * 0.5f)) - 1.0f, 1.0f - (mouse_pos.y / (window_size.y * 0.5f)), 0.0f, 1.0f);

    mat4_t inverse_projection = mat4_invert(projection_matrix);
    mat4_t inverse_view       = mat4_invert(view_matrix);

    ndc_pos = vec4_transform(inverse_projection, ndc_pos);
    ndc_pos = vec4_transform(inverse_view,    ndc_pos);

    result = ndc_pos.xy;
    return(result);
}

bool8
s_im_is_keyboard_key_pressed(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_pressed;
    return(result);
}

bool8
s_im_is_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_down;
    return(result);
}

bool8
s_im_is_keyboard_key_released(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_released;
    return(result);
}

void 
s_im_consume_keyboard_key_press(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_pressed              = false;
    button->half_transition_counter = 0;
}

void 
s_im_consume_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_down                 = false;
    button->half_transition_counter = 0;
}

void 
s_im_consume_keyboard_key_release(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_released             = false;
    button->half_transition_counter = 0;
}

bool8
s_im_is_shift_key_down(input_controller_t *controller)
{
    bool8 result = false;
    result = controller->keyboard.is_shift_key_down;

    return(result);
}

bool8
s_im_is_control_key_down(input_controller_t *controller)
{
    bool8 result = false;
    result = controller->keyboard.is_control_key_down;

    return(result);
}

bool8
s_im_is_alt_key_down(input_controller_t *controller)
{
    bool8 result = false;
    result = controller->keyboard.is_alt_key_down;

    return(result);
}

inline action_button_t*
s_im_get_key_state(input_controller_t *controller, s32 key_index)
{
    action_button_t *button = controller->keyboard.input + key_index;
    return(button);
}

/*=============================================
  =============== GAMEPAD INPUT ===============
  =============================================*/

bool8
s_im_is_gamepad_button_pressed(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = button->is_pressed;
    return(result);
}

bool8
s_im_is_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = (button->is_down || button->is_pressed);
    return(result);
}

bool8
s_im_is_gamepad_button_released(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = button->is_released;
    return(result);
}

void 
s_im_consume_gamepad_button_press(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_pressed              = false;
    button->half_transition_counter = 0;
}

void 
s_im_consume_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_down                 = false;
    button->half_transition_counter = 0;
}

void 
s_im_consume_gamepad_button_release(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_released             = false;
    button->half_transition_counter = 0;
}

inline action_button_t*
s_im_get_button_state(input_controller_t *controller, s32 button_index)
{
    action_button_t *button = controller->gamepad.digital_buttons + button_index;
    return(button);
}

/*===============================================
  =============== GAME ACTION API ===============
  ===============================================*/

void
s_game_action_create(input_manager_t *input_manager, 
                     string_t         name, 
                     u32              first_binding, 
                     u32              first_binding_type, 
                     u32              second_binding, 
                     u32              second_binding_type)
{
    game_action_t new_action       = {};
    new_action.keyboard.type       = first_binding_type;
    new_action.keyboard.binding_id = first_binding;
    new_action.gamepad.type        = second_binding;
    new_action.gamepad.binding_id  = second_binding_type;

    c_dynarray_push(input_manager->game_actions, new_action); 
}

