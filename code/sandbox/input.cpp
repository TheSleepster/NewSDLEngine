/* ========================================================================
   $File: input.cpp $
   $Date: July 30 2026 12:34 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>
#include <stdio.h>

#include <c_dynarray.h>
#include <c_intrinsics.h>

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
    u32    flags;

    vec2_t last_value;
    vec2_t current_value;
    vec2_t delta_value;
};

#define InputStatePressed(button_flags)  (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED)  != 0)
#define InputStateDown(button_flags)     (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_DOWN)     != 0)
#define InputStateReleased(button_flags) (((button_flags) & INPUT_MANAGER_INPUT_STATE_FLAG_RELEASED) != 0)

ENGINE_API inline u32            s_im_controller_get_input_state_flags(input_controller_t *controller, u32 inputID);
ENGINE_API inline input_state_t *s_im_controller_get_input_state(input_controller_t *controller, u32 inputID);
ENGINE_API inline bool8          s_im_is_input_button_pressed(input_controller_t *controller, u32 inputID);
ENGINE_API inline bool8          s_im_is_input_button_down(input_controller_t *controller, u32 inputID);
ENGINE_API inline bool8          s_im_is_input_button_released(input_controller_t *controller, u32 inputID);

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

using input_device_array_t = fixed_array_t<input_device_t, MAX_INPUT_DEVICES>;
struct input_manager_t 
{
    bool8                initialized;
    input_device_t      *primary_device;

    input_device_array_t devices;
    s32                  connected_device_count;

    dynarray_t<input_action_t> input_actions;
};

ENGINE_API inline input_device_t *s_im_find_first_device_of_type(input_manager_t *input_manager, s32 type);
ENGINE_API input_controller_t     s_im_init_input_controller(input_manager_t *input_manager);
ENGINE_API input_event_t         *s_im_read_next_event_from_controller(input_controller_t *controller);
ENGINE_API void                   s_im_controller_update_state(input_manager_t *input_manager, input_controller_t *controller, bool8 consume);

/* 
====================================================
FUNCTIONS
==================================================== 
*/

internal_api inline input_device_t*
get_or_create_input_device(input_manager_t *input_manager, s32 deviceID, s32 type)
{
    input_device_t *result = null;

    input_device_t *found = null;
    for(s32 device_index = 0;
        device_index < input_manager->connected_device_count;
        ++device_index)
    {
        input_device_t &device = input_manager->devices[device_index];
        if(((device.deviceID == deviceID) || (device.deviceID == 0)) && device.type == type)
        {
            found = &device;
            break;
        }
    }

    result = found;
    if(!result && input_manager->connected_device_count < MAX_INPUT_DEVICES)
    {
        result = input_manager->devices + input_manager->connected_device_count++;
        result->deviceID = deviceID;
        result->type     = type;
    }
    else
    {
        if(result->deviceID == (s32)INVALID_ID && deviceID != 0)
        {
            result->deviceID = deviceID;
        }
    }
    
    // NOTE(Sleepster): Most recently requested device becomes the "primary" device 
    input_manager->primary_device = result;

    Assert(result != null);
    return(result);
}

internal_api inline void
append_event_to_device(input_device_t *device, input_event_t *event)
{
    device->input_events[device->next_event_to_write] = *event;
    device->next_event_to_write = ((device->next_event_to_write + 1) % device->input_events.count);
}

ENGINE_API input_device_t*
s_im_find_first_device_of_type(input_manager_t *input_manager, s32 type)
{
    input_device_t *result = null;
    for(s32 device_index = 0;
        device_index < input_manager->connected_device_count;
        ++device_index)
    {
        input_device_t &device = input_manager->devices[device_index];
        if(device.type == type)
        {
            result = &device;
            break;
        }
    }

    return(result);
}

internal_api inline input_device_t*
get_first_valid_keyboard_device(input_manager_t *input_manager, u32 ID)
{
    input_device_t *result = null;
    for(s32 device_index = 0;
        device_index < input_manager->connected_device_count;
        ++device_index)
    {
        input_device_t &device = input_manager->devices[device_index];
        if(device.type == INPUT_DEVICE_TYPE_KEYBOARD && 
          ((device.keyboard.mouseID == 0) || (device.keyboard.mouseID == ID)))
        {
            result = &device;
            break;
        }
    }

    if(!result)
    {
        result = get_or_create_input_device(input_manager, 0, INPUT_DEVICE_TYPE_KEYBOARD);
        result->keyboard.mouseID = ID;
    }

    return(result);
}

