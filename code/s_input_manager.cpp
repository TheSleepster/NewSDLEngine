/* ========================================================================
   $File: s_input_manager.cpp $
   $Date: December 06 2025 09:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_input_manager.h>

input_controller_t*
s_im_find_controller_by_ID(input_manager_t *input_manager, u32 ID, s32 *index_out)
{
    input_controller_t *result = null;

    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *found = input_manager->controllers + controller_index;
        if(found->ID == ID)
        {
            result = found;
            if(index_out)
            {
                *index_out = controller_index;
            }

            break;
        }
    }

    return(result);
}

input_controller_t*
s_im_find_first_keyboard_controller(input_manager_t *input_manager, s32 *index_out)
{
    input_controller_t *result = null;

    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *found = input_manager->controllers + controller_index;
        if(found->type == IM_CONTROLLER_KEYBOARD)
        {
            result = found;
            if(index_out)
            {
                *index_out = controller_index;
            }

            break;
        }
    }

    return(result);
}

input_controller_t*
s_im_find_first_gamepad_controller(input_manager_t *input_manager, s32 *index_out)
{
    input_controller_t *result = null;

    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *found = input_manager->controllers + controller_index;
        if(found->type == IM_CONTROLLER_GAMEPAD)
        {
            result = found;
            if(index_out)
            {
                *index_out = controller_index;
            }

            break;
        }
    }

    return(result);
}

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
                new_controller.ID = event->gdevice.which;
                    
                new_controller.gamepad.gamepad_data   = SDL_OpenGamepad(new_controller.gamepad.gamepad_id);
                new_controller.gamepad.stick_data     = SDL_GetGamepadJoystick(new_controller.gamepad.gamepad_data);
                new_controller.gamepad.has_rumble     = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0xffff, 0xffff, 1);
                new_controller.gamepad.stick_deadzone = INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE;
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

                    break;
                }
            }

            --input_manager->connected_controller_count;
        }break;
        case SDL_EVENT_KEYBOARD_ADDED:
        {
            input_controller_t *controller = input_manager->controllers + input_manager->connected_controller_count++;
            s_im_initialize_keyboard_controller(controller, event->kdevice.which);
        }break;
        case SDL_EVENT_KEYBOARD_REMOVED:
        {
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->kdevice.which, null);
            ZeroStruct(*controller);
        }break;
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->ID != event->key.which)
            {
                s32 index = 0;
                controller = s_im_find_controller_by_ID(input_manager, event->key.which, &index);
                input_manager->primary_controller_index = index;
            }

            if(controller->is_valid)
            {
                Assert(controller->type == IM_CONTROLLER_KEYBOARD);

                u32 key_index = event->key.scancode;

                action_button_t *action_key = controller->keyboard.input + key_index;
                bool8 is_pressed  = (event->key.down && !event->key.repeat);
                bool8 is_down     =  event->key.down;
                bool8 is_released = (event->key.down == false);
                if(is_pressed)
                {
                    action_key->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
                }
                if(is_down)
                {
                    action_key->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                }
                if(is_released)
                {
                    action_key->flags  = (action_key->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
                    action_key->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED;
                }

                controller->keyboard.is_shift_key_down   = (event->key.mod & SDL_KMOD_SHIFT) != 0;
                controller->keyboard.is_control_key_down = (event->key.mod & SDL_KMOD_CTRL)  != 0;
                controller->keyboard.is_alt_key_down     = (event->key.mod & SDL_KMOD_ALT)   != 0;

                action_key->keycode  = SDL_GetKeyFromScancode(event->key.scancode, event->key.mod, false);
                action_key->scancode = event->key.scancode;

                action_key->half_transition_counter += 1;
                if(controller->action_inputs_this_frame < MAX_BUFFERED_INPUTS)
                {
                    controller->transient_action_inputs[controller->action_inputs_this_frame++] = action_key;
                }

                if(key_index < SDL_SCANCODE_COUNT &&
                   event->type == SDL_EVENT_KEY_DOWN &&
                   (action_key->keycode & SDLK_SCANCODE_MASK) == 0)
                {
                    text_input_event_t text_event = {};
                    text_event.type     = TEXT_INPUT_EVENT_TYPE_INPUT_EVENT;
                    text_event.scancode = key_index;
                    text_event.keycode  = action_key->keycode;

                    // NOTE(Sleepster): Set the input event type 
                    u32 flags = 0;
                    if(event->type == SDL_EVENT_KEY_DOWN)
                    {
                        flags |= TEXT_INPUT_EVENT_PRESSED;
                        if(action_key->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                        {
                            flags = TEXT_INPUT_EVENT_DOWN;
                        }
                    }
                    else if(event->type == SDL_EVENT_KEY_UP)
                    {
                        flags |= TEXT_INPUT_EVENT_RELEASED;
                    }
                    text_event.input_event_type = flags;

                    // NOTE(Sleepster): Set the modifiers 
                    u32 modifier_flags = 0;
                    if((event->key.mod & SDL_KMOD_SHIFT))
                    {
                        modifier_flags |= TEXT_INPUT_MODIFIER_SHIFT;
                    }

                    if((event->key.mod & SDL_KMOD_CTRL))
                    {
                        modifier_flags |= TEXT_INPUT_MODIFIER_CTRL;
                    }

                    if((event->key.mod & SDL_KMOD_ALT))
                    {
                        modifier_flags |= TEXT_INPUT_MODIFIER_ALT;
                    }

                    text_event.modifier_flags = modifier_flags;
                    controller->transient_text_inputs[controller->text_inputs_this_frame++] = text_event;
                }
            }
        }break;
        case SDL_EVENT_TEXT_INPUT:
        {
            input_controller_t *controller = input_manager->controllers + input_manager->primary_controller_index;
            Assert(controller->type == IM_CONTROLLER_KEYBOARD);

            text_input_event_t text_event = {};
            text_event.type         = TEXT_INPUT_EVENT_TYPE_CHARACTER_STREAM;
            text_event.input_stream = (u8*)event->text.text;

            controller->transient_text_inputs[controller->text_inputs_this_frame++] = text_event;
        }break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->type != IM_CONTROLLER_KEYBOARD)
            {
                s32 index = 0;
                controller = s_im_find_first_keyboard_controller(input_manager, &index);
            }

            float32 old_mouse_pos_x = controller->keyboard.current_mouse_pos.x; 
            float32 old_mouse_pos_y = controller->keyboard.current_mouse_pos.y;

            controller->keyboard.current_mouse_pos.x = event->motion.x;
            controller->keyboard.current_mouse_pos.y = event->motion.y;

            controller->keyboard.mouse_delta = vec2_subtract(controller->keyboard.current_mouse_pos, vec2(old_mouse_pos_x, old_mouse_pos_y));
        }break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->type != IM_CONTROLLER_KEYBOARD)
            {
                s32 index = 0;
                controller = s_im_find_first_keyboard_controller(input_manager, &index);
                input_manager->primary_controller_index = index;
            }

            Assert(controller->type == IM_CONTROLLER_KEYBOARD);
            u32 key_index = event->button.button + SDL_SCANCODE_COUNT;

            action_button_t *button = controller->keyboard.input + key_index;
            button->half_transition_counter += 1;
            button->scancode = key_index;
            if(event->button.down)
            {
                button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN|INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
            }
            else if(!event->button.down)
            {
                button->flags  = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
                button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED;
            }

            if(controller->action_inputs_this_frame < MAX_BUFFERED_INPUTS)
            {
                controller->transient_action_inputs[controller->action_inputs_this_frame++] = button;
            }
        }break;
        case SDL_EVENT_MOUSE_WHEEL: 
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->type != IM_CONTROLLER_KEYBOARD)
            {
                s32 index = 0;
                controller = s_im_find_first_keyboard_controller(input_manager, &index);

                input_manager->primary_controller_index = index;
            }

            s32 scroll_amount = event->wheel.integer_y;
            log_info("Scrolled...: '%d'\n", scroll_amount);
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->ID != event->gbutton.which)
            {
                s32 index = 0;
                controller = s_im_find_first_gamepad_controller(input_manager, &index);

                input_manager->primary_controller_index = index;
            }

            action_button_t *button = controller->gamepad.buttons + event->gbutton.button;
            SDL_GamepadButtonEvent button_data = event->gbutton; 

            bool8 is_pressed  = ((button_data.down == true) && (button->half_transition_counter <= 1));
            bool8 is_down     = (button_data.down  == true);
            bool8 is_released = (button_data.down  == false);
            if(is_pressed)
            {
                button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
            }
            if(is_down)
            {
                button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
            }
            if(is_released)
            {
                button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED;
            }

            button->half_transition_counter += 1;
            if(controller->action_inputs_this_frame < MAX_BUFFERED_INPUTS)
            {
                controller->transient_action_inputs[controller->action_inputs_this_frame++] = button;
            }
        }break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            input_controller_t *controller = s_im_get_primary_controller(input_manager);
            if(controller->ID != event->gaxis.which)
            {
                s32 index = 0;
                controller = s_im_find_first_gamepad_controller(input_manager, &index);

                input_manager->primary_controller_index = index;
            }

            action_button_t *analog_button = controller->gamepad.buttons + (SDL_GAMEPAD_BUTTON_COUNT + event->gaxis.axis);

            analog_button->analog_value = event->gaxis.value;
            if(analog_button->analog_value > 0)
            {
                analog_button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                analog_button->flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
            }
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
                        key_index < MAX_KEYBOARD_BUTTONS;
                        ++key_index)
                    {
                        action_button_t *button = controller->keyboard.input + key_index;
                        bool8 is_down = button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;

                        button->half_transition_counter = 0;
                        button->flags = 0;
                        if(is_down)
                        {
                            button->flags = INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                        }
                    }
                }break;
                case IM_CONTROLLER_GAMEPAD:
                {
                    for(u32 button_index = 0;
                        button_index < (SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT);
                        ++button_index)
                    {
                        action_button *button = controller->gamepad.buttons + button_index;
                        bool8 is_down = button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;

                        button->half_transition_counter = 0;
                        button->flags = 0;
                        //button->analog_value = 0;
                        if(is_down)
                        {
                            button->flags = INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                        }
                    }
                }break;
                default: {InvalidCodePath;}break;
            }
        }

        memset(controller->transient_action_inputs, 0, sizeof(action_button_t*) * controller->action_inputs_this_frame);
        controller->action_inputs_this_frame = 0;

        memset(&controller->text_inputs_this_frame, 0, sizeof(text_input_event_t) * controller->text_inputs_this_frame);
        controller->text_inputs_this_frame = 0;
    }
}

void
s_im_init_input_manager(input_manager_t *input_manager)
{
    input_manager->primary_controller_index = 0;
}

void
s_im_initialize_keyboard_controller(input_controller_t *controller, u32 ID)
{
    ZeroStruct(*controller);

    controller->is_valid  = true;
    controller->is_analog = false;
    controller->type      = IM_CONTROLLER_KEYBOARD;
    controller->ID        = ID;
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

void
s_im_set_active_controller(input_manager_t *input_manager, u32 controller_index)
{
    input_manager->active_controller_index = controller_index;
}

void
s_im_set_primary_controller(input_manager_t *input_manager, u32 controller_index)
{
    input_manager->primary_controller_index = controller_index;
}

/*==============================================
  =============== KEYBOARD INPUT ===============
  ==============================================*/

