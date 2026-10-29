/* ========================================================================
   $File: s_map_editor.cpp $
   $Date: September 25 2026 08:13 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
constexpr u32 MAX_EDITOR_UNDO_ACTION_COUNT = 1024;
constexpr u32 MAX_SELECTED_EDITOR_ENTITIES = 256;

enum editor_event_type_t
{
    EDITOR_EVENT_TYPE_CREATE_ENTITY,
    EDITOR_EVENT_TYPE_DESTROY_ENTITY,
};

struct editor_event_t 
{
    u32               ID;
    u32               type;
    union {
        array_t<entity_t>  entities_to_create;
        array_t<entity_t*> entities_to_destroy;
    };
};

struct editor_event_buffer_t 
{
    memory_arena_t                    *editor_arena;

    fixed_array_t<editor_event_t, 128> event_list;
    s32                                current_event_count;

    editor_event_buffer_t             *next_buffer;
};

using editor_selected_entity_array_t = fixed_array_t<entity_t*, MAX_SELECTED_EDITOR_ENTITIES>;
struct map_editor_t
{
    memory_arena_t                 editor_arena;
    memory_arena_t                 undo_arena;
    memory_arena_t                 redo_arena;

    // NOTE(Sleepster): This is definitely okay here since this needs to:
    // A.) Modify the game_state to actual manipulate the game
    // B.) Retrieve information related to the assets tiles and entities use
    game_state_t                  *game_state;
    asset_manager_t               *asset_manager;

    bool8                          show_panel;
    bool8                          show_grid;
    bool8                          show_player_viewport;
    bool8                          boxing;
    bool8                          box_placing;
    bool8                          box_removing;

    u32                            current_editor_mode;

    float32                        zoom;
    RHI_render_camera_t            camera;
    input_controller_t            *controller;

    asset_handle_t                 gizmo_image;
    asset_handle_t                 mouse_cursor_image;

    editor_selected_entity_array_t selected_entities;
    s32                            selected_entity_count;

    editor_event_buffer_t          first_undo_event_buffer;
    editor_event_buffer_t          first_redo_event_buffer;

    vec2_t                         boxing_start;
    vec2_t                         boxing_end;
};

internal_api map_editor_t*
s_map_editor_create(game_state_t *game_state, input_manager_t *input_manager, asset_manager_t *asset_manager, render_state_t *render_state)
{
    map_editor_t *result     = (map_editor_t*)c_alloc(sizeof(map_editor_t), ALLOCATOR_TAG_GAME);

    result->editor_arena     = c_arena_create(MB(50), ALLOCATOR_TAG_GAME);
    result->undo_arena       = c_arena_create(MB(50), ALLOCATOR_TAG_GAME);
    result->redo_arena       = c_arena_create(MB(50), ALLOCATOR_TAG_GAME);
    result->controller       = s_im_init_input_controller(input_manager);

    result->game_state    = game_state;
    result->asset_manager = asset_manager;

    result->first_undo_event_buffer.editor_arena = &result->editor_arena;
    result->first_redo_event_buffer.editor_arena = &result->editor_arena;

    result->zoom                 = 2.5f;
    result->show_grid            = true;
    result->show_player_viewport = true;
    result->camera = {
        .viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                         Max(render_state->RHI_context->window_size.y, 10)),
        .translation = {0.0f, 0.0f},
        .zoom = result->zoom 
    };

    result->gizmo_image        = s_asset_manager_acquire_asset_handle(asset_manager, STR("editor_gizmo"));
    result->mouse_cursor_image = s_asset_manager_acquire_asset_handle(asset_manager, STR("editor_mouse_cursor"));

    s_asset_manager_insert_image_into_atlas(asset_manager, &result->gizmo_image);
    s_asset_manager_insert_image_into_atlas(asset_manager, &result->mouse_cursor_image);

    return(result);
}

internal_api void 
s_map_editor_handle_ui(map_editor_t *editor, ui_state_t *main_ui, RHI_context_t *RHI_context)
{
    if(editor->show_panel)
    {
        ui_signal_t main_panel = ui_widget_panel(main_ui, 
                                                 STR("Editor Panel"), 
                                                 vec2_multiply(RHI_context->window_size, vec2(-0.5, 0.5)), 
                                                 vec2(20, 20), 
                                                 vec2(10.0f, 0.0f), 
                                                 vec4(10, 10, 10, 10), 
                                                 vec4(0.4, 0.4, 0.4, 0.6),
                                                 0);
        if(rect2_point_in_rect(main_panel.widget->state->widget_rect, main_ui->mouse_position))
        {
            main_ui->input_focused = true;
        }
        else
        {
            main_ui->input_focused = false;
        }

        ui_column(main_ui, main_panel.widget)
        {
            ui_widget_set_default_font_size(main_ui, 30);
            ui_signal_t title_bar = ui_widget_panel(main_ui, 
                                                    STR("Editor Title bar"), 
                                                    vec2(0, 0), 
                                                    vec2(20, 20),
                                                    vec2(10.0f, 4.0f), 
                                                    vec4(10, 10, 2, 0), 
                                                    vec4(0.0, 0.0, 0.0, 0.5),
                                                    0);
            ui_state_set_active_padding(main_ui, vec4(10, 10, 4, 4));
            ui_signal_t open_menu_button = {};
            ui_row(main_ui, title_bar.widget)
            {
                open_menu_button = ui_widget_sized_button(main_ui, 
                                                          STR("Open Editor Menu"), 
                                                          vec2(20, 20), 0);
                ui_widget_text(main_ui, STR("[Editor Panel]"));
            }

            if(ui_pressed(open_menu_button))
            {
                main_panel.widget->state->toggled = !main_panel.widget->state->toggled;
            }

            if(main_panel.widget->toggled)
            {
                ui_widget_divider(main_ui, 
                                  STR("editor panel divider"), 
                                  vec2(0.9, 5.0f), 
                                  {UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT, UI_WIDGET_SIZE_KIND_PIXELS});
                ui_widget_set_default_font_size(main_ui, 16);
                ui_signal_t sub_panel = ui_widget_panel(main_ui, 
                                                        STR("Editor Subpanel"), 
                                                        vec2(0, 0), 
                                                        vec2(20, 20),
                                                        vec2(10.0f, 10.0f), 
                                                        vec4(10, 10, 10, 0), 
                                                        vec4_zero(),
                                                        0);
                ui_column(main_ui, sub_panel.widget)
                {
                    ui_signal_t tile_grid = ui_widget_labeled_button(main_ui, STR("Toggle Editor Tile Grid"));
                    if(ui_pressed(tile_grid))
                    {
                        editor->show_grid = !editor->show_grid;
                    }

                    ui_signal_t show_viewport = ui_widget_labeled_button(main_ui, STR("Toggle Editor Player Viewport"));
                    if(ui_pressed(show_viewport))
                    {
                        editor->show_player_viewport = !editor->show_player_viewport;
                    }

                    ui_signal_t editor_selection_button_panel = ui_widget_panel(main_ui, 
                                                                                STR("Editor Subpanel"), 
                                                                                vec2(0, 0), 
                                                                                vec2(20, 20),
                                                                                vec2(10.0f, 10.0f), 
                                                                                vec4(10, 10, 10, 0), 
                                                                                vec4_zero(),
                                                                                0);
                    ui_row(main_ui, editor_selection_button_panel.widget)
                    {
                        RHI_set_texture_filter_mode(RHI_context, editor->gizmo_image.texture,        RHI_IMAGE_FILTER_TYPE_NEAREST);
                        RHI_set_texture_filter_mode(RHI_context, editor->mouse_cursor_image.texture, RHI_IMAGE_FILTER_TYPE_NEAREST);
                        ui_signal_t gizmo = ui_widget_textured_button(main_ui, 
                                                                      STR("Editor Gizmo Selection"), 
                                                                      vec2(28.0f, 28.0f), 
                                                                      vec2(5.0f, -10.0f), 
                                                                      editor->gizmo_image, 
                                                                      vec2_zero(), 
                                                                      vec2(editor->gizmo_image.texture->bitmap.width, editor->gizmo_image.texture->bitmap.height),
                                                                      0);
                        if(ui_pressed(gizmo))
                        {
                            editor->show_player_viewport = !editor->show_player_viewport;
                        }

                        ui_signal_t selection = ui_widget_textured_button(main_ui, 
                                                                          STR("Editor Mouse Selection"), 
                                                                          vec2(32.0f, 26.0f), 
                                                                          vec2(5.0f, -10.0f),
                                                                          editor->mouse_cursor_image, 
                                                                          vec2_zero(), 
                                                                          vec2(editor->mouse_cursor_image.texture->bitmap.width, editor->mouse_cursor_image.texture->bitmap.height),
                                                                          0);
                        if(ui_pressed(selection))
                        {
                            editor->show_player_viewport = !editor->show_player_viewport;
                        }
                    }
                }
            }
        }
    }
}

#if 0
internal_api editor_action_buffer_t*
s_editor_get_last_action_buffer(editor_action_buffer_t *first_buffer)
{
    editor_action_buffer_t *result = null;
    for(editor_action_buffer_t *current_buffer = first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        if(current_buffer->next_buffer == null)
        {
            result = current_buffer;
            break;
        }
    }

    return(result);
}

internal_api void
s_editor_append_action(map_editor_t *editor, editor_action_buffer_t *buffer, editor_action_t *action)
{
    editor_action_buffer_t *chosen_buffer = null;
    editor_action_buffer_t *last_buffer   = null;
    
    editor_action_buffer_t *first_buffer = buffer;
    for(editor_action_buffer_t *current_buffer = first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        last_buffer = current_buffer;
        if(current_buffer->current_action_count + 1 < current_buffer->actions.count)
        {
            chosen_buffer = current_buffer;
            break;
        }
    }

    if(!chosen_buffer)
    {
        chosen_buffer = c_arena_push_struct(&editor->editor_arena, editor_action_buffer_t);
        last_buffer->next_buffer = chosen_buffer;
    }

    // TODO(Sleepster): Search ALL the buffers to this point?
    s32 index = c_array_add_if_unique(chosen_buffer->actions, action, chosen_buffer->current_action_count);
    if(index == -1)
    {
        ++chosen_buffer->current_action_count;
    }
}
#endif

internal_api void
editor_append_event(editor_event_buffer_t *first_buffer, editor_event_t *event)
{
    editor_event_buffer_t *active_buffer = null;
    editor_event_buffer_t *last_buffer = null;

    for(editor_event_buffer_t *current_buffer = first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        last_buffer = current_buffer;
        if(current_buffer->current_event_count + 1 < current_buffer->event_list.count)
        {
            active_buffer = current_buffer;
            break;
        }
    }

    if(!active_buffer)
    {
        active_buffer = c_arena_push_struct(first_buffer->editor_arena, editor_event_buffer_t);
        last_buffer->next_buffer = active_buffer;
    }

    editor_event_t *new_event = active_buffer->event_list + active_buffer->current_event_count++;
    *new_event = *event;
}

internal_api editor_event_t*
editor_read_next_event(editor_event_buffer_t *first_buffer)
{
    editor_event_t *result = null;

    editor_event_buffer_t  *active_buffer = null;
    for(editor_event_buffer_t *current_buffer = first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        if(current_buffer->current_event_count > 0)
        {
            active_buffer = current_buffer;
        }
    }

    if(active_buffer)
    {
        result = active_buffer->event_list + active_buffer->current_event_count - 1;
        --active_buffer->current_event_count;
    }

    return(result);
}

internal_api void
process_next_editor_event(map_editor_t *editor, editor_event_buffer_t *source_event_buffer, editor_event_buffer_t *destination_event_buffer)
{
    editor_event_t *current_event = editor_read_next_event(source_event_buffer);
    if(current_event)
    {
        switch(current_event->type)
        {   
            default: { InvalidCodePath; }break;
            case EDITOR_EVENT_TYPE_CREATE_ENTITY:
            {
                 entity_create_info_t entity_create_info = {};
                 entity_create_info.game_state    = editor->game_state;
                 entity_create_info.asset_manager = editor->asset_manager;

                 // TODO(Sleepster): Arenas here are honestly kinda dumb. We just need a history buffer 
                 entity_t **entity_array = c_arena_push_array(&editor->redo_arena, entity_t*, current_event->entities_to_destroy.count);
                 for(s32 entity_index = 0;
                     entity_index < current_event->entities_to_create.count;
                     ++entity_index)
                 {
                     entity_t *entity_info = current_event->entities_to_create + entity_index;

                     entity_create_info.position         = entity_info->position;
                     entity_create_info.initial_position = entity_info->editor_position;
                     entity_create_info.size             = entity_info->size;
                     entity_create_info.flags            = entity_info->flags;
                     entity_create_info.extra_flags      = entity_info->flags;

                     entity_t *new_entity = s_entity_create_of_type(entity_info->archetype, &entity_create_info); 
                     entity_array[entity_index] = new_entity;
                 }

                 editor_event_t redo_event = {};
                 redo_event.type                = EDITOR_EVENT_TYPE_DESTROY_ENTITY;
                 redo_event.entities_to_destroy = {entity_array, current_event->entities_to_create.count};

                 editor_append_event(destination_event_buffer, &redo_event);
            }break;
            case EDITOR_EVENT_TYPE_DESTROY_ENTITY:
            {
                entity_t *entity_array = c_arena_push_array(&editor->redo_arena, entity_t, current_event->entities_to_destroy.count);
                for(s32 entity_index = 0;
                    entity_index < current_event->entities_to_destroy.count;
                    ++entity_index)
                {
                    entity_t *entity_info = current_event->entities_to_destroy[entity_index];
                    Assert(entity_info)

                    if((entity_info->flags & ENTITY_FLAG_IS_VALID) != 0)
                    {
                        entity_array[entity_index] = *entity_info;
                        s_entity_destroy(editor->game_state->entity_manager, entity_info);
                    }
                }

                 editor_event_t redo_event = {};
                 redo_event.type = EDITOR_EVENT_TYPE_CREATE_ENTITY;
                 redo_event.entities_to_create = {entity_array, current_event->entities_to_destroy.count};

                 editor_append_event(destination_event_buffer, &redo_event);
            }break;
        }
    }
    else
    {
        c_arena_reset(&editor->undo_arena);
    }
}

/* 
=========================================================
EDITOR CONTROLS:
 TAB = "Switch Mode" (X)
 
 BOTH MODES:
     ESCAPE       = "Open Editor UI Panel" (X)
     CTRL + E     = "Open Entity Panel"
     CTRL + Z     = "Undo" (X)
     CTRL + Y     = "Redo" (X)
     MIDDLE MOUSE = "Camera Pan" (X)
     WASD         = "Camera Pan" (X)
     MOUSE SCROLL = "Camera Zoom" (X)
 
 MANIPULATE MODE:
     LEFT MOUSE (Clicked) = "Select Hovered Entity" (X)
     LEFT MOUSE (Held)    = "Box Select" (X)
     CTRL + LEFT MOUSE    = "Deselect Selected Entity"
     ALT  + LEFT MOUSE    = "Drag Group"
     CTRL + C             = "Copy Item"
     CTRL + V             = "Paste Item"
 
 PAINT MODE:
     LEFT MOUSE          = "Paint Tiles" (X)
     SHIFT + LEFT MOUSE  = "Paint Tiles Within Area" (X)
     RIGHT MOUSE         = "Remove Painted Tile" (X)
     SHIFT + RIGHT MOUSE = "Remove Tiles Within Area" (X)
     O                   = "Box Outline mode"
     L                   = "Line mode"
=========================================================
*/

