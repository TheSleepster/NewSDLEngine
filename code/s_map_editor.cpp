/* ========================================================================
   $File: s_map_editor.cpp $
   $Date: September 25 2026 08:13 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
constexpr u32 MAX_EDITOR_UNDO_ACTION_COUNT = 1024;
constexpr u32 MAX_SELECTED_EDITOR_ENTITIES = 128;

enum editor_action_kind_t
{
    EDITOR_ACTION_ENTITY_CREATE,
};

enum editor_action_type_t
{
    EDITOR_ACTION_TYPE_UNDO,
    EDITOR_ACTION_TYPE_REDO,
};

struct editor_action_t
{
    u32       type;
    u32       kind;
    entity_t *entity;

    u32       entity_archetype;
    u32       entity_flags;
    vec2_t    entity_position;
};

using editor_undo_action_array_t     = fixed_array_t<editor_action_t, MAX_EDITOR_UNDO_ACTION_COUNT>;
using editor_selected_entity_array_t = fixed_array_t<entity_t*,       MAX_SELECTED_EDITOR_ENTITIES>;

struct editor_action_buffer_t
{
    editor_undo_action_array_t actions;
    s32                        current_action_count;

    editor_action_buffer_t    *next_buffer;
};

struct map_editor_t
{
    memory_arena_t                 editor_arena;

    bool32                         show_grid;
    bool32                         show_player_viewport;
    float32                        zoom;
    RHI_render_camera_t            camera;
    input_controller_t            *controller;

    asset_handle_t                 gizmo_image;
    asset_handle_t                 mouse_cursor_image;

    editor_action_buffer_t         first_undo_buffer;
    editor_action_buffer_t         first_redo_buffer;

    editor_selected_entity_array_t selected_entities;
    s32                            selected_entity_count;
};

internal_api map_editor_t*
s_map_editor_create(input_manager_t *input_manager, asset_manager_t *asset_manager, render_state_t *render_state)
{
    map_editor_t *result     = (map_editor_t*)c_alloc(sizeof(map_editor_t), ALLOCATOR_TAG_GAME);
    result->editor_arena     = c_arena_create(MB(50), ALLOCATOR_TAG_GAME);
    result->controller       = s_im_init_input_controller(input_manager);

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
s_map_editor_handle_ui(game_state_t *game_state, ui_state_t *main_ui, RHI_context_t *RHI_context)
{
    map_editor_t *editor = game_state->editor;
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
                    game_state->editor->show_grid = !game_state->editor->show_grid;
                }

                ui_signal_t show_viewport = ui_widget_labeled_button(main_ui, STR("Toggle Editor Player Viewport"));
                if(ui_pressed(show_viewport))
                {
                    game_state->editor->show_player_viewport = !game_state->editor->show_player_viewport;
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
                                                                  &editor->gizmo_image, 
                                                                  vec2_zero(), 
                                                                  vec2(editor->gizmo_image.texture->bitmap.width, editor->gizmo_image.texture->bitmap.height),
                                                                  0);
                    if(ui_pressed(gizmo))
                    {
                        game_state->editor->show_player_viewport = !game_state->editor->show_player_viewport;
                    }

                    ui_signal_t selection = ui_widget_textured_button(main_ui, 
                                                                  STR("Editor Mouse Selection"), 
                                                                  vec2(32.0f, 26.0f), 
                                                                  vec2(5.0f, -10.0f),
                                                                  &editor->mouse_cursor_image, 
                                                                  vec2_zero(), 
                                                                  vec2(editor->mouse_cursor_image.texture->bitmap.width, editor->mouse_cursor_image.texture->bitmap.height),
                                                                  0);
                    if(ui_pressed(selection))
                    {
                        game_state->editor->show_player_viewport = !game_state->editor->show_player_viewport;
                    }
                }
            }
        }
    }
}

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
s_editor_append_action(map_editor_t *editor, editor_action_t *action)
{
    editor_action_buffer_t *chosen_buffer = null;
    editor_action_buffer_t *last_buffer   = null;
    
    editor_action_buffer_t *first_buffer = action->type == EDITOR_ACTION_TYPE_UNDO ? &editor->first_undo_buffer : &editor->first_redo_buffer;
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

    chosen_buffer->actions[chosen_buffer->current_action_count++] = *action;
}

/* 
=========================================================
EDITOR CONTROLS:
 LSHIFT + S = "Switch Mode"
 
 BOTH MODES:
     ESCAPE       = "Open Editor UI Panel"
     CTRL + E     = "Open Entity Panel"
     CTRL + Z     = "Undo"
     CTRL + Y     = "Redo"
     MIDDLE MOUSE = "Camera Pan"
     WASD         = "Camera Pan"
     MOUSE SCROLL = "Camera Zoom"
 
 MANIPULATE MODE:
     LEFT MOUSE (Clicked) = "Select Hovered Entity"
     LEFT MOUSE (Held)    = "Box Select"
     CTRL + LEFT MOUSE    = "Deselect Selected Entity"
     ALT  + LEFT MOUSE    = "Drag Group"
     CTRL + C             = "Copy Item"
     CTRL + V             = "Paste Item"
 
 PAINT MODE:
     LEFT MOUSE         = "Paint Tiles"
     CTRL + LEFT MOUSE  = "Paint Tiles Within Area"
     RIGHT MOUSE        = "Remove Painted Tile"
     CTRL + RIGHT MOUSE = "Remove Tiles Within Area"
=========================================================
*/

