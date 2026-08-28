/* ========================================================================
   $File: s_input_manager.cpp $
   $Date: December 06 2025 09:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_input_manager.h>

template <u32 capacity>
internal_api void
append_input_event(array_t<input_event_t, capacity> *event_array, int *count_ptr, input_event_t *event)
{
    int count = *count_ptr;
    if(count + 1 < MAX_INPUT_EVENTS)
    {
        event_array->items[count] = *event;
        ++(*count_ptr);
    }
    else
    {
        log_warning("Skipping input event as the event buffer is full...\n");
    }
}

internal_api bool8
is_same_event(input_event_t *event0, input_event_t *event1)
{
    bool8 result = false;
    u64 time0 = event0->timestampMS;
    u64 time1 = event1->timestampMS;

    s64 delta_time = (time0 > time1) ? (time0 - time1) : (time1 - time0);
    if((delta_time <= 2) || 
       ((event0->controllerID == event1->controllerID) && 
        (event0->type == event1->type)))
    {
        result = true;
    }

    return(result);
}

internal_api u32 
SDL_axis_to_input_axis(u32 SDL_axis)
{
    u32 result = {};
    result = SDL_axis + SDL_GAMEPAD_BUTTON_COUNT;
    return(result);
}

void
s_im_handle_window_inputs(SDL_Event *event, input_manager_t *input_manager)
{
    switch(event->type)
    {
        case SDL_EVENT_KEYBOARD_ADDED:
        {
            input_controller_t new_controller = {};
            new_controller.type             = INPUT_CONTROLLER_TYPE_KEYBOARD;
            new_controller.ID               = event->kdevice.which;
            new_controller.controller_index = input_manager->connected_controller_count;

            input_manager->controllers[input_manager->connected_controller_count++] = new_controller;
        }break;
        case SDL_EVENT_KEYBOARD_REMOVED:
        {
            s32 index = 0;
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->kdevice.which, &index);

            c_array_remove(&input_manager->controllers, index, input_manager->connected_controller_count);
            controller->ID = -1;
            controller->controller_index = -1;

            --input_manager->connected_controller_count;
        }break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.inputID       = event->key.scancode;
            input_event.controllerID  = event->key.which;
            input_event.timestampMS   = SDL_GetTicks();

            bool8 pressed  =  event->key.down;
            bool8 down     =  event->key.repeat;
            bool8 released = !event->key.down;

            if(pressed)  input_event.type = INPUT_EVENT_TYPE_PRESSED;
            if(down)     input_event.type = INPUT_EVENT_TYPE_DOWN;
            if(released) input_event.type = INPUT_EVENT_TYPE_RELEASED;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_TEXT_INPUT:
        {
            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_TEXT_INPUT;
            input_event.inputID       = -1;
            input_event.controllerID  = -1;
            input_event.input_stream  = STR(event->text.text);
            input_event.timestampMS   = SDL_GetTicks();

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_find_first_keyboard_controller(input_manager, null);
            u32 buttonID = event->button.button + SDL_SCANCODE_COUNT; 

            input_event_t input_event = {};
            input_event.input_type = INPUT_CONTROLLER_TYPE_KEYBOARD;
            if(event->button.clicks > 0)
            {
                input_event.type          = INPUT_EVENT_TYPE_PRESSED;
                input_event.inputID       = buttonID;
                input_event.controllerID  = controller->ID;
                input_event.timestampMS   = SDL_GetTicks();

                append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
            }

            input_event.type          = INPUT_EVENT_TYPE_DOWN;
            input_event.inputID       = buttonID;
            input_event.controllerID  = controller->ID;

            // NOTE(Sleepster): This is a REAL magic number. It's job? To give just enough of an offset on the timestamp
            // that this does not get consumed as a duplicate event->
            input_event.timestampMS   = SDL_GetTicks() + 4;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            input_controller_t *controller = s_im_find_first_keyboard_controller(input_manager, null);
            u32 buttonID = event->button.button + SDL_SCANCODE_COUNT; 

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_RELEASED;
            input_event.inputID       = buttonID;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_MOUSE_WHEEL: 
        {
            input_controller_t *controller = s_im_find_first_keyboard_controller(input_manager, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.inputID       = INPUT_AXIS_MOUSE_WHEEL;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.axis_value    = vec2(event->wheel.integer_x, event->wheel.integer_y);

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            input_controller_t *controller = s_im_find_first_keyboard_controller(input_manager, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.controllerID  = controller->ID;
            input_event.inputID       = INPUT_AXIS_MOUSE;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.axis_value    = vec2(event->motion.x, event->motion.y);

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            input_controller_t new_controller = {};
            new_controller.type             = INPUT_CONTROLLER_TYPE_GAMEPAD;
            new_controller.ID               = event->gdevice.which;
            new_controller.controller_index = input_manager->connected_controller_count;

            new_controller.gamepad.gamepad_data = SDL_OpenGamepad(event->gdevice.which);
            if(!new_controller.gamepad.gamepad_data)
            {
                log_error("Failure opening a gamepad controller... SDL_Error: '%s'..\n", SDL_GetError());
            }
            new_controller.gamepad.has_rumble     = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0x1, 0x1, 1);
            new_controller.gamepad.stick_deadzone = INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE;

            input_manager->controllers[input_manager->connected_controller_count++] = new_controller;
            log_info("Controller '%s' connected...\n", SDL_GetGamepadName(new_controller.gamepad.gamepad_data));
        }break;
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            s32 index = 0;
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->gdevice.which, &index);
            SDL_CloseGamepad(controller->gamepad.gamepad_data);
            ZeroStruct(*controller);

            controller->ID = -1;
            controller->controller_index = -1;

            c_array_remove(&input_manager->controllers, index, input_manager->connected_controller_count);
            --input_manager->connected_controller_count;
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->gbutton.which, null);
            action_button_t *button = s_im_get_controller_action_button(controller, event->gbutton.button);
            if(((button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED) == 0) &&
               ((button->flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN) == 0))
            {
                input_event_t input_event = {};
                input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
                input_event.inputID       = event->gbutton.button;
                input_event.controllerID  = controller->ID;
                input_event.timestampMS   = SDL_GetTicks();
                input_event.type = INPUT_EVENT_TYPE_PRESSED;

                append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
            }
            else
            {
                input_event_t input_event = {};
                input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
                input_event.inputID       = event->gbutton.button;
                input_event.controllerID  = controller->ID;
                input_event.timestampMS   = SDL_GetTicks() + 4;
                input_event.type = INPUT_EVENT_TYPE_DOWN;

                append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
            }
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->gbutton.which, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
            input_event.inputID       = event->gbutton.button;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.type          = INPUT_EVENT_TYPE_RELEASED;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            input_controller_t *controller = s_im_find_controller_by_ID(input_manager, event->gbutton.which, null);
            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
            input_event.inputID       = SDL_axis_to_input_axis(event->gaxis.axis);
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.axis_value    = vec2(event->gaxis.value, 0.0f);

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
    }
}

void
s_im_clear_controller_events(input_controller_t *controller)
{
    controller->event_count = 0;
}

void
s_im_init_input_manager(input_manager_t *input_manager)
{
    *input_manager = {};
}

input_controller_t*
s_im_find_controller_by_ID(input_manager_t *input_manager, s32 ID, s32 *index_out)
{
    input_controller_t *result = null;
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = input_manager->controllers + controller_index;
        if(controller->ID == ID)
        {
            result = controller;
            if(index_out) *index_out = controller_index; 

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
        input_controller_t *controller = input_manager->controllers + controller_index;
        if(controller->type == INPUT_CONTROLLER_TYPE_GAMEPAD)
        {
            result = controller;
            if(index_out) *index_out = controller_index; 

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
        input_controller_t *controller = input_manager->controllers + controller_index;
        if(controller->type == INPUT_CONTROLLER_TYPE_KEYBOARD)
        {
            result = controller;
            if(index_out) *index_out = controller_index; 

            break;
        }
    }

    return(result);
}

vec2_t
s_im_transform_mouse_data(input_controller_t *controller,
                          vec2_t              surface_size,    
                          mat4_t              view_matrix,
                          mat4_t              projection_matrix)
{
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

input_binding_state_t
s_im_get_button_binding_state(input_controller_t *controller, game_action_binding_t *binding)
{
    input_binding_state_t result = {};

#if 0
    s32 flags = 0;
    for(s32 event_index = 0;
        event_index < controller->event_count;
        ++event_index)
    {
        input_event_t *event = controller->events + event_index; 
        if((event->input_type == controller->type && !event->consumed) &&
           (binding->bindingID == event->inputID))
        {
            switch(event->type)
            {
                case INPUT_EVENT_TYPE_PRESSED:
                {
                    flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
                    flags |= (flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);

                    ++result.half_transition_count;
                }break;
                case INPUT_EVENT_TYPE_DOWN:
                {
                    flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                    flags |= (flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
                }break;
                case INPUT_EVENT_TYPE_RELEASED:
                {
                    flags |= INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED;
                    flags |= (flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
                    flags |= (flags & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);

                    ++result.half_transition_count;
                }break;
            }
        }
    }
#else
    action_button_t *button = s_im_get_controller_action_button(controller, binding->bindingID);
#endif

    result.flags = button->flags;
    result.ID    = binding->bindingID;

    return(result);
}

float32
s_im_get_axis_value(input_controller_t *controller, game_action_binding_t *binding)
{
    float32 result = 0;
#if 0
    for(s32 event_index = 0;
        event_index < controller->event_count;
        ++event_index)
    {
        input_event_t *event = controller->events + event_index; 
        if((event->input_type == controller->type && !event->consumed) &&
           (event->type == INPUT_EVENT_TYPE_AXIS_MOVED) && event->inputID == binding->bindingID)
        {
            result = event->axis_value;
        }
    }
#else
    if(controller->type == INPUT_CONTROLLER_TYPE_KEYBOARD)
    {
        if(binding->bindingID == INPUT_AXIS_MOUSE)
        {
        }
        else if(binding->bindingID == INPUT_AXIS_MOUSE_WHEEL)
        {
        }
        else
        {
            InvalidCodePath;
        }
    }
    if(controller->type == INPUT_CONTROLLER_TYPE_GAMEPAD)
    {
        action_button_t *button = s_im_get_controller_action_button(controller, binding->bindingID + SDL_GAMEPAD_BUTTON_COUNT);
        result = button->analog_value.x;
    }
#endif

    return(result);
}

// GAME ACTION API
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

void
s_im_game_action_process_button_state(input_controller_t *controller, game_action_t *action)
{
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
        input_binding_state_t key_state = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
        action->button_flags = key_state.flags;
    }
}

void
s_im_game_action_process_axis1D_state(input_controller_t *controller, game_action_t *action)
{
    float32 axis_value = action->axis1D_value;

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
            case INPUT_CONTROLLER_TYPE_KEYBOARD:
            {
                input_binding_state_t button0 = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
                input_binding_state_t button1 = s_im_get_button_binding_state(controller, &mapping->bindings[1]);
                if(button0.flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    axis_value =  1.0f;
                }

                if(button1.flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    axis_value = -1.0f;
                }

                action->axis1D_value = axis_value;
            }break;
            case INPUT_CONTROLLER_TYPE_GAMEPAD:
            {
                action->axis1D_value = s_im_get_axis_value(controller, &mapping->bindings[0]);
            }break;
        }
    }
}

void
s_im_game_action_process_axis2D_state(input_controller_t *controller, game_action_t *action)
{
    vec2_t axis_value = vec2_zero();

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
            case INPUT_CONTROLLER_TYPE_KEYBOARD:
            {
                // NOTE(Sleepster): We need to actually use the action_buttons again. Put simply, the bug is because
                //  we're looking at transient events rather than the previous states. If we set axis_value to that of
                //  aciton->axis2D_value, the problem dissapears because it can rely on the previous frame's state.
                //
                //  However, this is not a real solution. Therefore the only real solution is to treat the action buttons
                //  as read only and have them updated by the input system whenever we poll events.
                //
                //  The issue of event consumption is not valid in that circumstance because the UI will simply check
                //  every single event that happened that frame, consuming them before the game actions are ever updated.
                //
                //  The process will go like this:
                //      - The UI polls the events, the events then set the states of each of the action buttons for the controller, marking events
                //        as consumed as apppropriate
                //      - The game then updates the game_actions array to set the action_button states again, ignoring events marked as consumed.
                input_binding_state_t button0 = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
                input_binding_state_t button1 = s_im_get_button_binding_state(controller, &mapping->bindings[1]);
                input_binding_state_t button2 = s_im_get_button_binding_state(controller, &mapping->bindings[2]);
                input_binding_state_t button3 = s_im_get_button_binding_state(controller, &mapping->bindings[3]);
                if(button0.flags & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.y += 1.0;
                }
                if(button1.flags & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.y += -1.0;
                }
                if(button2.flags & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.x += -1.0;
                }
                if(button3.flags & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.x += 1.0;
                }
            }break;
            case INPUT_CONTROLLER_TYPE_GAMEPAD:
            {
                float32 Yaxis = s_im_get_axis_value(controller, &mapping->bindings[0]);
                float32 Xaxis = s_im_get_axis_value(controller, &mapping->bindings[1]);
                if(Xaxis > 0.0f)
                {
                    int x = 0;
                }

                float32 Yaxis_normalized = Yaxis < 0.0f ? Yaxis / 32768.0f : Yaxis / 32767.0f;
                float32 Xaxis_normalized = Xaxis < 0.0f ? Xaxis / 32768.0f : Xaxis / 32767.0f;

                // NOTE(Sleepster): SDL reports up as negative, down as positive. Flip so +Y == up.
                axis_value = {Xaxis_normalized, -Yaxis_normalized};

                float32 magnitude = vec2_length(axis_value);
                if(magnitude > controller->gamepad.stick_deadzone)
                {
                    float32 scaled_magnitude = (magnitude - controller->gamepad.stick_deadzone) / (1.0f - controller->gamepad.stick_deadzone);
                    scaled_magnitude = Clamp(scaled_magnitude, 0.0f, 1.0f);

                    axis_value = vec2_multiply(axis_value, vec2_create(scaled_magnitude / magnitude));
                    axis_value = vec2_normalize(axis_value);
                }
                else
                {
                    axis_value = vec2_zero();
                }
            }break;
        }
    }

    action->axis2D_value = axis_value;
}

void
s_im_update_game_action_states(input_manager_t *input_manager, input_controller_t *controller)
{
    for(game_action_t &action: input_manager->game_actions)
    {
        action.button_flags &= ~(INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED | INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
        switch(action.action_binding_type)
        {
            case INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON:
            {
                s_im_game_action_process_button_state(controller, &action);
            }break;
            case INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS1D:
            {
                s_im_game_action_process_axis1D_state(controller, &action);
            }break;
            case INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS2D:
            {
                s_im_game_action_process_axis2D_state(controller, &action);
            }break;
        }
    }
}

action_button_t*
s_im_get_controller_action_button(input_controller_t *controller, s32 inputID)
{
    action_button_t *result = null;
    switch(controller->type)
    {
        case INPUT_CONTROLLER_TYPE_KEYBOARD:
        {
            result = controller->keyboard.input + inputID;
        }break;
        case INPUT_CONTROLLER_TYPE_GAMEPAD:
        {
            result = controller->gamepad.buttons + inputID;
        }break;
    }

    return(result);
}