internal_api void
s_editor_update_state(game_state_t *game_state, map_editor_t *editor, render_state_t *render_state, input_manager_t *input_manager)
{
    input_state_t *undo    = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_Z);
    input_state_t *redo    = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_Y);
    input_state_t *lctrl   = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LCTRL);
    input_state_t *lshift  = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LSHIFT);
    input_state_t *lalt    = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LALT);
    input_state_t *lclick  = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LEFT_MOUSE);
    input_state_t *rclick  = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_RIGHT_MOUSE);
    input_state_t *escape  = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_ESCAPE);
    input_state_t *tab_key = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_TAB);

    (void)lalt;
     
    // NOTE(Sleepster): Toggle between Paint and Manipulate modes
    if(InputStatePressed(tab_key->flags))
    {
        editor->current_editor_mode = !editor->current_editor_mode;
        s_im_input_state_consume_flags(editor->controller, 
                                       SDL_SCANCODE_TAB, 
                                       INPUT_MANAGER_INPUT_STATE_FLAG_PRESSED|INPUT_MANAGER_INPUT_STATE_FLAG_DOWN);
    }

    // NOTE(Sleepster): Toggle UI Panel
    if(InputStatePressed(escape->flags))
    {
        editor->show_panel = !editor->show_panel;
    }

    // NOTE(Sleepster): Undo 
    if(InputStatePressed(undo->flags) && InputStateDown(lctrl->flags))
    {
        process_next_editor_event(editor, &editor->first_undo_event_buffer, &editor->first_redo_event_buffer);
    }

    // NOTE(Sleepster): Redo 
    if(InputStatePressed(redo->flags) && InputStateDown(lctrl->flags))
    {
        process_next_editor_event(editor, &editor->first_redo_event_buffer, &editor->first_undo_event_buffer);
    }

    editor->camera.zoom     = editor->zoom;
    editor->camera.viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                                   Max(render_state->RHI_context->window_size.y, 10));
    RHI_render_camera_set_matrices(&editor->camera);

    s_im_controller_update_state(input_manager, editor->controller, false);
    vec2_t mouse_position = s_im_transform_mouse_data(editor->controller, 
                                                      editor->camera.viewport, 
                                                      editor->camera.matrices.view_matrix, 
                                                      editor->camera.matrices.projection_matrix);

    input_state_t *middle_mouse   = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_MIDDLE_MOUSE);
    input_state_t *mouse_movement = s_im_controller_get_input_state(editor->controller, INPUT_ARRAY_INPUT_AXIS_MOUSE_MOVEMENT);
    input_state_t *mouse_scroll   = s_im_controller_get_input_state(editor->controller, INPUT_ARRAY_INPUT_AXIS_MOUSE_WHEEL);

    s_im_input_action_update_state(input_manager, editor->controller);

    // NOTE(Sleepster): Keyboard camera movement 
    float32 navigation_speed_modifier = expf(logf(2400.0f * gc->frame_time) + (editor->camera.zoom * 0.1));
    {

        vec2_t camera_movement = vec2_scale(game_state->mappings.move->axis2D_value, navigation_speed_modifier);
        editor->camera.translation.x += camera_movement.x;
        editor->camera.translation.y -= camera_movement.y;
    }

    vec2_t scroll_delta     = mouse_scroll->delta_value; 
    vec2_t mouse_move_delta = mouse_movement->delta_value;

    // NOTE(Sleepster): Mouse Panning 
    if(InputStateDown(middle_mouse->flags))
    {
        editor->camera.translation.x += mouse_move_delta.x * navigation_speed_modifier;
        editor->camera.translation.y -= mouse_move_delta.y * navigation_speed_modifier;
    }

    // NOTE(Sleepster): Zoom adjustment 
    if(scroll_delta.y != 0.0f)
    {
        float32 previous_zoom = editor->zoom;

        // NOTE(Sleepster): Taken from this raylib example:
        // https://github.com/raysan5/raylib/blob/master/examples/core/core_2d_camera_mouse_zoom.c
        float scale = 0.2f * scroll_delta.y;
        editor->zoom = Clamp(expf(logf(previous_zoom) + scale), 1.0f, 10.0f);

        vec2_t new_mouse_offset_scaling = vec2_scale(mouse_position, editor->zoom - previous_zoom);
        editor->camera.translation = vec2_add(editor->camera.translation, new_mouse_offset_scaling);
    }

    // NOTE(Sleepster): Box Tracking
    if(!editor->boxing && (InputStateDown(lclick->flags) || InputStateDown(rclick->flags)))
    {
        editor->boxing = true;
        editor->boxing_start = mouse_position;
    }

    if(editor->boxing)
    {
        editor->boxing_end = mouse_position; 
        if(!InputStateDown(lclick->flags) && !InputStateDown(rclick->flags))
        {
            editor->boxing = false;
        }
    }

    s_im_input_action_update_state(input_manager, editor->controller);

    // NOTE(Sleepster): Manipulate Mode 
    if(!editor->current_editor_mode)
    {
        // NOTE(Sleepster): Entity selection
        if(InputStatePressed(lclick->flags))
        {
            world_chunk_t *chunk = s_entity_manager_get_or_create_chunk(game_state->entity_manager, mouse_position);
            if(chunk)
            {
                entity_t *selected_entity = null;
                for(s32 entity_index = 0;
                    entity_index < chunk->chunk_entity_count;
                    ++entity_index)
                {
                    entity_t *entity = s_entity_access_sparse_chunk_entity(chunk, entity_index);
                    rectangle2_t editor_collider_rect = rect2_create(entity->editor_position, entity->size);
                    if(rect2_point_in_rect(editor_collider_rect, mouse_position))
                    {
                        selected_entity = entity;
                        break;
                    }
                }

                if(selected_entity)
                {
                    if(editor->selected_entity_count + 1 < editor->selected_entities.count)
                    {
                        s32 result = c_array_add_if_unique(editor->selected_entities, &selected_entity, editor->selected_entity_count);
                        if(result == -1)
                        {
                            ++editor->selected_entity_count;
                        }
                    }
                }
                else if(!selected_entity && !InputStateDown(lshift->flags))
                {
                    editor->selected_entity_count = 0;
                    c_array_clear(editor->selected_entities);
                }
            }
        }

        // NOTE(Sleepster): Box selecting 
        if(editor->boxing)
        {
            vec2_t min = vec2(Min(editor->boxing_start.x, editor->boxing_end.x), Min(editor->boxing_start.y, editor->boxing_end.y)); 
            vec2_t max = vec2(Max(editor->boxing_start.x, editor->boxing_end.x), Max(editor->boxing_start.y, editor->boxing_end.y));

            entity_query_t spatial_query = s_entity_get_entities_within_position_range(game_state->entity_manager, min, max);
            if(spatial_query.entity_count > 0)
            {
                for(entity_t *entity: spatial_query)
                {
                    if(editor->selected_entity_count + 1 > editor->selected_entities.count)
                    {
                        break;
                    }

                    s32 result = c_array_add_if_unique(editor->selected_entities, &entity, editor->selected_entity_count);
                    if(result == -1)
                    {
                        ++editor->selected_entity_count;
                    }
                }
            }
        }
    }
    // NOTE(Sleepster): Paint mode 
    else
    {
        editor->selected_entity_count = 0;

        vec2_t rounded_start = vec2((roundf(editor->boxing_start.x) / 8.0) * 8.0, (roundf(editor->boxing_start.y) / 8.0) * 8.0);
        vec2_t rounded_end   = vec2((roundf(editor->boxing_end.x)   / 8.0) * 8.0, (roundf(editor->boxing_end.y)   / 8.0) * 8.0);

        vec2_t min = vec2(Min(rounded_start.x, rounded_end.x), Min(rounded_start.y, rounded_end.y)); 
        vec2_t max = vec2(Max(rounded_start.x, rounded_end.x), Max(rounded_start.y, rounded_end.y));

        vec2_t min_tile_position = vec2_scale(world_to_tile(min), WORLD_TILE_SIZE);
        vec2_t max_tile_position = vec2_scale(world_to_tile(max), WORLD_TILE_SIZE);
        if(InputStateDown(lclick->flags))
        {
            if(!InputStateDown(lshift->flags))
            {
                // NOTE(Sleepster): Normal Tile placing 
                world_chunk_t *chunk = s_entity_manager_get_or_create_chunk(game_state->entity_manager, mouse_position);
                if(chunk)
                {
                    bool8 cell_occupied = false;
                    for(s32 entity_index = 0;
                        entity_index < chunk->chunk_entity_count;
                        ++entity_index)
                    {
                        entity_t *entity = s_entity_access_sparse_chunk_entity(chunk, entity_index);
                        if(rect2_point_in_rect(entity->bounding_box, mouse_position))
                        {
                            cell_occupied = true;
                            break;
                        }
                    }

                    if(!cell_occupied)
                    {
                        vec2_t tile_position = vec2_scale(world_to_tile(mouse_position), WORLD_TILE_SIZE);
                        entity_t *tile = s_entity_tile_create(game_state, tile_position, 0);

                        entity_t **entities = c_arena_push_array(&editor->undo_arena, entity_t*, 1);
                        entities[0] = tile;

                        editor_event_t new_event = {};
                        new_event.type                = EDITOR_EVENT_TYPE_DESTROY_ENTITY;
                        new_event.entities_to_destroy = {entities, 1};

                        editor_append_event(&editor->first_undo_event_buffer, &new_event);
                    }
                }
            }
            else
            {
                editor->box_placing = true;
            }
        }
        else
        {
            // NOTE(Sleepster): Box placement of tiles 
            if(editor->box_placing)
            {
                editor->box_placing = false;

                scratch_arena_t temp_arena = c_arena_begin_temporary_memory(&gc->temp_arena);
                defer(c_arena_end_temporary_memory(&temp_arena));

                u32 entities_created = 0;
                entity_t **temp_entity_array = c_arena_push_array(temp_arena.parent, entity_t*, MAX_CHUNK_ENTITIES);

                entity_query_t spatial_query = s_entity_get_entities_within_position_range(game_state->entity_manager, min, max);
                for(s32 tile_x = min_tile_position.x;
                    tile_x != max_tile_position.x;
                    tile_x += WORLD_TILE_SIZE)
                {
                    for(s32 tile_y = min_tile_position.y;
                        tile_y != max_tile_position.y;
                        tile_y += WORLD_TILE_SIZE)
                    {
                        bool8 collision = false;
                        for(entity_t *entity: spatial_query)
                        {
                            rectangle2_t new_bounding_box = rect2_create(entity->editor_position, vec2_scale(entity->size, 0.5f));
                            if(rect2_point_in_rect(new_bounding_box, vec2(tile_x, tile_y)))
                            {
                                collision = true;
                                break;
                            }
                        }

                        if(!collision)
                        {
                            entity_t *tile = s_entity_tile_create(game_state, vec2(tile_x, tile_y), 0);
                            temp_entity_array[entities_created++] = tile;
                        }
                    }
                }

                entity_t **real_entity_array = c_arena_push_array(&editor->undo_arena, entity_t*, entities_created);
                memcpy(real_entity_array, temp_entity_array, sizeof(entity_t*) * entities_created);

                editor_event_t new_event = {};
                new_event.type                = EDITOR_EVENT_TYPE_DESTROY_ENTITY;
                new_event.entities_to_destroy = {real_entity_array, (s32)entities_created};

                editor_append_event(&editor->first_undo_event_buffer, &new_event);
            }
        }

        // NOTE(Sleepster): Destroy entities 
        if(InputStateDown(rclick->flags))
        {
            // NOTE(Sleepster): Normal Destroy 
            if(!InputStateDown(lshift->flags))
            {
                world_chunk_t *chunk = s_entity_manager_get_or_create_chunk(game_state->entity_manager, mouse_position);
                if(chunk)
                {
                    entity_t *entity_destroyed = null;
                    for(s32 entity_index = 0;
                        entity_index < chunk->chunk_entity_count;
                        ++entity_index)
                    {
                        entity_t *entity = s_entity_access_sparse_chunk_entity(chunk, entity_index);
                        if(rect2_point_in_rect(entity->bounding_box, mouse_position))
                        {
                            entity_destroyed = entity;
                            break;
                        }
                    }

                    if(entity_destroyed)
                    {
                        // TODO(Sleepster): Allocating this as part of the editor arena may be bad
                        // since this means that each undo will just append a bunch of data on to the
                        // arena, eventually overflowing it with no way of freeing said data. A free list will
                        // be needed here.
                        array_t<entity_t> entities;
                        entity_t *entity_data = c_arena_push_array(&editor->undo_arena, entity_t, 1);

                        entities.items = entity_data;
                        entities.count = 1;

                        *entity_data = *entity_destroyed;

                        editor_event_t new_event = {};
                        new_event.type               = EDITOR_EVENT_TYPE_CREATE_ENTITY;
                        new_event.entities_to_create = entities;

                        editor_append_event(&editor->first_undo_event_buffer, &new_event);
                        s_entity_destroy(game_state->entity_manager, entity_destroyed);
                    }
                }
            }
            else
            {
                editor->box_removing = true;
            }
        }
        else
        {
            // NOTE(Sleepster): Box Destroy 
            if(editor->box_removing)
            {
                editor->box_removing = false;

                entity_query_t spatial_query = s_entity_get_entities_within_position_range(game_state->entity_manager, min, max);
                if(spatial_query.entity_count > 0)
                {
                    u32 entity_count = 0;
                    entity_t *entity_data = c_arena_push_array(&editor->undo_arena, entity_t, spatial_query.entity_count);
                    for(entity_t *entity: spatial_query)
                    {
                        entity_data[entity_count++] = *entity;
                        s_entity_destroy(game_state->entity_manager, entity);
                    }

                    editor_event_t new_event = {};
                    new_event.type               = EDITOR_EVENT_TYPE_CREATE_ENTITY;
                    new_event.entities_to_create = {entity_data, (s32)entity_count};

                    editor_append_event(&editor->first_undo_event_buffer, &new_event);
                }
            }
        }
    }
}