internal_api void
s_editor_update_state(game_state_t *game_state, render_state_t *render_state, input_manager_t *input_manager)
{
    map_editor_t *editor = game_state->editor;

    input_state_t *undo   = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_Z);
    input_state_t *redo   = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_Y);
    input_state_t *lctrl  = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LCTRL);
    input_state_t *lshift = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LSHIFT);
    input_state_t *lalt   = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LALT);
    input_state_t *lclick = s_im_controller_get_input_state(editor->controller, SDL_SCANCODE_LEFT_MOUSE);

    // NOTE(Sleepster): Undo  
    if(InputStatePressed(undo->flags) && InputStateDown(lctrl->flags))
    {
        editor_action_buffer_t *undo_buffer = s_editor_get_last_action_buffer(&editor->first_undo_buffer);
        if(undo_buffer->current_action_count != 0)
        {
            editor_action_t *undo_action = undo_buffer->actions + (undo_buffer->current_action_count - 1);
            if(undo_action->kind == EDITOR_ACTION_ENTITY_CREATE)
            {
                editor_action_t action = {
                    .type = EDITOR_ACTION_TYPE_REDO,
                    .kind = EDITOR_ACTION_ENTITY_CREATE,
                    .entity_archetype = undo_action->entity->archetype,
                    .entity_flags     = undo_action->entity->flags,
                    .entity_position  = undo_action->entity->position
                };
                s_editor_append_action(editor, &action);
                s_entity_destroy(game_state->entity_manager, undo_action->entity);
            }

            undo_buffer->current_action_count--;
            Assert(undo_buffer->current_action_count >= 0);
        }
    }

    // NOTE(Sleepster): Redo 
    if(InputStatePressed(redo->flags) && InputStateDown(lctrl->flags))
    {
        editor_action_buffer_t *redo_buffer = s_editor_get_last_action_buffer(&editor->first_redo_buffer);
        if(redo_buffer->current_action_count != 0)
        {
            editor_action_t *redo_action = redo_buffer->actions + (redo_buffer->current_action_count - 1);
            if(redo_action->kind == EDITOR_ACTION_ENTITY_CREATE)
            {
                switch(redo_action->entity_archetype)
                {
                    case ENTITY_ARCHETYPE_TILE:
                    {
                        entity_tile_create(game_state, redo_action->entity_position, redo_action->entity_flags);
                    }break;
                }
            }

            redo_buffer->current_action_count--;
            Assert(redo_buffer->current_action_count >= 0);

            editor_action_buffer_t *undo_buffer = s_editor_get_last_action_buffer(&editor->first_undo_buffer);
            ++undo_buffer->current_action_count;
        }
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
    {
        vec2_t camera_movement = game_state->mappings.move->axis2D_value;
        editor->camera.translation.x += camera_movement.x;
        editor->camera.translation.y -= camera_movement.y;
    }

    vec2_t scroll_delta     = mouse_scroll->delta_value; 
    vec2_t mouse_move_delta = mouse_movement->delta_value;

    // NOTE(Sleepster): Mouse Panning 
    if(InputStateDown(middle_mouse->flags))
    {
        editor->camera.translation.x += mouse_move_delta.x;
        editor->camera.translation.y -= mouse_move_delta.y;
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


    // NOTE(Sleepster): Entity selection
    if(InputStatePressed(lclick->flags) && InputStateDown(lalt->flags))
    {
        world_chunk_t *chunk = s_entity_manager_get_or_create_chunk(game_state->entity_manager, mouse_position);
        if(chunk)
        {
            entity_t *selected_entity = null;
            for(u32 entity_index = 0;
                entity_index < chunk->chunk_entity_count;
                ++entity_index)
            {
                entity_t *entity = chunk->entities + entity_index;
                rectangle2_t editor_collider_rect = rect2_create(entity->editor_position, entity->size);
                if(rect2_point_in_rect(editor_collider_rect, mouse_position))
                {
                    selected_entity = entity;
                    break;
                }
                else
                {
                    s32 index = c_array_find(editor->selected_entities, &entity);
                    if(index != -1)
                    {
                        c_array_remove(editor->selected_entities, index, editor->selected_entity_count);
                        --editor->selected_entity_count;
                    }
                }
            }

            if(selected_entity)
            {
                s32 result = c_array_add_if_unique(editor->selected_entities, &selected_entity, editor->selected_entity_count);
                if(result == -1)
                {
                    ++editor->selected_entity_count;
                }
            }
        }
    }
    else if(InputStateDown(lclick->flags) && !InputStateDown(lalt->flags))
    {
        // NOTE(Sleepster): Tile placing 
        world_chunk_t *chunk = s_entity_manager_get_or_create_chunk(game_state->entity_manager, mouse_position);
        if(chunk)
        {
            bool8 cell_occupied = false;
            for(const entity_t &entity: chunk->entities)
            {
                if(rect2_point_in_rect(entity.bounding_box, mouse_position))
                {
                    cell_occupied = true;
                    break;
                }
            }

            if(!cell_occupied)
            {
                vec2_t tile_position = vec2_scale(world_to_tile(mouse_position), WORLD_TILE_SIZE);
                entity_t *tile = entity_tile_create(game_state, tile_position, 0);
                editor_action_t action = {
                    .type   = EDITOR_ACTION_TYPE_UNDO,
                    .kind   = EDITOR_ACTION_ENTITY_CREATE,
                    .entity = tile
                };

                s_editor_append_action(editor, &action);
            }
        }
    }
}
