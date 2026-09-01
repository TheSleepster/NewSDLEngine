/* ========================================================================
   $File: input.cpp $
   $Date: July 30 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <c_dynarray.h>

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

struct action_button_t
{
    // NOTE(Sleepster): UTF32 keycode 
    u32   keycode;
    u32   scancode;
    u32   flags;
    s16   analog_value;
};

struct keyboard_controller_data_t
{
    action_button_t  input[MAX_KEYBOARD_BUTTONS];

    vec2_t           current_mouse_pos;
    vec2_t           last_mouse_pos;
    vec2_t           mouse_delta;

    bool8            is_shift_key_down;
    bool8            is_control_key_down;
    bool8            is_alt_key_down;
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
constexpr s32 MAX_INPUT_CONTROLLERS = 4;

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
    INPUT_AXIS_MOUSE_X,
    INPUT_AXIS_MOUSE_Y,
    INPUT_AXIS_MOUSE_WHEEL_X,
    INPUT_AXIS_MOUSE_WHEEL_Y,

    INPUT_AXIS_GAMEPAD_LEFT_X,
    INPUT_AXIS_GAMEPAD_LEFT_Y,
    INPUT_AXIS_GAMEPAD_RIGHT_X,
    INPUT_AXIS_GAMEPAD_RIGHT_Y,
    INPUT_AXIS_GAMEPAD_LEFT_TRIGGER,
    INPUT_AXIS_GAMEPAD_RIGHT_TRIGGER,
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
    float32  axis_value;
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

// TODO(Sleepster): event_handler_t 
struct input_event_handler_t 
{
    s32 event_count;
    s32 connected_controller_count;
    s32 active_controller_index;

    dynarray_t<game_action_t>                          game_actions;
    array_t<input_controller_t, MAX_INPUT_CONTROLLERS> controllers;
    array_t<input_event_t,      MAX_INPUT_EVENTS>      events;
};

input_controller_t*
find_controller_by_ID(input_event_handler_t *event_handler, s32 ID, s32 *index_out)
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
find_first_gamepad_controller(input_event_handler_t *event_handler, s32 *index_out)
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
find_first_keyboard_controller(input_event_handler_t *event_handler, s32 *index_out)
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

input_controller_t*
find_controller_of_type(input_event_handler_t *event_handler, s32 type)
{
    input_controller_t *result = null;
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = event_handler->controllers + controller_index;
        if(controller->type == type)
        {
            result = controller;
            break;
        }
    }

    return(result);
}

template <int capacity>
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

// game actions
game_action_t*
s_im_game_action_create(input_event_handler_t     *event_handler, 
                        string_t                   action_name, 
                        game_action_mapping_type_t mapping_type)
{
    game_action_t *result = null;

    game_action_t new_action = {
        .action_binding_type = (u32)mapping_type,
        .name                = action_name
    };
    c_dynarray_add(&event_handler->game_actions, &new_action);
    result = &event_handler->game_actions[event_handler->game_actions.used - 1];

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

struct binding_state_t
{
    s32 flags;
    s32 half_transition_count;
    s32 ID;
};

s32
s_im_get_button_binding_state(input_controller_t *controller, game_action_binding_t *binding)
{
    s32 result = 0;
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
                    result |= INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED;
                    result |= (result & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED);
                }break;
                case INPUT_EVENT_TYPE_DOWN:
                {
                    result |= INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN;
                    result |= (result & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
                }break;
                case INPUT_EVENT_TYPE_RELEASED:
                {
                    result |= INPUT_MANAGER_ACTION_BUTTON_FLAG_RELEASED;
                    result |= (result & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED);
                    result |= (result & ~INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN);
                }break;
            }
        }
    }

    return(result);
}

s16
s_im_get_axis_value(input_controller_t *controller, game_action_binding_t *binding)
{
    s16 result = 0;
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

    return(result);
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
        s32 key_state = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
        action->button_flags = key_state;
    }
}

void
s_im_game_action_process_axis1D_state(input_controller_t *controller, game_action_t *action)
{
    float32 axis_value = 0.0f;

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
                s32 button0 = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
                s32 button1 = s_im_get_button_binding_state(controller, &mapping->bindings[1]);
                if(button0 & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    axis_value += 1.0f;
                }

                if(button1 & INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN)
                {
                    axis_value += -1.0f;
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
                s32 button0 = s_im_get_button_binding_state(controller, &mapping->bindings[0]);
                s32 button1 = s_im_get_button_binding_state(controller, &mapping->bindings[1]);
                s32 button2 = s_im_get_button_binding_state(controller, &mapping->bindings[2]);
                s32 button3 = s_im_get_button_binding_state(controller, &mapping->bindings[3]);
                if(button0 & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.y += 1.0;
                }
                if(button1 & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.y += -1.0;
                }
                if(button2 & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.x += -1.0;
                }
                if(button3 & (INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED|INPUT_MANAGER_ACTION_BUTTON_FLAG_DOWN))
                {
                    axis_value.x += 1.0;
                }
            }break;
            case INPUT_CONTROLLER_TYPE_GAMEPAD:
            {
                s16 Yaxis = s_im_get_axis_value(controller, &mapping->bindings[0]);
                s16 Xaxis = s_im_get_axis_value(controller, &mapping->bindings[1]);

                float32 Yaxis_normalized = Yaxis < 0 ? (float32)Yaxis / 32768.0f : (float32)Yaxis / 32767.0f;
                float32 Xaxis_normalized = Xaxis < 0 ? (float32)Xaxis / 32768.0f : (float32)Xaxis / 32767.0f;

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

int
main(void)
{   
    input_event_handler_t event_handler = {};
    c_global_context_init();

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD);

    SDL_Window *window     = SDL_CreateWindow("Test Window", 800, 600, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, null);
    SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED);

    SDL_ShowWindow(window);
    SDL_StartTextInput(window);

    // NOTE(Sleepster): Movement 
    game_action_t *movement_action = s_im_game_action_create(&event_handler, STR("character move"), INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS2D);

    game_action_mapping_t keyboard_movement_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_SCANCODE_W}, {SDL_SCANCODE_S}, {SDL_SCANCODE_A}, {SDL_SCANCODE_D}},
        .binding_count = 4,
        .controller_type = INPUT_CONTROLLER_TYPE_KEYBOARD 
    };
    s_im_game_action_add_mapping(movement_action, &keyboard_movement_mapping);

    game_action_mapping_t controller_movement_mapping = (game_action_mapping_t) {
        .bindings = {
            {SDL_GAMEPAD_AXIS_LEFTY, INPUT_MANAGER_BINDING_TYPE_JOYSTICK}, 
            {SDL_GAMEPAD_AXIS_LEFTX, INPUT_MANAGER_BINDING_TYPE_JOYSTICK}
        },
        .binding_count   = 2,
        .controller_type = INPUT_CONTROLLER_TYPE_GAMEPAD 
    };
    s_im_game_action_add_mapping(movement_action, &controller_movement_mapping);

    // NOTE(Sleepster): Jump 
    game_action_t *jump_action = s_im_game_action_create(&event_handler, STR("Jump"), INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON);
    game_action_mapping_t keyboard_jump_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_SCANCODE_SPACE}},
        .binding_count = 1,
        .controller_type = INPUT_CONTROLLER_TYPE_KEYBOARD
    };
    s_im_game_action_add_mapping(jump_action, &keyboard_jump_mapping);

    game_action_mapping_t gamepad_jump_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_GAMEPAD_BUTTON_SOUTH}},
        .binding_count = 1,
        .controller_type = INPUT_CONTROLLER_TYPE_GAMEPAD
    };
    s_im_game_action_add_mapping(jump_action, &gamepad_jump_mapping);

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    u64 last_tsc        = SDL_GetPerformanceCounter();
    u64 current_tsc     = 0;
    u64 delta_tsc       = 0;

    float32 delta_time     = 0;
    float64 dt_accumulator = 0.0f;
    for(;;)
    {
        SDL_Event event = {};
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_EVENT_KEYBOARD_ADDED:
                {
                    input_controller_t new_controller = {};
                    new_controller.type             = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    new_controller.ID               = event.kdevice.which;
                    new_controller.controller_index = event_handler.connected_controller_count;

                    event_handler.controllers[event_handler.connected_controller_count++] = new_controller;
                }break;
                case SDL_EVENT_KEYBOARD_REMOVED:
                {
                    s32 index = 0;
                    input_controller_t *controller = find_controller_by_ID(&event_handler, event.kdevice.which, &index);

                    c_array_remove(event_handler.controllers, index, event_handler.connected_controller_count);
                    controller->ID = -1;
                    controller->controller_index = -1;

                    --event_handler.connected_controller_count;
                }break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                {
                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    input_event.inputID       = event.key.scancode;
                    input_event.controllerID  = event.key.which;
                    input_event.timestampMS   = SDL_GetTicks();

                    bool8 pressed  =  event.key.down;
                    bool8 down     =  event.key.repeat;
                    bool8 released = !event.key.down;

                    if(pressed)  input_event.type = INPUT_EVENT_TYPE_PRESSED;
                    if(down)     input_event.type = INPUT_EVENT_TYPE_DOWN;
                    if(released) input_event.type = INPUT_EVENT_TYPE_RELEASED;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_TEXT_INPUT:
                {
                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    input_event.type          = INPUT_EVENT_TYPE_TEXT_INPUT;
                    input_event.inputID       = -1;
                    input_event.controllerID  = -1;
                    input_event.input_stream  = STR(event.text.text);
                    input_event.timestampMS   = SDL_GetTicks();

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    input_controller_t *controller = find_first_keyboard_controller(&event_handler, null);
                    u32 buttonID = event.button.button + SDL_SCANCODE_COUNT; 

                    input_event_t input_event = {};
                    input_event.input_type = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    if(event.button.clicks > 0)
                    {
                        input_event.type          = INPUT_EVENT_TYPE_PRESSED;
                        input_event.inputID       = buttonID;
                        input_event.controllerID  = controller->ID;
                        input_event.timestampMS   = SDL_GetTicks();

                        append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                    }

                    input_event.type          = INPUT_EVENT_TYPE_DOWN;
                    input_event.inputID       = buttonID;
                    input_event.controllerID  = controller->ID;

                    // NOTE(Sleepster): This is a REAL magic number. It's job? To give just enough of an offset on the timestamp
                    // that this does not get consumed as a duplicate event.
                    input_event.timestampMS   = SDL_GetTicks() + 4;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    input_controller_t *controller = find_first_keyboard_controller(&event_handler, null);
                    u32 buttonID = event.button.button + SDL_SCANCODE_COUNT; 

                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    input_event.type          = INPUT_EVENT_TYPE_RELEASED;
                    input_event.inputID       = buttonID;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_MOUSE_WHEEL: 
                {
                    input_controller_t *controller = find_first_keyboard_controller(&event_handler, null);

                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
                    input_event.inputID       = INPUT_AXIS_MOUSE_WHEEL_X;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.axis_value    = event.wheel.integer_x;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);

                    input_event.inputID       = INPUT_AXIS_MOUSE_WHEEL_Y;
                    input_event.timestampMS   = SDL_GetTicks() + 4;
                    input_event.axis_value    = event.wheel.integer_y;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_MOUSE_MOTION:
                {
                    input_controller_t *controller = find_first_keyboard_controller(&event_handler, null);

                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_KEYBOARD;
                    input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
                    input_event.inputID       = INPUT_AXIS_MOUSE_X;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.axis_value    = event.motion.x;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);

                    input_event.inputID       = INPUT_AXIS_MOUSE_Y;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.axis_value    = event.motion.y;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_GAMEPAD_ADDED:
                {
                    input_controller_t new_controller = {};
                    new_controller.type             = INPUT_CONTROLLER_TYPE_GAMEPAD;
                    new_controller.ID               = event.gdevice.which;
                    new_controller.controller_index = event_handler.connected_controller_count;

                    new_controller.gamepad.gamepad_data   = SDL_OpenGamepad(new_controller.gamepad.gamepadID);
                    new_controller.gamepad.has_rumble     = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0x1, 0x1, 1);
                    new_controller.gamepad.stick_deadzone = INPUT_MANAGER_GAMEPAD_DEFAULT_DEADZONE;

                    event_handler.controllers[event_handler.connected_controller_count++] = new_controller;
                    log_info("Controller '%s' connected...\n", SDL_GetGamepadName(new_controller.gamepad.gamepad_data));
                }break;
                case SDL_EVENT_GAMEPAD_REMOVED:
                {
                    s32 index = 0;
                    input_controller_t *controller = find_controller_by_ID(&event_handler, event.gdevice.which, &index);
                    SDL_CloseGamepad(controller->gamepad.gamepad_data);
                    ZeroStruct(*controller);

                    controller->ID = -1;
                    controller->controller_index = -1;

                    c_array_remove(event_handler.controllers, index, event_handler.connected_controller_count);
                    --event_handler.connected_controller_count;
                }break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                {
                    input_controller_t *controller = find_controller_by_ID(&event_handler, event.gbutton.which, null);

                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
                    input_event.inputID       = event.gbutton.button;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.type = INPUT_EVENT_TYPE_PRESSED;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);

                    input_event.inputID       = event.gbutton.button;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks() + 4;
                    input_event.type = INPUT_EVENT_TYPE_DOWN;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    input_controller_t *controller = find_controller_by_ID(&event_handler, event.gbutton.which, null);

                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
                    input_event.inputID       = event.gbutton.button;
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.type          = INPUT_EVENT_TYPE_RELEASED;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                {
                    input_controller_t *controller = find_controller_by_ID(&event_handler, event.gbutton.which, null);
                    input_event_t input_event = {};
                    input_event.input_type    = INPUT_CONTROLLER_TYPE_GAMEPAD;
                    input_event.inputID       = SDL_axis_to_input_axis(event.gaxis.axis);
                    input_event.controllerID  = controller->ID;
                    input_event.timestampMS   = SDL_GetTicks();
                    input_event.type          = INPUT_EVENT_TYPE_AXIS_MOVED;
                    input_event.axis_value    = event.gaxis.value;

                    append_input_event(&event_handler.events, &event_handler.event_count, &input_event);
                }break;
                case SDL_EVENT_QUIT:
                {
                    goto exit;
                }break;
            }
        }
         
        // NOTE(Sleepster): Consume redundant events
        u32 redundant_events[MAX_INPUT_EVENTS] = {};
        u32 events_to_remove = 0;

        for(s32 event_index = 0;
            event_index < event_handler.event_count - 1;
            ++event_index)
        {
            input_event_t *event      = event_handler.events + (event_index);
            input_event_t *next_event = event_handler.events + (event_index + 1);
            if(is_same_event(event, next_event))
            {
                redundant_events[events_to_remove++] = (event_index + 1);
                if(event->input_stream.data == null && next_event->input_stream.data != null)
                {
                    event->input_stream = next_event->input_stream;
                }
            }
        }

        for(u32 removal_index = 0;
            removal_index < events_to_remove;
            ++removal_index)
        {
            u32 index = redundant_events[removal_index];
            c_array_remove(event_handler.events, index, event_handler.event_count - removal_index);
        }

        event_handler.event_count -= events_to_remove;

        // NOTE(Sleepster): Dispatch to the according controller 
        u64 most_recent_timestamp = 0;
        for(s32 event_index = 0;
            event_index < event_handler.event_count;
            ++event_index)
        {
            input_event_t *event = event_handler.events + event_index;
            input_controller_t *controller = null;
            if(event->input_type == INPUT_CONTROLLER_TYPE_KEYBOARD) controller = find_controller_by_ID(&event_handler, event->controllerID, null);
            else                                                    controller = find_controller_by_ID(&event_handler, event->controllerID, null);
            Assert(controller);

            append_input_event(&controller->events, &controller->event_count, event);
            if(event->timestampMS >= most_recent_timestamp)
            {
                event_handler.active_controller_index = controller->controller_index;
                most_recent_timestamp = event->timestampMS;
            }
        }
        event_handler.event_count = 0;

        // NOTE(Sleepster): Simulate 
        if(delta_time >= (gc->tick_rate * 2.0f))
        {
            delta_time = gc->tick_rate * 2.0f;
        }

        dt_accumulator += delta_time;
        while(dt_accumulator >= gc->tick_rate)
        {
            input_controller_t *controller = event_handler.controllers + event_handler.active_controller_index;
            // NOTE(Sleepster): (fake) UI LOOP 
            for(s32 event_index = 0;
                event_index < controller->event_count;
                ++event_index)
            {
                input_event_t *event = controller->events + event_index;
                if(!event->consumed)
                {
                    switch(event->type)
                    {
                        case INPUT_EVENT_TYPE_PRESSED:
                        {
                            printf("Simulated UI Event: '%c' %s...\n", event->inputID, "Pressed");
                        }break;
                        case INPUT_EVENT_TYPE_DOWN:
                        {
                            printf("Simulated UI Event: '%c' '%s'...\n", event->inputID, "Down");
                        }break;
                        case INPUT_EVENT_TYPE_RELEASED:
                        {
                            printf("Simulated UI Event: '%c' '%s'...\n", event->inputID, "Released");
                        }break;
                        case INPUT_EVENT_TYPE_AXIS_MOVED:
                        {
                            printf("Simulated UI Event: '%s'...\n", "Axis Motion");
                        }break;
                    }
                }
            }

#if 0
            // NOTE(Sleepster): Resolve the game_actions
            for(game_action_t &action: event_handler.game_actions)
            {
                action.axis2D_value  = vec2_zero();
                action.axis1D_value  = 0.0f;
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
            printf("Axis value: '%.02f', '%.02f'...\n", movement_action->axis2D_value.x, movement_action->axis2D_value.y);
#endif
            controller->event_count = 0;
            if(jump_action->button_flags & INPUT_MANAGER_ACTION_BUTTON_FLAG_PRESSED)
            {
                printf("jumped pressed\n");
            }

            dt_accumulator -= gc->tick_rate;
        }
        // NOTE(Sleepster): Simulate 

        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

exit:
    return(0);
}