internal_api void
s_editor_render_to_output(game_state_t *game_state, map_editor_t *editor, asset_manager_t *asset_manager, render_state_t *render_state)
{
    asset_handle_t immediate_rectangle = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_rectangle"));

    s32 window_width  = Max(render_state->RHI_context->window_size.x, 10);
    s32 window_height = Max(render_state->RHI_context->window_size.y, 10);
    s_render_graph_command_clear_renderpass(&render_state->render_graph, render_state->fullscreen_renderpass_ID);

    // NOTE(Sleepster): Line render_group 
    RHI_render_camera_t *scene_camera = &editor->camera;
    {
        u32 minimum_vertex_count = 8;

        render_group_t *current_render_group = s_render_group_begin(&render_state->render_graph, render_state->fullscreen_renderpass_ID);
        current_render_group->shader         = immediate_rectangle;
        current_render_group->line_width     = 1.0f;

        RHI_pipeline_state_t current_render_state = {};
        current_render_state.blend_enabled  = true;
        current_render_state.depth_testing_enabled = true;
        current_render_state.depth_writing_enabled = true;
        current_render_state.polygon_mode   = RENDER_PIPELINE_POLYGON_MODE_LINE;
        current_render_state.primitive_type = RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_LIST;

        current_render_group->line_width = 1.0f;

        current_render_group->pipeline_state = current_render_state;
        current_render_group->scissor = {
            .offset = vec2_zero(),
            .extent = vec2(window_width, window_height)
        };
        current_render_group->viewport = {
            .offset = vec2(0,             window_height),
            .extent = vec2(window_width, -window_height)
        };

        s_render_group_add_constant_buffer(current_render_group, render_state->camera_matrices_buffer, &scene_camera->matrices, sizeof(mat4_t) * 2);
        float32 half_width  = (scene_camera->viewport.x * 0.5f) * scene_camera->zoom;
        float32 half_height = (scene_camera->viewport.y * 0.5f) * scene_camera->zoom;

        float32 left   = scene_camera->translation.x - half_width;
        float32 right  = scene_camera->translation.x + half_width;
        float32 bottom = scene_camera->translation.y - half_height;
        float32 top    = scene_camera->translation.y + half_height;

        vec2_t start = world_to_tile(vec2(left,  bottom));
        vec2_t end   = world_to_tile(vec2(right, top));

        // NOTE(Sleepster): + 4 is for the selection box. 
        s32 line_count = (((end.x - start.x + 1) + (end.y - start.y)) * 2) + minimum_vertex_count + 4;
        render_group_vertex_stream_t *vertex_stream = s_render_group_append_vertex_stream(current_render_group, 
                                                                                          sizeof(immediate_vertex_t), 
                                                                                          line_count, 
                                                                                          &render_state->vertex_buffer);
        if(editor->show_grid)
        {
            for(s32 tile_x = start.x;
                tile_x <= end.x;
                ++tile_x)
            {
                float32 current_tile_x = tile_x * WORLD_TILE_SIZE;
                immediate_line(vertex_stream, 
                               vec2(current_tile_x, top),
                               vec2(current_tile_x, bottom),
                               0.90,
                               vec4(0.01, 0.01, 0.1, 1.0f));
            }

            for(s32 tile_y = start.y;
                tile_y < end.y;
                ++tile_y)
            {
                float32 current_tile_y = tile_y * WORLD_TILE_SIZE;
                immediate_line(vertex_stream, 
                               vec2(left, current_tile_y),
                               vec2(right, current_tile_y),
                               0.90,
                               vec4(0.01, 0.01, 0.1, 1.0f));
            }
        }

        if(editor->show_player_viewport)
        {
            float32 half_width  = (game_state->game_camera.viewport.x * 0.5f) * game_state->game_camera.zoom;
            float32 half_height = (game_state->game_camera.viewport.y * 0.5f) * game_state->game_camera.zoom;

            float32 left   = game_state->game_camera.translation.x - half_width;
            float32 right  = game_state->game_camera.translation.x + half_width;
            float32 bottom = game_state->game_camera.translation.y - half_height;
            float32 top    = game_state->game_camera.translation.y + half_height;

            immediate_line(vertex_stream,
                           vec2(left,  bottom),
                           vec2(left, top),
                           0.0,
                           vec4(1.0f, 1.0, 1.0f, 1.0f));

            immediate_line(vertex_stream,
                           vec2(left,  top),
                           vec2(right, top),
                           0.0,
                           vec4(1.0f, 1.0, 1.0f, 1.0f));

            immediate_line(vertex_stream,
                           vec2(right, top),
                           vec2(right, bottom),
                           0.0,
                           vec4(1.0f, 1.0, 1.0f, 1.0f));

            immediate_line(vertex_stream,
                           vec2(right, bottom),
                           vec2(left,  bottom),
                           0.0,
                           vec4(1.0f, 1.0, 1.0f, 1.0f));
        }
        s_render_group_end(&render_state->render_graph, current_render_group);
    }

    // NOTE(Sleepster): Quad render group 
    {
        render_group_t *current_render_group = s_render_group_begin(&render_state->render_graph, render_state->fullscreen_renderpass_ID);
        current_render_group->shader         = immediate_rectangle;
        current_render_group->line_width     = 1.0f;

        RHI_pipeline_state_t current_render_state = {};
        current_render_state.blend_enabled  = true;
        current_render_state.depth_testing_enabled = true;
        current_render_state.depth_writing_enabled = true;
        current_render_state.primitive_type = RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        current_render_group->pipeline_state = current_render_state;
        current_render_group->index_buffer   = &render_state->index_buffer;

        current_render_group->line_width = 1.0f;
        current_render_group->scissor = {
            .offset = vec2_zero(),
            .extent = vec2(window_width, window_height)
        };
        current_render_group->viewport = {
            .offset = vec2(0,             window_height),
            .extent = vec2(window_width, -window_height)
        };

        render_group_vertex_stream_t *vertex_stream = s_render_group_append_vertex_stream(current_render_group, 
                                                                                          sizeof(immediate_vertex_t), 
                                                                                          4, 
                                                                                         &render_state->vertex_buffer);
        // NOTE(Sleepster): Visual selection box 
        if(editor->boxing)
        {
            input_state_t *lshift = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LSHIFT);
            float32 size_x = (editor->boxing_end.x - editor->boxing_start.x);
            float32 size_y = (editor->boxing_end.y - editor->boxing_start.y);

            vec4_t box_color = vec4(0.0, 0.6, 0.0, 0.1);
            if(editor->current_editor_mode)
            {
                box_color = editor->box_removing 
                            ? vec4(0.2, 0.0, 0.0, 0.2)
                            : vec4(0.1, 0.1, 0.1, 0.1);
                if(InputStateDown(lshift->flags))
                {
                    immediate_rect(vertex_stream,
                                   vec3(editor->boxing_start.x, editor->boxing_start.y, 0.8),
                                   vec2(size_x, size_y),
                                   box_color);
                }
            }
            else
            {
                immediate_rect(vertex_stream,
                               vec3(editor->boxing_start.x, editor->boxing_start.y, 0.8),
                               vec2(size_x, size_y),
                               box_color);
            }
        }

        s_render_group_end(&render_state->render_graph, current_render_group);
    }
}