vec2_t
s_im_transform_mouse_data(input_controller_t *controller,
                          vec2_t              surface_size,    
                          mat4_t              view_matrix,
                          mat4_t              projection_matrix)
{
    Assert(controller->is_analog == false);
    vec2_t result = {};

    vec2_t mouse_pos   = controller->keyboard.current_mouse_pos;
    vec2_t window_size = surface_size;
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

    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
    return(result);
}

bool8
s_im_is_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
    return(result);
}

bool8
s_im_is_keyboard_key_released(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
    return(result);
}

void 
s_im_consume_keyboard_key_press(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;

    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
    button->half_transition_counter = 0;
}

void 
s_im_consume_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
    button->half_transition_counter = 0;
}

void 
s_im_consume_keyboard_key_release(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
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

action_button_t*
s_im_get_key_state(input_controller_t *controller, s32 key_index)
{
    action_button_t *button = controller->keyboard.input + key_index;
    return(button);
}

internal_api vec2_t
s_im_measure_keyboard_axis_2D(input_controller_t *controller, game_action_mapping_t *mapping)
{
    vec2_t result = {};
    if(s_im_is_keyboard_key_down(controller, mapping->bindings[0].binding_id))
    {
        result.y += 1.0;
    }
    if(s_im_is_keyboard_key_down(controller, mapping->bindings[1].binding_id))
    {
        result.y += -1.0;
    }
    if(s_im_is_keyboard_key_down(controller, mapping->bindings[2].binding_id))
    {
        result.x += -1.0;
    }
    if(s_im_is_keyboard_key_down(controller, mapping->bindings[3].binding_id))
    {
        result.x += 1.0;
    }

    return(result);
}

/*=============================================
  =============== GAMEPAD INPUT ===============
  =============================================*/

bool8
s_im_is_gamepad_button_pressed(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.buttons + button_index;

    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
    return(result);
}

bool8
s_im_is_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.buttons + button_index;

    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
    return(result);
}