ENGINE_API input_controller_t
s_im_init_input_controller(input_manager_t *input_manager)
{
    input_controller_t result = {};
    result.owner_device = input_manager->primary_device;

    return(result);
}

ENGINE_API input_event_t*
s_im_read_next_event_from_controller(input_controller_t *controller)
{
    input_event_t *result = null;
    if(controller->next_controller_event_to_read != controller->next_controller_event_to_write)
    {
        result = controller->input_events + controller->next_controller_event_to_read;
        controller->next_controller_event_to_read = ((controller->next_controller_event_to_read + 1) % controller->input_events.count);
    }

    return(result);
}

ENGINE_API void
s_im_controller_update_state(input_manager_t *input_manager, input_controller_t *controller, bool8 consume)
{
    // NOTE(Sleepster): Update controller's target device 
    input_device_t *old_device = controller->owner_device;
    controller->owner_device   = input_manager->primary_device;
    if(old_device != null && controller->owner_device != old_device)
    {
        controller->next_controller_event_to_read  = 0;
        controller->next_controller_event_to_write = 0;
        controller->next_device_event_to_read      = 0;

        // TODO(Sleepster): I don't THINK this is needed... 
        //ZeroMemory(controller->input_events.items, sizeof(input_event_t) * controller->input_events.count);
    }

    // NOTE(Sleepster): Add events from the device to the controller 
    input_device_t *device = controller->owner_device;
    if(device && device->deviceID != (s32)INVALID_ID)
    {
        s32 events_read = 0;
        for(s32 event_index = controller->next_device_event_to_read;
            event_index != device->next_event_to_write;
            ++event_index)
        {
            s32 source_index = (controller->next_device_event_to_read + events_read) % controller->input_events.count;

            input_event_t *source      = device->input_events + source_index;
            input_event_t *destination = controller->input_events + controller->next_controller_event_to_write;
            if(!source->consumed)
            {
                *destination = *source;
                if(consume) source->consumed = true;

                controller->next_controller_event_to_write = ((controller->next_controller_event_to_write + 1) % controller->input_events.count);
                ++events_read;
            }
        }

        controller->next_device_event_to_read = ((controller->next_device_event_to_read + events_read) % controller->input_events.count);
    }

    // NOTE(Sleepster): Decay transient button states
    for(u32 button_index = 0;
        button_index < controller->transient_data.pressed_button_count;
        ++button_index)
    {
        input_state_t *button = controller->transient_data.buttons_pressed[button_index];
        button->flags &= ~INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED;
    }

    for(u32 button_index = 0;
        button_index < controller->transient_data.released_button_count;
        ++button_index)
    {
        input_state_t *button = controller->transient_data.buttons_released[button_index];
        button->flags &= ~INPUT_MANAGER_INPUT_STATE_FLAG_RELEASED;
    }

    controller->transient_data.pressed_button_count  = 0;
    controller->transient_data.released_button_count = 0;

    // NOTE(Sleepster): Update state button states with the new events 
    input_event_t *event = null;
    do {
        event = s_im_read_next_event_from_controller(controller);
        if(event)
        {
            switch(event->type)
            {
                case INPUT_EVENT_TYPE_KEY_DOWN:
                case INPUT_EVENT_TYPE_GAMEPAD_BUTTON_DOWN:
                case INPUT_EVENT_TYPE_MOUSE_DOWN:
                {
                    input_state_t *button = controller->inputs + event->inputID;
                    button->flags  |= (INPUT_MANAGER_INPUT_STATE_FLAG_DOWN|INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED);
                    button->inputID = event->inputID;

                    controller->transient_data.buttons_pressed[controller->transient_data.pressed_button_count++] = button;
                }break;
                case INPUT_EVENT_TYPE_KEY_UP:
                case INPUT_EVENT_TYPE_GAMEPAD_BUTTON_UP:
                case INPUT_EVENT_TYPE_MOUSE_UP:
                {
                    input_state_t *button = controller->inputs + event->inputID;
                    button->flags  |=  (INPUT_MANAGER_INPUT_STATE_FLAG_RELEASED);
                    button->flags  &= ~(INPUT_MANAGER_INPUT_STATE_FLAG_DOWN);
                    button->inputID = event->inputID;

                    controller->transient_data.buttons_released[controller->transient_data.released_button_count++] = button;
                }break;
                case INPUT_EVENT_TYPE_TEXT_INPUT:
                {
                    //printf("%.*s...\n", fprint_string(event->text));
                }break;
            }
        }
    }while(event);

    // NOTE(Sleepster): Update controller information that must derive from device state 
    if(device)
    {
        input_state_t *device_source          = device->input_axis_info_array.items;
        input_state_t *controller_destination = controller->inputs + INPUT_ARRAY_INPUT_AXIS_OFFSET;

        memcpy(controller_destination, device_source, sizeof(input_state_t) * device->input_axis_info_array.count);
#if 0
        if(device->type == INPUT_DEVICE_TYPE_KEYBOARD)
        {
            s32 device_axis_index = INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT - INPUT_ARRAY_INPUT_AXIS_OFFSET;
            input_state_t *device_state     = device->input_axis_info_array + device_axis_index;
            input_state_t *controller_state = controller->inputs + INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT;

            controller_state->current_value = device_state->current_value;
            controller_state->last_value    = device_state->last_value; 
            controller_state->delta_value   = device_state->delta_value; 
        }

        if(device->type == INPUT_DEVICE_TYPE_GAMEPAD)
        {
        }
#endif
    }
}

