/* ========================================================================
   $File: s_input_manager.cpp $
   $Date: December 06 2025 09:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_input_manager.h>

input_controller_t*
find_controller_by_ID(input_manager_t *event_handler, s32 ID, s32 *index_out)
{
    input_controller_t *result = null;
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = event_handler->controllers + controller_index;
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
find_first_gamepad_controller(input_manager_t *event_handler, s32 *index_out)
{
    input_controller_t *result = null;
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = event_handler->controllers + controller_index;
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
find_first_keyboard_controller(input_manager_t *event_handler, s32 *index_out)
{
    input_controller_t *result = null;
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = event_handler->controllers + controller_index;
        if(controller->type == INPUT_CONTROLLER_TYPE_KEYBOARD)
        {
            result = controller;
            if(index_out) *index_out = controller_index; 

            break;
        }
    }

    return(result);
}

template <u32 capacity>
void
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

internal_api input_axis_t
SDL_axis_to_input_axis(u32 SDL_axis)
{
    input_axis_t result = {};
    switch(SDL_axis)
    {
        case SDL_GAMEPAD_AXIS_LEFTX:
        {
            result = INPUT_AXIS_GAMEPAD_LEFT_X;
        }break;
        case SDL_GAMEPAD_AXIS_LEFTY:
        {
            result = INPUT_AXIS_GAMEPAD_LEFT_Y;
        }break;
        case SDL_GAMEPAD_AXIS_RIGHTX:
        {
            result = INPUT_AXIS_GAMEPAD_RIGHT_X;
        }break;
        case  SDL_GAMEPAD_AXIS_RIGHTY:
        {
            result = INPUT_AXIS_GAMEPAD_RIGHT_Y;
        }break;
        case  SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        {
            result = INPUT_AXIS_GAMEPAD_LEFT_TRIGGER;
        }break;
        case  SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        {
            result = INPUT_AXIS_GAMEPAD_RIGHT_TRIGGER;
        }break;
    }

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
            input_controller_t *controller = find_controller_by_ID(input_manager, event->kdevice.which, &index);

            c_array_remove(&input_manager->controllers, index, input_manager->connected_controller_count);
            controller->ID = -1;
            controller->controller_index = -1;

            --input_manager->connected_controller_count;
        }break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            u32 keyID = event->key.key;

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.inputID       = keyID;
            input_event.controllerID  = event->key.which;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.scancode      = event->key.scancode;

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
            input_controller_t *controller = find_first_keyboard_controller(input_manager, null);
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
            input_controller_t *controller = find_first_keyboard_controller(input_manager, null);
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
            input_controller_t *controller = find_first_keyboard_controller(input_manager, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.inputID       = INPUT_AXIS_MOUSE_WHEEL_X;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.axis_value    = event->wheel.integer_x;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);

            input_event.inputID       = INPUT_AXIS_MOUSE_WHEEL_Y;
            input_event.timestampMS   = SDL_GetTicks() + 4;
            input_event.axis_value    = event->wheel.integer_y;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            input_controller_t *controller = find_first_keyboard_controller(input_manager, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.inputID       = INPUT_AXIS_MOUSE_X;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.axis_value    = event->motion.x;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);

            input_event.inputID       = INPUT_AXIS_MOUSE_Y;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.axis_value    = event->motion.y;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_GAMEPAD_ADDED:
        {
            input_controller_t new_controller = {};
            new_controller.type             = INPUT_CONTROLLER_TYPE_GAMEPAD;
            new_controller.ID               = event->gdevice.which;
            new_controller.controller_index = input_manager->connected_controller_count;

            new_controller.gamepad.gamepad_data   = SDL_OpenGamepad(new_controller.gamepad.gamepad_id);
            new_controller.gamepad.has_rumble     = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0x1, 0x1, 1);
            new_controller.gamepad.stick_deadzone = INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE;

            input_manager->controllers[input_manager->connected_controller_count++] = new_controller;
            log_info("Controller '%s' connected...\n", SDL_GetGamepadName(new_controller.gamepad.gamepad_data));
        }break;
        case SDL_EVENT_GAMEPAD_REMOVED:
        {
            s32 index = 0;
            input_controller_t *controller = find_controller_by_ID(input_manager, event->gdevice.which, &index);
            SDL_CloseGamepad(controller->gamepad.gamepad_data);
            ZeroStruct(*controller);

            controller->ID = -1;
            controller->controller_index = -1;

            c_array_remove(&input_manager->controllers, index, input_manager->connected_controller_count);
            --input_manager->connected_controller_count;
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        {
            input_controller_t *controller = find_controller_by_ID(input_manager, event->gbutton.which, null);

            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
            input_event.inputID       = event->gbutton.button;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.type = INPUT_EVENT_TYPE_PRESSED;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);

            input_event.inputID       = event->gbutton.button;
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks() + 4;
            input_event.type = INPUT_EVENT_TYPE_DOWN;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            input_controller_t *controller = find_controller_by_ID(input_manager, event->gbutton.which, null);

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
            input_controller_t *controller = find_controller_by_ID(input_manager, event->gbutton.which, null);
            input_event_t input_event = {};
            input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
            input_event.inputID       = SDL_axis_to_input_axis(event->gaxis.axis);
            input_event.controllerID  = controller->ID;
            input_event.timestampMS   = SDL_GetTicks();
            input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
            input_event.axis_value    = event->gaxis.value;

            append_input_event(&input_manager->events, &input_manager->event_count, &input_event);
        }break;
    }
}

void
s_im_init_input_manager(input_manager_t *input_manager)
{
    ZeroStruct(*input_manager);
}