bool8
s_im_is_gamepad_button_released(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.buttons + button_index;
    result = (button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
    return(result);
}

void 
s_im_consume_gamepad_button_press(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.buttons + button_index;
    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
    button->half_transition_counter = 0;
}

void 
s_im_consume_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.buttons + button_index;
    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
    button->half_transition_counter = 0;
}

void 
s_im_consume_gamepad_button_release(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.buttons + button_index;
    button->flags = (button->flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
    button->half_transition_counter = 0;
}

action_button_t*
s_im_gamepad_get_button_state(input_controller_t *controller, s32 button_index)
{
    action_button_t *button = controller->gamepad.buttons + button_index;
    return(button);
}

s16
s_im_gamepad_get_axis_value(input_controller_t *controller, s32 axis_id)
{
    Expect(controller->type == IM_CONTROLLER_GAMEPAD, "Must be a gamepad...\n");
    s16 result = 0;
    result = controller->gamepad.buttons[SDL_GAMEPAD_BUTTON_COUNT + axis_id].analog_value;

    return(result);
}

internal_api vec2_t
s_im_measure_gamepad_axis_2D(input_controller_t *controller, game_action_mapping_t *mapping)
{
    vec2_t result = {};

    game_action_binding_t *up_down    = &mapping->bindings[0];
    game_action_binding_t *left_right = &mapping->bindings[1];

    Expect(up_down->binding_type    == INPUT_MANAGER_BINDING_TYPE_JOYSTICK, "Sorry, for controllers we only support joysticks for axis bindings...");
    Expect(left_right->binding_type == INPUT_MANAGER_BINDING_TYPE_JOYSTICK, "Sorry, for controllers we only support joysticks for axis bindings...");

    s16 up_down_value    = s_im_gamepad_get_axis_value(controller, up_down->binding_id);
    s16 left_right_value = s_im_gamepad_get_axis_value(controller, left_right->binding_id);

    float32 up_down_normalized    = up_down_value    < 0 ? (float32)up_down_value    / 32768.0f : (float32)up_down_value    / 32767.0f;
    float32 left_right_normalized = left_right_value < 0 ? (float32)left_right_value / 32768.0f : (float32)left_right_value / 32767.0f;

    // NOTE(Sleepster): SDL reports up as negative, down as positive. Flip so +Y == up.
    result = {left_right_normalized, -up_down_normalized};

    float32 magnitude = vec2_length(result);
    if(magnitude > controller->gamepad.stick_deadzone)
    {
        float32 scaled_magnitude = (magnitude - controller->gamepad.stick_deadzone) / (1.0f - controller->gamepad.stick_deadzone);
        scaled_magnitude = Clamp(scaled_magnitude, 0.0f, 1.0f);

        result = vec2_multiply(result, vec2_create(scaled_magnitude / magnitude));
    }
    else
    {
        result = vec2_zero();
    }

    return(result);
}

/*===============================================
  =============== GAME ACTION API ===============
  ===============================================*/

game_action_t*
s_im_game_action_create(input_manager_t           *input_manager, 
                        string_t                   action_name, 
                        game_action_mapping_type_t mapping_type)
{
    game_action_t *result = null;

    game_action_t new_action = {
        .action_binding_type = mapping_type,
        .name                = action_name
    };
    c_dynarray_add(&input_manager->game_actions, &new_action);
    result = &input_manager->game_actions[input_manager->game_actions.used - 1];

    return(result);
}

void
s_im_game_action_add_mapping(game_action_t *action, game_action_mapping_t *mapping)
{
    Expect((action->mapping_count + 1) <= MAX_GAME_ACTION_MAPPINGS, "Attempted to add more mappings to a game_action than allowed... the max is 4...\n");
    action->mappings[action->mapping_count++] = *mapping;
}

void
s_im_game_action_reset_mappings(game_action_t *action)
{
    ZeroMemory(action->mappings, sizeof(game_action_mapping_t) * MAX_GAME_ACTION_MAPPINGS);
    action->mapping_count = 0;
}

s32
s_im_game_action_read_button_state(input_controller_t *controller, game_action_t *action)
{
    s32 result = 0;

    game_action_mapping_t *mapping = null;
    for(s32 mapping_index = 0;
        mapping_index < action->mapping_count;
        ++mapping_index)
    {
        game_action_mapping_t *found = action->mappings + mapping_index;
        if(found->controller_type == controller->type)
        {
            mapping = found;
            break;
        }
    }

    if(mapping)
    {
        switch(mapping->controller_type)
        {
            case IM_CONTROLLER_KEYBOARD:
            {
                action_button_t *button = s_im_get_key_state(controller, mapping->bindings[0].binding_id);
                result = button->flags;
            }break;
            case IM_CONTROLLER_GAMEPAD:
            {
                action_button_t *button = s_im_gamepad_get_button_state(controller, mapping->bindings[0].binding_id);
                result = button->flags;
            }break;
        }
    }

    return(result);
}

float32
s_im_game_action_read_axis1D_value(input_controller_t *controller, game_action_t *action)
{
    float32 result = 0.0f;

    game_action_mapping_t *mapping = null;
    for(s32 mapping_index = 0;
        mapping_index < action->mapping_count;
        ++mapping_index)
    {
        game_action_mapping_t *found = action->mappings + mapping_index;
        if(found->controller_type == controller->type)
        {
            mapping = found;
            break;
        }
    }

    if(mapping)
    {
        switch(mapping->controller_type)
        {
            case IM_CONTROLLER_KEYBOARD:
            {
                action_button_t *left_button  = s_im_get_key_state(controller, mapping->bindings[0].binding_id);
                action_button_t *right_button = s_im_get_key_state(controller, mapping->bindings[1].binding_id);
                if(left_button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    result += 1.0f;
                }

                if(right_button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    result += -1.0f;
                }
            }break;
            case IM_CONTROLLER_GAMEPAD:
            {
                s16 value = s_im_gamepad_get_axis_value(controller, mapping->bindings[0].binding_id);
                float32 value_normalized = ((float32)value / S16_MAX);

                // NOTE(Sleepster): We don't use the deadzone... 
                result = value_normalized;
            }break;
        }
    }

    return(result);
}

vec2_t
s_im_game_action_read_axis2D_value(input_controller_t *controller, game_action_t *action)
{
    vec2_t result = {};

    game_action_mapping_t *mapping = null;
    for(s32 mapping_index = 0;
        mapping_index < action->mapping_count;
        ++mapping_index)
    {
        game_action_mapping_t *found = action->mappings + mapping_index;
        if(found->controller_type == controller->type)
        {
            mapping = found;
            break;
        }
    }

    // NOTE(Sleepster): If there is a valid mapping, measure it. 
    if(mapping)
    {
        switch(mapping->controller_type)
        {
            case IM_CONTROLLER_KEYBOARD:
            {
                result = s_im_measure_keyboard_axis_2D(controller, mapping);
            }break;
            case IM_CONTROLLER_GAMEPAD:
            {
                result = s_im_measure_gamepad_axis_2D(controller, mapping);
            }break;
        }
    }

    return(result);
}