ENGINE_API inline u32
s_im_controller_get_input_state_flags(input_controller_t *controller, u32 inputID)
{
    u32 result = 0;
    result = (controller->inputs + inputID)->flags;
    return(result);
}

ENGINE_API inline input_state_t*
s_im_controller_get_input_state(input_controller_t *controller, u32 inputID)
{
    input_state_t *result = controller->inputs + inputID;
    return(result);
}

ENGINE_API inline bool8
s_im_is_input_button_pressed(input_controller_t *controller, u32 inputID)
{
    bool8 result = false;

    u32 input_flags = s_im_controller_get_input_state_flags(controller, inputID);
    result = InputStatePressed(input_flags);

    return(result);
}

ENGINE_API inline bool8
s_im_is_input_button_down(input_controller_t *controller, u32 inputID)
{
    bool8 result = false;

    u32 input_flags = s_im_controller_get_input_state_flags(controller, inputID);
    result = InputStateDown(input_flags);

    return(result);
}

ENGINE_API inline bool8
s_im_is_input_button_released(input_controller_t *controller, u32 inputID)
{
    bool8 result = false;

    u32 input_flags = s_im_controller_get_input_state_flags(controller, inputID);
    result = InputStateReleased(input_flags);

    return(result);
}

ENGINE_API input_action_t*
s_im_input_action_create(input_manager_t *input_manager, s32 type)
{
    input_action_t *result = null;
    input_action_t new_action = {
        .type = type,
    };

    c_dynarray_add(&input_manager->input_actions, &new_action);
    result = &input_manager->input_actions[Max(input_manager->input_actions.used - 1, 0)];

    return(result);
}

ENGINE_API void
s_im_input_action_add_mapping(input_action_t *action, input_action_mapping_t *mapping)
{
    action->mappings[action->mapping_count++] = *mapping;
}

internal_api input_action_mapping_t*
get_valid_action_mapping(input_controller_t *controller, input_action_t *action)
{
    input_action_mapping_t *result = null;
    if(controller->owner_device)
    {
        for(u32 mapping_index = 0;
            mapping_index < action->mapping_count;
            ++mapping_index)
        {
            input_action_mapping_t *found = action->mappings + mapping_index;
            if(found->device_type == controller->owner_device->type)
            {
                result = found;
                break;
            }
        }
    }

    return(result);
}

internal_api void
input_action_update_button_state(input_controller_t *controller, input_action_t *action)
{
    input_action_mapping_t *valid_mapping = get_valid_action_mapping(controller, action);
    if(valid_mapping)
    {
        input_state_t *input_state = s_im_controller_get_input_state(controller, valid_mapping->bindings[0].inputID);

        // TODO(Sleepster): Modifiers 
        bool8 modifiers_set = true;
        if(valid_mapping->bindings[0].required_modifiers != 0)
        {
            // check modifiers
        }

        if(modifiers_set)
        {
            action->button_flags = input_state->flags;
        }
    }
}

internal_api void
input_action_update_axis1D_state(input_controller_t *controller, input_action_t *action)
{
    input_action_mapping_t *valid_mapping = get_valid_action_mapping(controller, action);
    if(valid_mapping)
    {
        switch(controller->owner_device->type)
        {
            case INPUT_DEVICE_TYPE_KEYBOARD:
            {
                input_state_t *button0 = s_im_controller_get_input_state(controller, valid_mapping->bindings[0].inputID);
                input_state_t *button1 = s_im_controller_get_input_state(controller, valid_mapping->bindings[1].inputID);

                action->axis1D_value = 0.0f;

                if(InputStateDown(button0->flags)) action->axis1D_value =  1.0f;
                if(InputStateDown(button1->flags)) action->axis1D_value = -1.0f;
            }break;
            case INPUT_DEVICE_TYPE_GAMEPAD:
            {
                input_state_t *axis_state = s_im_controller_get_input_state(controller, valid_mapping->bindings[0].inputID);
                action->axis1D_value = axis_state->current_value.x;
            }break;
        }
    }
}

internal_api void
input_action_update_axis2D_state(input_controller_t *controller, input_action_t *action)
{
    input_action_mapping_t *valid_mapping = get_valid_action_mapping(controller, action);
    if(valid_mapping)
    {
        switch(controller->owner_device->type)
        {
            case INPUT_DEVICE_TYPE_KEYBOARD:
            {
                input_state_t *button0 = s_im_controller_get_input_state(controller, valid_mapping->bindings[0].inputID);
                input_state_t *button1 = s_im_controller_get_input_state(controller, valid_mapping->bindings[1].inputID);
                input_state_t *button2 = s_im_controller_get_input_state(controller, valid_mapping->bindings[2].inputID);
                input_state_t *button3 = s_im_controller_get_input_state(controller, valid_mapping->bindings[3].inputID);

                action->axis2D_value = vec2_zero();

                if(InputStateDown(button0->flags)) action->axis2D_value.y += -1.0f;
                if(InputStateDown(button1->flags)) action->axis2D_value.y +=  1.0f;
                if(InputStateDown(button2->flags)) action->axis2D_value.x += -1.0f;
                if(InputStateDown(button3->flags)) action->axis2D_value.x +=  1.0f;
            }break;
            case INPUT_DEVICE_TYPE_GAMEPAD:
            {
                input_state_t *Xaxis = s_im_controller_get_input_state(controller, valid_mapping->bindings[0].inputID);
                input_state_t *Yaxis = s_im_controller_get_input_state(controller, valid_mapping->bindings[1].inputID);

                action->axis2D_value = {Xaxis->current_value.x, Yaxis->current_value.x};
            }break;
        }
    }
}

ENGINE_API void 
s_im_input_action_update_state(input_manager_t *input_manager, input_controller_t *controller)
{
    for(input_action_t &action: input_manager->input_actions)
    {
        switch(action.type)
        {
            case INPUT_ACTION_TYPE_BUTTON: { input_action_update_button_state(controller, &action); }break;
            case INPUT_ACTION_TYPE_AXIS1D: { input_action_update_axis1D_state(controller, &action); }break;
            case INPUT_ACTION_TYPE_AXIS2D: { input_action_update_axis2D_state(controller, &action); }break;
        }
    }
}

int
main(void)
{   
    c_global_context_init();
    input_manager_t *input_manager = Alloc(input_manager_t);

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD);

    SDL_Window *window     = SDL_CreateWindow("Test Window", 800, 600, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, null);
    SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED);

    // TODO(Sleepster): SDL is retarded and for some reason believes that just enabling their text input stuff means
    // you want a stupid text input window on some platforms because they hate life. We'll unfortunately need to replace this eventually
    SDL_ShowWindow(window);
    SDL_StartTextInput(window);

    input_controller_t game_controller       = s_im_init_input_controller(input_manager);
    input_controller_t simulation_controller = s_im_init_input_controller(input_manager);

    input_action_t *movement_action = s_im_input_action_create(input_manager, INPUT_ACTION_TYPE_AXIS2D);
    input_action_mapping_t keyboard_movement_mapping = {
        .bindings = {
            {{SDL_SCANCODE_W}, {SDL_SCANCODE_S}, {SDL_SCANCODE_A}, {SDL_SCANCODE_D}}
        },
        .binding_count = 4,
        .device_type   = INPUT_DEVICE_TYPE_KEYBOARD
    };

    input_action_mapping_t gamepad_movement_mapping = {
        .bindings = {
            {{INPUT_ARRAY_INPUT_AXIS_GAMEPAD_LEFTX}, {INPUT_ARRAY_INPUT_AXIS_GAMEPAD_LEFTY}}
        },
        .binding_count = 2,
        .device_type   = INPUT_DEVICE_TYPE_GAMEPAD
    };

    s_im_input_action_add_mapping(movement_action, &keyboard_movement_mapping);
    s_im_input_action_add_mapping(movement_action, &gamepad_movement_mapping);

    input_action_t *jump_action = s_im_input_action_create(input_manager, INPUT_ACTION_TYPE_BUTTON);
    input_action_mapping_t keyboard_jump_mapping = {
        .bindings = {
            {{SDL_SCANCODE_SPACE}}
        },
        .binding_count = 1,
        .device_type   = INPUT_DEVICE_TYPE_KEYBOARD
    };

    input_action_mapping_t gamepad_jump_mapping = {
        .bindings = {
            {{SDL_GAMEPAD_BUTTON_SOUTH}}
        },
        .binding_count = 1,
        .device_type   = INPUT_DEVICE_TYPE_GAMEPAD
    };

    s_im_input_action_add_mapping(jump_action, &keyboard_jump_mapping);
    s_im_input_action_add_mapping(jump_action, &gamepad_jump_mapping);

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    u64 last_tsc        = SDL_GetPerformanceCounter();
    u64 current_tsc     = 0;
    u64 delta_tsc       = 0;

    float32 delta_time     = 0.0f;
    float64 dt_accumulator = 0.0;
    for(;;)
    {
        SDL_Event event = {};
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_EVENT_KEY_DOWN:
                {
                    u32 ID = event.kdevice.which;
                    if(ID != 0)
                    {
                        input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_KEYBOARD); 
                        if(device)
                        {
                            // NOTE(Sleepster): Only register the PRESSED event
                            if(!event.key.repeat)
                            {
                                input_event_t new_event = {};
                                new_event.type      = INPUT_EVENT_TYPE_KEY_DOWN;
                                new_event.timestamp = rdtsc();
                                new_event.inputID   = event.key.scancode;

                                append_event_to_device(device, &new_event);
                            }
                        }
                    }
                }break;
                case SDL_EVENT_KEY_UP:
                {
                    u32 ID = event.kdevice.which;
                    if(ID != 0)
                    {
                        input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_KEYBOARD); 
                        if(device)
                        {
                            input_event_t new_event = {};
                            new_event.type      = INPUT_EVENT_TYPE_KEY_UP;
                            new_event.timestamp = rdtsc();
                            new_event.inputID   = event.key.scancode;

                            append_event_to_device(device, &new_event);
                        }
                    }
                }break;
                case SDL_EVENT_TEXT_INPUT:
                {
                    input_device_t *device = get_first_valid_keyboard_device(input_manager, 0); 
                    if(device)
                    {
                        input_event_t new_event = {};
                        new_event.type      = INPUT_EVENT_TYPE_TEXT_INPUT;
                        new_event.timestamp = rdtsc();
                        new_event.text      = c_string_make_copy(&gc->simulation_arena, STR(event.text.text));

                        append_event_to_device(device, &new_event);
                    }
                }break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    s32 ID = event.button.which;

                    input_device_t *device = get_first_valid_keyboard_device(input_manager, ID);

                    input_event_t new_event = {};
                    new_event.type      = INPUT_EVENT_TYPE_MOUSE_DOWN;
                    new_event.timestamp = rdtsc();
                    new_event.inputID   = SDL_SCANCODE_COUNT + event.button.button;

                    append_event_to_device(device, &new_event);
                }break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    s32 ID = event.button.which;

                    input_device_t *device = get_first_valid_keyboard_device(input_manager, ID);

                    input_event_t new_event = {};
                    new_event.type      = INPUT_EVENT_TYPE_MOUSE_UP;
                    new_event.timestamp = rdtsc();
                    new_event.inputID   = SDL_SCANCODE_COUNT + event.button.button;

                    append_event_to_device(device, &new_event);
                }break;
                case SDL_EVENT_MOUSE_MOTION:
                {
                    s32 ID = event.motion.which;
                    // NOTE(Sleepster): We process this in place because some data MUST be owned by the device.
                    // If the calculations of the mouse delta were on each and every controller then we would have a massive problem
                    // with the polling. Some examples being:
                    // 1.) Controller A & Controller B will poll on the same frame. When they do controller A initialally calculates the delta correctly,
                    //     however controller B will ALSO attempt to calculate the mouse delta for the frame causing the data of the mouse delta to then be 
                    //     incorrect. 
                    //
                    // 2.) Each contorller then has a completely different delta upon their polling
                    //
                    //
                    // Both of these problems have the root cause of "trying to defer the update of the mouse data to later and handing ownership of the 
                    // hardware state to that of the controller" and this is stupid. There is a world where a simple "is_updated" on the mouse delta would fix this, but this kinda doesn't sit right with me
                    // so some information about the RAW device will instead be owned by the device.
                    //
                    // Devices own their own state -> controllers derive from their state
                    input_device_t *device = get_first_valid_keyboard_device(input_manager, ID);
                    if(device)
                    {
                        s32 index = INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT - INPUT_ARRAY_INPUT_AXIS_OFFSET;
                        input_state_t *state = device->input_axis_info_array + index;

                        state->last_value    = state->current_value;
                        state->current_value = vec2(event.motion.x, event.motion.y);
                        state->delta_value   = vec2_subtract(state->current_value, state->last_value);
                    }
                }break;
                case SDL_EVENT_MOUSE_WHEEL:
                {
                    s32 ID = event.button.which;
                    input_device_t *device = get_first_valid_keyboard_device(input_manager, ID);
                    if(device)
                    {
                        s32 index = INPUT_ARRAY_INPUT_AXIS_MOUSE_WHEEL - INPUT_ARRAY_INPUT_AXIS_OFFSET;
                        input_state_t *state = device->input_axis_info_array + index;

                        vec2_t new_mouse_wheel;
                        float32 flip_value = (event.wheel.direction == SDL_MOUSEWHEEL_NORMAL) ? 1.0f : -1.0f;

                        new_mouse_wheel.x = event.wheel.x * flip_value; 
                        new_mouse_wheel.y = event.wheel.y * flip_value; 

                        state->last_value    = state->current_value;
                        state->current_value = new_mouse_wheel;
                        state->delta_value   = vec2_subtract(state->current_value, state->last_value);
                    }
                }break;
                case SDL_EVENT_GAMEPAD_ADDED:
                {
                    u32 ID = event.gdevice.which;

                    input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_GAMEPAD); 
                    device->deviceID = ID;
                    device->gamepad.handle = SDL_OpenGamepad(event.gdevice.which);
                    if(!device->gamepad.handle)
                    {
                        log_error("Failure opening a gamepad controller... SDL_Error: '%s'..\n", SDL_GetError());
                    }

                    device->gamepad.has_rumble = SDL_RumbleGamepad(device->gamepad.handle, 0x1, 0x1, 1);
                    device->gamepad.deadzone   = DEFAULT_GAMEPAD_DEADZONE;

                    log_info("Controller '%s' connected...\n", SDL_GetGamepadName(device->gamepad.handle));
                }break;
                case SDL_EVENT_GAMEPAD_REMOVED:
                {
                    u32 ID = event.gdevice.which;

                    input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_GAMEPAD);
                    log_info("Controller '%s' disconnected...\n", SDL_GetGamepadName(device->gamepad.handle));

                    SDL_CloseGamepad(device->gamepad.handle);
                    ZeroStruct(*device);

                    device->deviceID = -1;
                }break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                {
                    // TODO(Sleepster): I don't think we need to check for zero? SDL's docs don't say anything about this being able to be a 0
                    // https://wiki.libsdl.org/SDL3/SDL_GamepadButtonEvent
                    u32 ID = event.gbutton.which;
                    input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_GAMEPAD); 

                    input_event_t new_event = {};
                    new_event.type      = INPUT_EVENT_TYPE_GAMEPAD_BUTTON_DOWN;
                    new_event.inputID   = (u32)event.gbutton.button;
                    new_event.timestamp = rdtsc();

                    append_event_to_device(device, &new_event);
                }break;
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                {
                    u32 ID = event.gbutton.which;
                    input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_GAMEPAD); 

                    input_event_t new_event = {};
                    new_event.type      = INPUT_EVENT_TYPE_GAMEPAD_BUTTON_UP;
                    new_event.inputID   = (u32)event.gbutton.button;
                    new_event.timestamp = rdtsc();

                    append_event_to_device(device, &new_event);
                }break;
                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                {
                    u32 ID = event.gaxis.which;
                    input_device_t *device = get_or_create_input_device(input_manager, ID, INPUT_DEVICE_TYPE_GAMEPAD); 

                    s32 axis_index = event.gaxis.axis;
                    input_state_t *input_state = device->input_axis_info_array + axis_index;

                    // TODO(Sleepster): Maybe not how we want to do delta, I can see issues arising where the stick snaps from
                    // 13858 -> 0 instantly which would probably not be good...
                    float32 normalizer = event.gaxis.value < 0.0f ? 32768.0f : 32767.0f;
                    float32 axis_normalized = Clamp(event.gaxis.value / normalizer, -1.0, 1.0);
                    if(abs(axis_normalized) < device->gamepad.deadzone)
                    {
                        axis_normalized = 0.0f;
                    }

                    input_state->last_value    = input_state->current_value;
                    input_state->current_value = vec2(axis_normalized, 0.0f);
                    input_state->delta_value   = vec2_subtract(input_state->current_value, input_state->last_value);
                }break;
                case SDL_EVENT_QUIT:
                {
                    goto exit;
                }break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderClear(renderer);

        const u32 test_inputID = SDL_SCANCODE_LEFT_MOUSE;

        s_im_controller_update_state(input_manager, &game_controller, false);
        s_im_input_action_update_state(input_manager, &game_controller);

#if 0
        if()
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_FRect rect = {400, 340, 20, 20};
            SDL_RenderFillRect(renderer, &rect);
        }
#endif

        if(InputActionPressed(jump_action))
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_FRect rect = {360, 320, 20, 20};
            SDL_RenderFillRect(renderer, &rect);
        }

        if(InputActionDown(jump_action))
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_FRect rect = {400, 320, 20, 20};
            SDL_RenderFillRect(renderer, &rect);
        }

        if(InputActionReleased(jump_action))
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_FRect rect = {440, 320, 20, 20};
            SDL_RenderFillRect(renderer, &rect);
        }

        static vec2_t position = {};
        position.x += movement_action->axis2D_value.x;
        position.y += movement_action->axis2D_value.y;

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect rect = {position.x - 10, position.y - 10, 20, 20};
        SDL_RenderFillRect(renderer, &rect);

        // NOTE(Sleepster): Simulate 
        if(delta_time >= (gc->tick_rate * 2.0f))
        {
            delta_time = gc->tick_rate * 2.0f;
        }

        dt_accumulator += delta_time;
        while(dt_accumulator >= gc->tick_rate)
        {
            s_im_controller_update_state(input_manager, &simulation_controller, false);
            s_im_input_action_update_state(input_manager, &simulation_controller);

            input_state_t *state = s_im_controller_get_input_state(&simulation_controller, INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_FRect rect = {state->current_value.x - 10, state->current_value.y - 10, 20, 20};
            SDL_RenderFillRect(renderer, &rect);

            if(s_im_is_input_button_pressed(&simulation_controller, test_inputID))
            {
                SDL_SetRenderDrawColor(renderer, 127, 255, 127, 255);
                SDL_FRect rect = {360, 300, 20, 20};
                SDL_RenderFillRect(renderer, &rect);
            }

            if(s_im_is_input_button_down(&simulation_controller, test_inputID))
            {
                SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                SDL_FRect rect = {400, 300, 20, 20};
                SDL_RenderFillRect(renderer, &rect);
            }

            if(s_im_is_input_button_released(&simulation_controller, test_inputID))
            {
                SDL_SetRenderDrawColor(renderer, 127, 0, 255, 255);
                SDL_FRect rect = {440, 300, 20, 20};
                SDL_RenderFillRect(renderer, &rect);
            }

            c_arena_reset(&gc->simulation_arena);
            dt_accumulator -= gc->tick_rate;
        }
        // NOTE(Sleepster): Simulate 

        SDL_RenderPresent(renderer);
        SDL_Delay(7);

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

exit:
    return(0);
}
