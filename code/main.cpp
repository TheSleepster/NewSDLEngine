/* ========================================================================
   $File: main.cpp $
   $Date: March 27 2026 06:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
// NOTE(Sleepster): For srand() and rand()
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_global_context.h>
#include <c_zone_allocator.h>
#include <c_program_flag_handler.h>
#include <c_tokenizer.h>
#include <p_platform_data.h>

#include <s_RHI_image.h>
#include <s_RHI_core.h>
#include <c_duration_counter.h>
#include <r_immediate_rendering.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>
#include <s_ui_core.h>
#include <s_entity.h>

void process_window_events(RHI_context_t *RHI_context, input_manager_t *input_manager);

/*===========================================
  =============== ANIMATION2D ===============
  =========================================== */

// TODO(Sleepster): This will eventually belong to the asset manager 
struct animation_source2D_t 
{
    // NOTE(Sleepster): Debug information 
    string_t           name;

    // NOTE(Sleepster): Actual animation information 
    asset_handle_t     texture;
    u32                frame_count;
    ivec2_t            frame_size;
    ivec2_t            frame_offset;
    duration_counter_t frame_timer;
};

// NOTE(Sleepster): This is like the material instance vs material archetype 
struct animation2D_t
{
    // NOTE(Sleepster): This is read only 
    animation_source2D_t animation_info;

    // NOTE(Sleepster): Modifiable data 
    u32                   current_frame;
    duration_counter_t    frame_timer;
};

struct render_state_t
{
    RHI_image_t         game_color_buffer;
    RHI_image_t         game_depth_buffer;

    RHI_image_t         fullscreen_color_buffer;
    RHI_image_t         fullscreen_depth_buffer;

    RHI_vertex_buffer_t vertex_buffer;
    RHI_index_buffer_t  index_buffer;

    RHI_uniform_constant_buffer_t *camera_matrices_buffer;

    u32                 game_renderpass_ID;
    u32                 fullscreen_renderpass_ID; 

    RHI_context_t      *RHI_context;
};

enum game_mode_t
{
    GAME_MODE_NORMAL,
    GAME_MODE_EDIT_MODE
};

struct game_state_t
{
    bool8               is_initialized;

    render_state_t      render_state;
    ui_state_t         *main_ui;

    input_controller_t *controller;
    entity_manager_t   *entity_manager;
    float64             render_alpha;
    bool32              open_debug_menu;

    bool32              editor_opened;
    bool32              show_grid;
    bool32              show_player_viewport;
    float32             editor_zoom;
    RHI_render_camera_t editor_camera;

    RHI_render_camera_t game_camera;
    RHI_render_camera_t fullscreen_camera;

    float32             gravity;
    s32                 game_mode;

    struct {
        game_action_t *move;
        game_action_t *jump;
        game_action_t *dash;
    }mappings;

    struct {
        vec2_t movement_axis;
        bool8  jumped;
        bool8  dashed;
    }input_info;
};

enum player_animation_state_t
{
    PLAYER_ANIMATION_STATE_INVALID,
    PLAYER_ANIMATION_STATE_IDLE,
    PLAYER_ANIMATION_STATE_RUNNING,
    PLAYER_ANIMATION_STATE_JUMPING,
    PLAYER_ANIMATION_STATE_FALLING,
    PLAYER_ANIMATION_STATE_COUNT
};

constexpr u32 GAME_FRAMEBUFFER_WIDTH  = 320;
constexpr u32 GAME_FRAMEBUFFER_HEIGHT = 180;

constexpr float32 COLLISION_EPSILON       = 0.01f;
constexpr float32 MAX_ENTITY_ACCELERATION = 100;

constexpr s32     WORLD_TILE_SIZE = 8;

// TODO(Sleepster): DEBUG CODE 
global_variable string_t global_test_textbox_string = {}; 
// TODO(Sleepster): DEBUG CODE 

// NOTE(Sleepster): DEBUG CODE
internal_api void
handle_debug_ui_menu(ui_state_t *main_ui, RHI_context_t *RHI_context, asset_handle_t *player_sprite)
{
    // NOTE(Sleepster): DEBUG UI 
    ui_signal_t main_panel = ui_widget_draggable_panel(main_ui, 
                                                       STR("Test panel..."), 
                                                       vec2(20, 20), 
                                                       vec2(20, 20), 
                                                       vec2(10.0f, 10.0f), 
                                                       vec4(10, 10, 10, 10), 
                                                       vec4(0.4, 0.4, 0.4, 0.5),
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
        ui_widget_set_default_font_size(main_ui, 40);
        ui_signal_t title_bar = ui_widget_panel(main_ui, 
                                                STR("Title bar"), 
                                                vec2(0, 0), 
                                                vec2(20, 20),
                                                vec2(10.0f, 0.0f), 
                                                vec4(10, 10, 10, 10), 
                                                vec4(0.0, 0.0, 0.0, 0.2),
                                                0);
        ui_state_set_active_padding(main_ui, vec4(10, 10, 4, 4));
        ui_signal_t open_menu_button = {};
        ui_row(main_ui, title_bar.widget)
        {
            open_menu_button = ui_widget_sized_button(main_ui, 
                                                      STR("Test button..."), 
                                                      vec2(20, 20), 0);
            ui_widget_text(main_ui, STR("[Debug Menu Information]"));
        }

        if(ui_pressed(open_menu_button))
        {
            main_panel.widget->state->toggled = !main_panel.widget->state->toggled;
        }

        if(main_panel.widget->toggled)
        {
            ui_widget_divider(main_ui, 
                              STR("main menu divider"), 
                              vec2(0.9, 5.0f), 
                              {UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT, UI_WIDGET_SIZE_KIND_PIXELS});

            ui_widget_set_default_font_size(main_ui, 20);
            ui_signal_t debug_menu = ui_widget_labeled_button(main_ui, STR("Enable Debug Overlay"));
            if(ui_pressed(debug_menu))
            {
                debug_menu.widget->state->toggled = !debug_menu.widget->state->toggled;
            }

            if(debug_menu.widget->state->toggled == true)
            {
                ui_signal_t sub_panel = ui_widget_panel(main_ui, 
                                                        STR("Debug Menu subpanel"), 
                                                        vec2(0, 0), 
                                                        vec2(20, 20),
                                                        vec2(10.0f, 10.0f), 
                                                        vec4(10, 10, 0, 0), 
                                                        vec4_zero(),
                                                        0);
                ui_column(main_ui, sub_panel.widget)
                {
                    ui_widget_divider(main_ui, 
                                      STR("Debug submenu bar 2"), 
                                      vec2(1.0, 5.0f), 
                                      {UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT, UI_WIDGET_SIZE_KIND_PIXELS});
                    ui_signal_t perf_counters = ui_widget_labeled_button(main_ui, STR("Display Performance Counters"));
                    if(ui_pressed(perf_counters))
                    {
                        perf_counters.widget->state->toggled = !perf_counters.widget->state->toggled;
                    }

                    if(perf_counters.widget->state->toggled == true)
                    {                    
                        ui_signal_t another_sub_panel = ui_widget_panel(main_ui, 
                                                                        STR("Debug PERF Menu subpanel stuff"), 
                                                                        vec2(0, 0), 
                                                                        vec2(20, 20),
                                                                        vec2(10.0f, 10.0f), 
                                                                        vec4(20, 20, 10, 10), 
                                                                        vec4_zero(),
                                                                        0);
                        ui_column(main_ui, another_sub_panel.widget)
                        {
                            ui_widget_labeled_button(main_ui, STR("Show Performance Chart"));
                            ui_widget_labeled_button(main_ui, STR("Show Running Chart"));
                            ui_widget_labeled_button(main_ui, STR("Show Entity Culling Chart"));
                        }
                    }

                    ui_widget_labeled_button(main_ui, STR("Show Timing Flame Graph"));
                    ui_widget_labeled_button(main_ui, STR("Show RAM Stats"));

                    ui_widget_float_slider_bar(main_ui, STR("Test slider..."), 100, 8, 2.5f);
                    ui_widget_divider(main_ui, 
                                      STR("Debug submenu divider"), 
                                      vec2(1.0, 5.0f), 
                                      {UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT, UI_WIDGET_SIZE_KIND_PIXELS});
                }
            }

            ui_widget_labeled_button(main_ui, STR("Enable Editor"));

            ui_widget_textbox(main_ui, STR("Information Box"), &global_test_textbox_string, {400, 20});
            ui_signal_t editor_select_panel = ui_widget_panel(main_ui, 
                                                              STR("editor_select_panel"),
                                                              vec2(0, 0), 
                                                              vec2(20, 20),
                                                              vec2(10.0f, 10.0f), 
                                                              vec4(0, 0, 0, 0), 
                                                              vec4_zero(),
                                                              0);
            ui_row(main_ui, editor_select_panel.widget)
            {
                ui_signal_t editor_buttons = ui_widget_panel(main_ui, 
                                                             STR("game_editor_button_panel"),
                                                             vec2(0, 0), 
                                                             vec2(20, 20),
                                                             vec2(10.0f, 10.0f), 
                                                             vec4(0, 0, 0, 0), 
                                                             vec4_zero(),
                                                             0);
                ui_column(main_ui, editor_buttons.widget)
                {
                    ui_widget_labeled_button(main_ui, STR("Save Map"));
                    ui_widget_labeled_button(main_ui, STR("Test Load map"));
                    ui_widget_labeled_button(main_ui, STR("Entity Selection"));
                    ui_widget_labeled_button(main_ui, STR("Tester BLABAH"));
                }

                ui_signal_t test_display_panel = ui_widget_panel(main_ui, 
                                                                 STR("TEST display panel"),
                                                                 vec2(0, 0), 
                                                                 vec2(20, 20),
                                                                 vec2(10.0f, 10.0f), 
                                                                 vec4(0, 0, 0, 0), 
                                                                 vec4(0.1, 0.1, 0.1, 0.4),
                                                                 UI_WIDGET_FLAG_INTERACTABLE|UI_WIDGET_FLAG_RESIZEABLE);
                ui_row(main_ui, test_display_panel.widget)
                {
                    RHI_set_texture_filter_mode(RHI_context, player_sprite->texture, RHI_IMAGE_FILTER_TYPE_NEAREST);
                    ui_widget_texture(main_ui, 
                                      STR("test atlas"), 
                                      vec2(1.0, 1.0), 
                                      player_sprite, 
                                      vec2_zero(), 
                                      vec2(player_sprite->texture->bitmap.width, player_sprite->texture->bitmap.height), 
                                      {UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT, UI_WIDGET_SIZE_KIND_PERCENT_OF_PARENT},
                                      0);
                }
            }
        }
    }
}

internal_api entity_t*
entity_player_create(game_state_t *game_state, asset_manager_t *asset_manager, vec2_t initial_position)
{
    entity_t *result = null;

    u32 entity_flags = ENTITY_FLAG_USES_TRANSFORM|ENTITY_FLAG_HAS_SPRITE|ENTITY_FLAG_GRAVITIC|ENTITY_FLAG_ACTOR|ENTITY_FLAG_HAS_COLLIDER|ENTITY_FLAG_ANIMATED|ENTITY_FLAG_GRAVITIC;
    result = s_entity_create(game_state->entity_manager, ENTITY_ARCHETYPE_PLAYER, entity_flags);
    
    asset_handle_t player_sprite = s_asset_manager_acquire_asset_handle(asset_manager, STR("player_sprite_sheet"));

    result->sprite = player_sprite;
    result->direction_x = 1;

    result->position = vec2(0,  40);
    result->size     = vec2(15, 18);
    result->animation_state = PLAYER_ANIMATION_STATE_RUNNING;

    result->editor_position = initial_position;
    result->position        = initial_position;

    result->animations      = c_arena_push_array(&gc->persistent_arena, animation2D_t, PLAYER_ANIMATION_STATE_COUNT);
    result->animation_count = PLAYER_ANIMATION_STATE_COUNT;
    result->bounding_box    = rect2_create(result->position, result->size);

    result->friction         = vec2_create(100);
    result->max_acceleration = vec2_create(100);
    result->max_velocity     = vec2(3, 15);

    // idle
    local_persist duration_counter_t idle_counter = {
        .duration_ms = 1000,
        .looped      = true
    };

    local_persist animation_source2D_t player_idle = {
        .name         = STR("player_idle"),
        .texture      = player_sprite,
        .frame_count  = 2,
        .frame_size   = ivec2(15, 18),
        .frame_offset = ivec2(0, 1),
        .frame_timer  = idle_counter
    };

    // running
    local_persist duration_counter_t running_counter = {
        .duration_ms = 75,
        .looped = true
    };

    local_persist animation_source2D_t player_running = {
        .name         = STR("player_running"),
        .texture      = player_sprite,
        .frame_count  = 12,
        .frame_size   = ivec2(16, 18),
        .frame_offset = ivec2(1, 0),
        .frame_timer  = running_counter
    };

    // animation instances
    animation2D_t player_idle_instance = {
        .animation_info = player_idle,
        .frame_timer    = idle_counter
    };

    animation2D_t player_running_instance = {
        .animation_info = player_running,
        .frame_timer    = running_counter
    };

    // init
    result->animations[PLAYER_ANIMATION_STATE_IDLE]    = player_idle_instance;
    result->animations[PLAYER_ANIMATION_STATE_RUNNING] = player_running_instance;

    return(result);
}

internal_api entity_t*
entity_test_collider_create(game_state_t *game_state, vec2_t position, vec2_t size, u32 flags)
{
    entity_t *result = s_entity_create(game_state->entity_manager, ENTITY_ARCHETYPE_COLLIDER, ((ENTITY_FLAG_USES_TRANSFORM|ENTITY_FLAG_HAS_COLLIDER|ENTITY_FLAG_STATIC) | flags));
    result->archetype       = ENTITY_ARCHETYPE_COLLIDER;
    result->position        = position;
    result->editor_position = position;
    result->size            = size;

    result->bounding_box = rect2_create(position, size);

    return(result);
}

internal_api void
render_collider(game_state_t *game_state, render_state_t *render_state, RHI_command_list_t *command_list, entity_t *entity)
{
    vec2_t render_position = entity->bounding_box.min;
    if(game_state->game_mode == GAME_MODE_EDIT_MODE)
    {
        render_position = entity->render_position;
    }

    vec4_t color = vec4(0.5f, 0.0f, 0.0f, 0.2f);
    immediate_rect_ex(command_list, 
                     &render_state->vertex_buffer,
                      vec2_expand_vec3(render_position, 0.6f),
                      vec2_multiply(entity->bounding_box.half_size, vec2(2, 2)),
                      color,
                      vec2_zero(),
                      vec2_zero(),
                      vec2_zero(),
                      vec2_zero(),
                      vec2_zero());
}

internal_api void
entity_render(render_state_t *render_state, RHI_command_list_t *command_list, entity_t *entity)
{
    texture2D_t *texture = null;
    vec2_t       uv_min  = vec2_zero();
    vec2_t       uv_max  = vec2_zero();

    if(entity->flags & ENTITY_FLAG_HAS_SPRITE)
    {
        subtexture_data_t *data = entity->sprite.slot->subtexture_data;
        if(entity->animations && data)
        {
            // NOTE(Sleepster): Animated entity 
            animation2D_t        *current_animation_state = &entity->animations[entity->animation_state];
            animation_source2D_t *animation_data          = &current_animation_state->animation_info;

            texture = &data->atlas->texture;

            float32 frame_x = data->offset.x + (animation_data->frame_size.x * animation_data->frame_offset.x) + (animation_data->frame_size.x * current_animation_state->current_frame);
            float32 frame_y = data->offset.y + (animation_data->frame_size.y * animation_data->frame_offset.y);

            float32 frame_w = animation_data->frame_size.x;
            float32 frame_h = animation_data->frame_size.y;

            if(entity->direction_x > 0)
            {
                uv_min = vec2(frame_x, frame_y);
                uv_max = vec2(frame_x + frame_w, frame_y + frame_h);
            }
            else
            {
                uv_min = vec2(frame_x + frame_w, frame_y);
                uv_max = vec2(frame_x, frame_y + frame_h);
            }
        }
        else
        {
            // NOTE(Sleepster): If it's a static entity, then we'll just use the subtexture UVs 
            if(data)
            {
                texture = &data->atlas->texture;
                uv_min  =  data->uv_min;
                uv_max  =  data->uv_max;
            }
            else
            {
                texture = entity->sprite.texture;
                uv_min  = vec2(0.0f, 0.0f);
                uv_max  = vec2(1.0f, 1.0f);
            }
        }
    }
    
    immediate_quad_ex(command_list,
                     &render_state->vertex_buffer, 
                      vec2_expand_vec3(entity->render_position, 0.8f), 
                      entity->size, 
                      vec4(1.0, 1.0, 1.0, 1.0),
                      uv_min,
                      uv_max,
                      vec2_zero(),
                      vec2_zero(),
                      vec2_zero(),
                      texture);
}

internal_api void
create_test_environment(game_state_t *game_state, asset_manager_t *asset_manager)
{
    (void)asset_manager;
#if 1
    entity_t *top_wall    = entity_test_collider_create(game_state, vec2(-160,  80), vec2(320, 20),  0);
    entity_t *bottom_wall = entity_test_collider_create(game_state, vec2(-160, -90), vec2(320, 20),  ENTITY_FLAG_IS_GROUND);
    entity_t *left_wall   = entity_test_collider_create(game_state, vec2(-160, -80), vec2(20,  180), 0);
    entity_t *right_wall  = entity_test_collider_create(game_state, vec2( 140, -80), vec2(20,  180), 0);

    (void)top_wall;
    (void)bottom_wall;
    (void)left_wall;
    (void)right_wall;
#else
    entity_t *top_wall = entity_test_collider_create(game_state, vec2(-25, -25), vec2(50, 50));
#endif
}

internal_api void
r_init_render_state(render_state_t *render_state)
{
    // NOTE(Sleepster): Clear colors 
    RHI_clear_value_t color_buffer_clear_value = {
        .float_color = {0.01f, 0.01f, 0.01f, 1.0f},
    };

    RHI_clear_value_t depth_buffer_clear_value = {
        .depth   = 1.0f,
        .stencil = 0
    };

    // NOTE(Sleepster): Game Renderpass
    RHI_renderpass_desc_t game_renderpass_desc;
    render_state->game_color_buffer = {};
    render_state->game_depth_buffer = {};
    {
        RHI_image_create_info_t primary_game_color_buffer_create_info = {
            .width  = GAME_FRAMEBUFFER_WIDTH,
            .height = GAME_FRAMEBUFFER_HEIGHT,
            .format = BMF_RGBA32_UNORM,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT
        };

        RHI_image_create_info_t primary_game_depth_buffer_create_info = {
            .width  = GAME_FRAMEBUFFER_WIDTH,
            .height = GAME_FRAMEBUFFER_HEIGHT,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT 
        };

        render_state->game_color_buffer = RHI_image_create(render_state->RHI_context, &primary_game_color_buffer_create_info);
        render_state->game_depth_buffer = RHI_image_create(render_state->RHI_context, &primary_game_depth_buffer_create_info);

        game_renderpass_desc = {
            .render_width           = GAME_FRAMEBUFFER_WIDTH,
            .render_height          = GAME_FRAMEBUFFER_HEIGHT,
            .color_attachment_count = 1,
            .resize_with_window     = false,
            .color_attachments = {
                [0] = {
                    .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                    .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                    .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                    .image           = &render_state->game_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                .image           = &render_state->game_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    // NOTE(Sleepster): Fullscreen Renderpass
    RHI_renderpass_desc_t fullscreen_renderpass_desc;
    render_state->fullscreen_color_buffer = {};
    render_state->fullscreen_depth_buffer = {};
    {
        RHI_image_create_info_t fullscreen_color_buffer_create_info = {
            .width  = (u32)render_state->RHI_context->window_size.x,
            .height = (u32)render_state->RHI_context->window_size.y,
            .format = BMF_RGBA32_UNORM,
            .usage  = (RHI_image_usage_t)(RHI_IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT|RHI_IMAGE_USAGE_BLIT_SOURCE),
        };

        RHI_image_create_info_t fullscreen_depth_buffer_create_info = {
            .width  = (u32)render_state->RHI_context->window_size.x,
            .height = (u32)render_state->RHI_context->window_size.y,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT,
        };

        render_state->fullscreen_color_buffer = RHI_image_create(render_state->RHI_context, &fullscreen_color_buffer_create_info);
        render_state->fullscreen_depth_buffer = RHI_image_create(render_state->RHI_context, &fullscreen_depth_buffer_create_info);

        fullscreen_renderpass_desc = {
            .render_width           = (u32)render_state->RHI_context->window_size.x,
            .render_height          = (u32)render_state->RHI_context->window_size.y,
            .color_attachment_count = 1,
            .resize_with_window     = true,
            .color_attachments = {
                [0] = {
                    .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                    .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_LOAD,
                    .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                    .image           = &render_state->fullscreen_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                .image           = &render_state->fullscreen_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    render_state->game_renderpass_ID       = RHI_build_renderpass(render_state->RHI_context, &game_renderpass_desc);
    render_state->fullscreen_renderpass_ID = RHI_build_renderpass(render_state->RHI_context, &fullscreen_renderpass_desc);

    u32 *indices = c_arena_push_array(&render_state->RHI_context->transient_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
    u32  index_offset = 0;
    for(u32 index = 0;
        index < 60000;
        index += 6)
    {
        indices[index + 0] = index_offset + 0;
        indices[index + 1] = index_offset + 1;
        indices[index + 2] = index_offset + 2;
        indices[index + 3] = index_offset + 2;
        indices[index + 4] = index_offset + 3;
        indices[index + 5] = index_offset + 0;

        index_offset += 4;
    }

    const u32 VERTEX_BUFFER_SIZE = 4 * 10000;
    immediate_vertex_t *vertices = c_arena_push_array(&render_state->RHI_context->RHI_arena, immediate_vertex_t, VERTEX_BUFFER_SIZE);
    render_state->vertex_buffer = RHI_vertex_buffer_create(render_state->RHI_context, 
                                                           RHI_RENDER_BUFFER_ALLOCATION_TYPE_MAPPED, 
                                                           RHI_RENDER_BUFFER_ADVANCE_RATE_PER_ELEMENT, 
                                                           (byte*)vertices, 
                                                           sizeof(immediate_vertex_t), 
                                                           VERTEX_BUFFER_SIZE);
    render_state->index_buffer  = RHI_index_buffer_create(render_state->RHI_context,  
                                                          RHI_RENDER_BUFFER_ALLOCATION_TYPE_GPU_ONLY, 
                                                          sizeof(u32),
                                                          indices, 
                                                          (sizeof(u32) * (6 * MAX_ENTITIES)));
}

internal_api void
game_state_init_bindings(game_state_t *game_state, input_manager_t *input_manager)
{
    // NOTE(Sleepster): Movement 
    game_action_t *movement_action = s_im_game_action_create(input_manager, STR("character move"), INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_AXIS2D);

    game_action_mapping_t keyboard_movement_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_SCANCODE_W}, {SDL_SCANCODE_S}, {SDL_SCANCODE_A}, {SDL_SCANCODE_D}},
        .binding_count = 4,
        .controller_type = INPUT_DEVICE_TYPE_KEYBOARD
    };
    s_im_game_action_add_mapping(movement_action, &keyboard_movement_mapping);

    game_action_mapping_t controller_movement_mapping = (game_action_mapping_t) {
        .bindings = {
            {SDL_GAMEPAD_AXIS_LEFTY, INPUT_MANAGER_BINDING_TYPE_JOYSTICK}, 
            {SDL_GAMEPAD_AXIS_LEFTX, INPUT_MANAGER_BINDING_TYPE_JOYSTICK}
        },
        .binding_count   = 2,
        .controller_type = INPUT_DEVICE_TYPE_GAMEPAD 
    };
    s_im_game_action_add_mapping(movement_action, &controller_movement_mapping);
    game_state->mappings.move = movement_action;

    // NOTE(Sleepster): Jump
    game_action_t *jump_action = s_im_game_action_create(input_manager, STR("Jump"), INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON);
    game_action_mapping_t keyboard_jump_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_SCANCODE_SPACE}},
        .binding_count = 1,
        .controller_type = INPUT_DEVICE_TYPE_KEYBOARD
    };
    s_im_game_action_add_mapping(jump_action, &keyboard_jump_mapping);

    game_action_mapping_t gamepad_jump_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_GAMEPAD_BUTTON_SOUTH}},
        .binding_count = 1,
        .controller_type = INPUT_DEVICE_TYPE_GAMEPAD
    };
    s_im_game_action_add_mapping(jump_action, &gamepad_jump_mapping);
    game_state->mappings.jump = jump_action;

    // NOTE(Sleepster): Dash 
    game_action_t *dash_action = s_im_game_action_create(input_manager, STR("Dash"), INPUT_MANAGER_GAME_ACTION_MAPPING_TYPE_BUTTON);
    game_action_mapping_t keyboard_dash_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_SCANCODE_LSHIFT}},
        .binding_count = 1,
        .controller_type = INPUT_DEVICE_TYPE_KEYBOARD 
    };
    s_im_game_action_add_mapping(dash_action, &keyboard_dash_mapping);

    game_action_mapping_t gamepad_dash_mapping = (game_action_mapping_t) {
        .bindings = {{SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1}},
        .binding_count = 1,
        .controller_type = INPUT_DEVICE_TYPE_GAMEPAD
    };
    s_im_game_action_add_mapping(dash_action, &gamepad_dash_mapping);
    game_state->mappings.dash = dash_action;
}

internal_api void
poll_player_input(game_state_t *game_state)
{
    game_state->input_info.movement_axis = game_state->mappings.move->axis2D_value;
    game_state->input_info.jumped        = GameActionPressed(game_state->mappings.jump);
    game_state->input_info.dashed        = GameActionPressed(game_state->mappings.dash);
}

internal_api void
entity_transition_animation(entity_t *entity, u32 new_state)
{
    animation2D_t *current_animation = entity->animations + entity->animation_state;
    if(new_state != entity->animation_state)
    {
        current_animation->current_frame = 0;
        c_duration_counter_reset(&current_animation->frame_timer);
    }

    entity->animation_state = new_state;
}

internal_api void
game_state_simulate(game_state_t *game_state)
{
    vec2_t movement_axis_value = game_state->input_info.movement_axis;
    bool8 jumped               = game_state->input_info.jumped;
    bool8 dashed               = game_state->input_info.dashed;
    (void)dashed;

    entity_query_t player_query = s_entity_query_archetype(game_state->entity_manager, ENTITY_ARCHETYPE_PLAYER);
    for(entity_t *entity: player_query)
    {
        // NOTE(Sleepster): X Movement 
        entity->acceleration.x += movement_axis_value.x * 100;
        entity->acceleration.x  = Clamp(entity->acceleration.x, -entity->max_acceleration.x, entity->max_acceleration.x);
        entity->acceleration.y  = Clamp(entity->acceleration.y, -entity->max_acceleration.y, entity->max_acceleration.y);
        if(movement_axis_value.x == 0.0f)
        {
            entity->acceleration.x = 0.0f;
        }

        // NOTE(Sleepster): Y Movement
        if(jumped && (entity->flags & ENTITY_FLAG_GROUNDED))
        {
            entity->acceleration.y = entity->max_acceleration.y * 1.0f;
            entity->flags &= ~ENTITY_FLAG_GROUNDED;
        }

        entity->velocity.x = entity->velocity.x + ((entity->acceleration.x * 100) * gc->tick_rate);
        entity->velocity.y = entity->velocity.y + ((entity->acceleration.y) * gc->tick_rate);

        entity->velocity.x = Clamp(entity->velocity.x, -entity->max_velocity.x, entity->max_velocity.x);
        entity->velocity.y = Clamp(entity->velocity.y, -entity->max_velocity.y, entity->max_velocity.y);
        if(entity->acceleration.x != 0) entity_transition_animation(entity, PLAYER_ANIMATION_STATE_RUNNING);
        else                            entity_transition_animation(entity, PLAYER_ANIMATION_STATE_IDLE);
    }

    entity_query_t gravity_query = s_entity_query_flags_exact(game_state->entity_manager, ENTITY_FLAG_ACTOR|ENTITY_FLAG_GRAVITIC);
    for(entity_t *entity: gravity_query)
    {
        //f32_approach(&entity->acceleration.y, entity->max_acceleration.y, game_state->gravity, gc->tick_rate);
        entity->acceleration.y += (game_state->gravity * (gc->tick_rate * 1));
        if(entity->flags & ENTITY_FLAG_GROUNDED)
        {
            entity->acceleration.y = 0;
        }
    }

    entity_query_t collision_query = s_entity_query_flags_exact(game_state->entity_manager, ENTITY_FLAG_HAS_COLLIDER);
    entity_query_t actor_query     = s_entity_query_flags_exact(game_state->entity_manager, ENTITY_FLAG_ACTOR|ENTITY_FLAG_HAS_COLLIDER);
    for(entity_t *entity: actor_query)
    {
        for(entity_t *collider: collision_query)
        {
            vec2_t target_velocity = entity->velocity;

            vec2_t target_velocity_x = vec2(target_velocity.x, 0.0f);
            vec2_t target_velocity_y = vec2(0.0f, target_velocity.y);
            if(collider != entity)
            {
                collision_query_result_t collision = s_entity_test_collisions(entity, collider);
                if(collision.hit)
                {
                    if(collision.x.hit)
                    {
                        // NOTE(Sleepster): Find our time of impact, with a bit of offset (an Epsilon)
                        // to prevent the object from being stuck inside another object.
                        //
                        // We then use this time of impact and predict the change in velocity for this frame by saying
                        //
                        // V1 = V0 * T
                        //
                        // Where T is the Time of Impact we recorded ranging between 0.0f - 1.0f.
                        //
                        // 0.0f meaning we either never hit, or is immediate
                        // 1.0f meaning at the end of the timestep (which is 16.667ms)
                        //
                        // Essentially, TOI is just a value that is meant to tell us when between right now (0.0f) and 
                        // the end of the timestep (1.0f) we will hit the object.
                        float32 our_TOI = Max(0.0f, collision.x.toi - 0.0001f);
                        float32 displacement = target_velocity_x.x * our_TOI;

                        // NOTE(Sleepster): A little bit of bias to the velocity to prevent getting stuck 
                        float32 Vbias = ((0.1f * our_TOI) * Max(0.0f, collision.depth - 0.0001f)) * collision.x.normal.x;
                        target_velocity.x = displacement + Vbias;
                    }

                    if(collision.y.hit)
                    {
                        // NOTE(Sleepster): Do the same as above for our Y axis 
                        float32 our_TOI = Max(0.0f, collision.y.toi - 0.0001f);
                        float32 displacement = target_velocity_y.y * our_TOI;

                        float32 Vbias = ((0.1f * our_TOI) * Max(0.0f, collision.depth - 0.0001f)) * collision.y.normal.y;
                        target_velocity.y = displacement + Vbias;
                    }

                    if(collider->flags & ENTITY_FLAG_IS_GROUND)
                    {
                        entity->flags |= ENTITY_FLAG_GROUNDED;
                    }
                }
            }
            entity->velocity = target_velocity;
        }
    }

    entity_query_t transform_query = s_entity_query_flags_exact(game_state->entity_manager, ENTITY_FLAG_USES_TRANSFORM|ENTITY_FLAG_ACTOR);
    for(entity_t *entity: transform_query)
    {
        entity->last_position = entity->position;
        entity->position = vec2_add(entity->position, entity->velocity);

        if(entity->velocity.x < 0.0) entity->direction_x = -1;
        if(entity->velocity.x > 0.0) entity->direction_x =  1;

        entity->velocity = vec2_zero();
        rect2_shift_by(&entity->bounding_box, vec2_subtract(entity->position, entity->last_position));
    }

    entity_query_t animated_sprites_query = s_entity_query_flags(game_state->entity_manager, ENTITY_FLAG_ANIMATED);
    for(entity_t *entity: animated_sprites_query)
    {
        animation2D_t *animation = &entity->animations[entity->animation_state];
        if(c_duration_counter_advance(&animation->frame_timer, gc->tick_rate_ms))
        {
            animation->current_frame = (animation->current_frame + 1) % animation->animation_info.frame_count;
        }
    }
}

internal_api void
DEBUG_toggle_state_recording(void)
{
    gc->recording_input = !gc->recording_input;
    if(gc->recording_input)
    {
        gc->saved_arena_size = gc->persistent_arena.used;
        
        c_file_close(&gc->input_manager_playback_file);
        gc->input_manager_playback_file = c_file_open(gc->input_manager_playback_file.filepath, true, true);

        c_file_write(&gc->input_manager_playback_file, (byte*)gc->persistent_arena.base, gc->saved_arena_size);
        gc->playing_back_input = false;
    }
}

internal_api void
DEBUG_toggle_state_playback(void)
{
    gc->playing_back_input = !gc->playing_back_input;
    if(gc->playing_back_input)
    {
        gc->input_manager_playback_file.current_read_offset = 0;
        c_file_read(&gc->input_manager_playback_file, gc->persistent_arena.base, gc->saved_arena_size); 
    }
}

internal_api void
DEBUG_update_state_recording_and_playback(void)
{
    if(gc->recording_input)
    {
        c_file_write(&gc->input_manager_playback_file, (byte*)gc->input_manager, sizeof(input_manager_t));
    }

    if(gc->recording_input)
    {
        Assert(!gc->playing_back_input)
    }

    if(gc->playing_back_input)
    {
        if(!c_file_read(&gc->input_manager_playback_file, (byte*)gc->input_manager, sizeof(input_manager_t)))
        {
            gc->input_manager_playback_file.current_read_offset = 0;
            c_file_read(&gc->input_manager_playback_file, gc->persistent_arena.base, gc->saved_arena_size); 
            c_file_read(&gc->input_manager_playback_file, (byte*)gc->input_manager, sizeof(input_manager_t));
        }
    }
}

internal_api void
handle_editor_ui(game_state_t *game_state, ui_state_t *main_ui, RHI_context_t *RHI_context)
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
        ui_widget_set_default_font_size(main_ui, 25);
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
                    game_state->show_grid = !game_state->show_grid;
                }

                ui_signal_t show_viewport = ui_widget_labeled_button(main_ui, STR("Toggle Editor Player Viewport"));
                if(ui_pressed(show_viewport))
                {
                    game_state->show_player_viewport = !game_state->show_player_viewport;
                }

                ui_signal_t reset_editor_camera_translation = ui_widget_labeled_button(main_ui, STR("Reset Editor Camera Translation"));
                if(ui_pressed(reset_editor_camera_translation))
                {
                    game_state->editor_camera.translation = vec2_zero();
                }
                ui_widget_text(main_ui, STR("Click this if the camera flies away!"));
            }
        }
    }
}

internal_api vec2_t
world_to_tile(vec2_t world_position)
{
    vec2_t result;

    result.x = floorf(world_position.x / WORLD_TILE_SIZE);
    result.y = floorf(world_position.y / WORLD_TILE_SIZE);

    return(result);
}

external int
game_main(global_context_t *_global_context)
{
    gc = _global_context;
    Assert(gc);
    if(!gc->game_state)
    {
        gc->game_state          = c_arena_push_struct(&gc->persistent_arena, game_state_t);
        gc->game_state->main_ui = c_arena_push_struct(&gc->persistent_arena, ui_state_t);
    }

    game_state_t    *game_state    =  gc->game_state;
    input_manager_t *input_manager =  gc->input_manager;
    asset_manager_t *asset_manager =  gc->asset_manager;
    render_state_t  *render_state  = &game_state->render_state;
    ui_state_t      *main_ui       =  game_state->main_ui;
    if(!game_state->is_initialized)
    {
        srand(rdtsc());

        //game_state->controller     = s_im_get_controller_from_active_device(input_manager, game_state->controller);
        game_state->entity_manager = c_arena_push_struct(&gc->persistent_arena, entity_manager_t);
        game_state->entity_manager->transient_storage = c_arena_create(MB(100), ALLOCATOR_TAG_GAME);

        game_state->gravity = -120.0f;

        render_state->RHI_context = gc->RHI_context;
        r_init_render_state(render_state);
        render_state->camera_matrices_buffer = RHI_get_constant_buffer(render_state->RHI_context, STR("CameraMatrices"));

        ui_state_init(main_ui, input_manager, asset_manager, render_state->RHI_context, render_state->fullscreen_renderpass_ID);

        global_test_textbox_string.data  = (byte*)c_alloc(256, ALLOCATOR_TAG_CACHE);
        global_test_textbox_string.count = 0;

        game_state->editor_zoom          = 2.5f;
        game_state->show_grid            = true;
        game_state->show_player_viewport = true;

        // GAME INIT
        entity_player_create(game_state, asset_manager, vec2(0, 0));
        create_test_environment(game_state, asset_manager);
        // GAME INIT

        // NOTE(Sleepster): Init bindings 
        game_state_init_bindings(game_state, gc->input_manager);
        // NOTE(Sleepster): Init bindings 

        game_state->is_initialized = true;
    }

    // TODO(Sleepster): Some system for managing and storing loaded assets (oh... so like an asset_manager????) so we don't need to constantly create
    // handles.
    asset_handle_t immediate_textured  = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_textured_unnormalized"));
    asset_handle_t immediate_rectangle = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_rectangle"));
    asset_handle_t immediate_font      = s_asset_manager_acquire_asset_handle(asset_manager, STR("immediate_font"));
    asset_handle_t player_sprite       = s_asset_manager_acquire_asset_handle(asset_manager, STR("player"));
    asset_handle_t basic_font          = s_asset_manager_acquire_asset_handle(asset_manager, STR("LiberationMono_Regular"));
    asset_handle_t player_sprite_sheet = s_asset_manager_acquire_asset_handle(asset_manager, STR("player_sprite_sheet"));

    texture_atlas_t *atlas = s_texture_atlas_create(asset_manager, 1024, 4, BMF_RGBA32_SRGB, 32);
    s_texture_atlas_add_texture(atlas, &player_sprite);
    s_texture_atlas_add_texture(atlas, &player_sprite_sheet);

    game_state->game_camera = {
        .viewport = {
            .x = GAME_FRAMEBUFFER_WIDTH,
            .y = GAME_FRAMEBUFFER_HEIGHT
        },
        .translation = {0.0f, 0.0f},
        .zoom = 1.0f
    };

    game_state->fullscreen_camera = {
        .viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                         Max(render_state->RHI_context->window_size.y, 10)),
        .translation = {0.0f, 0.0f},
        .zoom = 1.0f
    };

    game_state->editor_camera = {
        .viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                         Max(render_state->RHI_context->window_size.y, 10)),
        .translation = {0.0f, 0.0f},
        .zoom = game_state->editor_zoom 
    };

    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    u64 last_tsc        = SDL_GetPerformanceCounter();
    u64 current_tsc     = 0;
    u64 delta_tsc       = 0;

    float32 delta_time     = 0;
    float64 dt_accumulator = 0.0f;
    //float32 delta_time_ms = 0;
    while(gc->running)
    {
        //s_im_reset_controller_states(input_manager);
        process_window_events(render_state->RHI_context, input_manager);
#ifndef RELEASE
        c_file_watcher_process_changes(&gc->file_watcher);
#endif
        game_state->controller = s_im_get_controller_from_active_device(input_manager, game_state->controller);
        if(game_state->open_debug_menu || game_state->editor_opened)
        {
            ui_state_get_input_events(main_ui);
            ui_state_begin_frame(main_ui);
            if(game_state->open_debug_menu)
            {
                handle_debug_ui_menu(main_ui, render_state->RHI_context, &player_sprite);
            }

            if(game_state->editor_opened)
            {
                handle_editor_ui(game_state, main_ui, render_state->RHI_context);
            }
        }

        game_state->game_camera.viewport = vec2(GAME_FRAMEBUFFER_WIDTH, GAME_FRAMEBUFFER_HEIGHT);
        game_state->game_camera.zoom = 1.0f;

        game_state->fullscreen_camera.viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                                          Max(render_state->RHI_context->window_size.y, 10));
        game_state->fullscreen_camera.zoom = 1.0f;

        game_state->editor_camera.zoom = game_state->editor_zoom;
        game_state->editor_camera.viewport = vec2(Max(render_state->RHI_context->window_size.x, 10),
                                                  Max(render_state->RHI_context->window_size.y, 10));

        RHI_render_camera_set_matrices(&game_state->game_camera);
        RHI_render_camera_set_matrices(&game_state->fullscreen_camera);
        RHI_render_camera_set_matrices(&game_state->editor_camera);

        // NOTE(Sleepster): Editor controls block 
        if(game_state->editor_opened)
        {
            action_button_t *middle_mouse = s_im_get_controller_action_button(game_state->controller, SDL_MIDDLE_MOUSE);
            vec2_t scroll_delta     = game_state->controller->device->keyboard_data.current_mouse_wheel;
            vec2_t mouse_move_delta = game_state->controller->device->keyboard_data.mouse_delta;

            // NOTE(Sleepster): Panning 
            if(ActionButtonDown(middle_mouse))
            {
                game_state->editor_camera.translation.x -= (mouse_move_delta.x * 3) * game_state->editor_camera.zoom;
                game_state->editor_camera.translation.y += (mouse_move_delta.y * 3) * game_state->editor_camera.zoom;
            }

            // NOTE(Sleepster): Zoom adjustment 
            if(scroll_delta.y != 0.0f)
            {
                game_state->editor_zoom += (scroll_delta.y * 0.5f);
                game_state->editor_zoom  = Clamp(game_state->editor_zoom, 2.0f, 5.0f);
            }

            // NOTE(Sleepster): Tile placing 
            action_button_t *left_mouse = s_im_get_controller_action_button(game_state->controller, SDL_LEFT_MOUSE);
            if(ActionButtonDown(left_mouse))
            {
                vec2_t mouse_position = s_im_transform_mouse_data(game_state->controller, 
                                                                  game_state->editor_camera.viewport, 
                                                                  game_state->editor_camera.matrices.view_matrix, 
                                                                  game_state->editor_camera.matrices.projection_matrix);
                vec2_t tile_position = world_to_tile(mouse_position);
                (void)tile_position;
            }

            game_state->controller->device->keyboard_data.mouse_delta = {};
            game_state->controller->device->keyboard_data.current_mouse_wheel = {};
        }
        // NOTE(Sleepster): Editor controls block 

        // NOTE(Sleepster):} Simulate loop 
        if(delta_time >= (gc->tick_rate * 2.0f))
        {
            delta_time = gc->tick_rate * 2.0f;
        }

        bool8 first_tick = true;
        dt_accumulator  += delta_time;
        while(dt_accumulator >= gc->tick_rate)
        {
            if(first_tick)
            {
                // NOTE(Sleepster): Apply events. 
                s_im_apply_events_to_controller(game_state->controller, game_state->controller->device->events, true);

                if(game_state->controller->type == INPUT_DEVICE_TYPE_KEYBOARD)
                {
                    // NOTE(Sleepster): Debug menu 
                    if(s_im_is_button_pressed(game_state->controller, SDL_SCANCODE_SEMICOLON))
                    {
                        game_state->open_debug_menu = !game_state->open_debug_menu;
                    }

                    // NOTE(Sleepster): Edit mode 
                    if(s_im_is_button_pressed(game_state->controller, SDL_SCANCODE_E))
                    {
                        game_state->editor_opened = !game_state->editor_opened;

                        if(game_state->editor_opened) 
                        {
                            game_state->game_mode = GAME_MODE_EDIT_MODE;
                            for(u32 entity_index = 0;
                                entity_index < game_state->entity_manager->active_entities;
                                ++entity_index)
                            {
                                entity_t *entity = game_state->entity_manager->entities + entity_index;
                                entity->render_position = entity->editor_position;
                            }
                        }
                        else
                        {
                            game_state->game_mode = GAME_MODE_NORMAL;
                            for(u32 entity_index = 0;
                                entity_index < game_state->entity_manager->active_entities;
                                ++entity_index)
                            {
                                entity_t *entity = game_state->entity_manager->entities + entity_index;
                                entity->render_position = entity->position;
                            }
                        }
                    }

                    // NOTE(Sleepster): Input and state recording 
                    if(s_im_is_button_pressed(game_state->controller, SDL_SCANCODE_R))
                    {
                        DEBUG_toggle_state_recording();
                    }

                    // NOTE(Sleepster): Input and state looping 
                    if(s_im_is_button_pressed(game_state->controller, SDL_SCANCODE_L))
                    {
                        DEBUG_toggle_state_playback();
                    }
                    DEBUG_update_state_recording_and_playback();

                    // NOTE(Sleepster): Respawn the player 
                    if(s_im_is_button_pressed(game_state->controller, SDL_SCANCODE_P))
                    {
                        entity_query_t player_query = s_entity_query_archetype(game_state->entity_manager, ENTITY_ARCHETYPE_PLAYER);
                        entity_t *player = player_query.entities[0];
                        ZeroStruct(*player);
                        s_entity_destroy(game_state->entity_manager, player);

                        entity_player_create(game_state, asset_manager, vec2(0, 0));
                    }
                }

                s_im_update_game_action_states(input_manager, game_state->controller);
                s_im_clear_device_events(game_state->controller->device);

                first_tick = false;
                c_global_context_reset_simulation_arena();
            }

            if(game_state->game_mode == GAME_MODE_NORMAL)
            {
                poll_player_input(game_state);
                game_state_simulate(game_state);
            }

            dt_accumulator -= gc->tick_rate;
        }
        game_state->render_alpha = (float32)(dt_accumulator / gc->tick_rate);

        RHI_command_list_t *command_list = RHI_get_command_list(render_state->RHI_context, RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
        {
            u32 renderpassID = render_state->game_renderpass_ID;
            if(game_state->game_mode == GAME_MODE_EDIT_MODE)
            {
                renderpassID = render_state->fullscreen_renderpass_ID;
            }

            RHI_cmd_renderpass_begin(command_list, renderpassID);
            if(game_state->game_mode == GAME_MODE_EDIT_MODE)
            {
                RHI_cmd_clear_renderpass_attachments(command_list, render_state->fullscreen_renderpass_ID);
            }

            if(game_state->game_mode == GAME_MODE_EDIT_MODE)
            {
                RHI_render_camera_t *scene_camera = &game_state->editor_camera;

                // NOTE(Sleepster): Tile Grid 
                if(game_state->show_grid)
                {
                    float32 half_width  = (scene_camera->viewport.x * 0.5f) * scene_camera->zoom;
                    float32 half_height = (scene_camera->viewport.y * 0.5f) * scene_camera->zoom;

                    float32 left   = scene_camera->translation.x - half_width;
                    float32 right  = scene_camera->translation.x + half_width;
                    float32 bottom = scene_camera->translation.y - half_height;
                    float32 top    = scene_camera->translation.y + half_height;

                    vec2_t start = world_to_tile(vec2(left,  bottom));
                    vec2_t end   = world_to_tile(vec2(right, top));

                    for(s32 tile_x = start.x;
                        tile_x <= end.x;
                        ++tile_x)
                    {
                        float32 current_tile_x = tile_x * WORLD_TILE_SIZE;
                        immediate_line(command_list, 
                                       &render_state->vertex_buffer, 
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
                        immediate_line(command_list, 
                                       &render_state->vertex_buffer, 
                                       vec2(left, current_tile_y),
                                       vec2(right, current_tile_y),
                                       0.90,
                                       vec4(0.01, 0.01, 0.1, 1.0f));
                    }
                }

                // NOTE(Sleepster): Viewport box 
                if(game_state->show_player_viewport)
                {
                    float32 half_width  = (game_state->game_camera.viewport.x * 0.5f) * game_state->game_camera.zoom;
                    float32 half_height = (game_state->game_camera.viewport.y * 0.5f) * game_state->game_camera.zoom;

                    float32 left   = game_state->game_camera.translation.x - half_width;
                    float32 right  = game_state->game_camera.translation.x + half_width;
                    float32 bottom = game_state->game_camera.translation.y - half_height;
                    float32 top    = game_state->game_camera.translation.y + half_height;

                    immediate_line(command_list,
                                  &render_state->vertex_buffer,
                                   vec2(left,  bottom),
                                   vec2(left, top),
                                   0.0,
                                   vec4(1.0f, 1.0, 1.0f, 1.0f));

                    immediate_line(command_list,
                                  &render_state->vertex_buffer,
                                   vec2(left,  top),
                                   vec2(right, top),
                                   0.0,
                                   vec4(1.0f, 1.0, 1.0f, 1.0f));

                    immediate_line(command_list,
                                  &render_state->vertex_buffer,
                                   vec2(right, top),
                                   vec2(right, bottom),
                                   0.0,
                                   vec4(1.0f, 1.0, 1.0f, 1.0f));

                    immediate_line(command_list,
                                  &render_state->vertex_buffer,
                                   vec2(right, bottom),
                                   vec2(left,  bottom),
                                   0.0,
                                   vec4(1.0f, 1.0, 1.0f, 1.0f));
                }

                // NOTE(Sleepster): We explicitly do this here because the blit from before doesn't make any sense.
                // The problem as well is that the fullscreen renderpass uses a LOAD instead of a CLEAR. So we must
                // MANUALLY clear it.
                RHI_cmd_update_buffer_contents(command_list, &render_state->vertex_buffer);

                RHI_cmd_bind_vertex_buffer(command_list, &render_state->vertex_buffer);

                s32 window_width  = Max(render_state->RHI_context->window_size.x, 10);
                s32 window_height = Max(render_state->RHI_context->window_size.y, 10);

                RHI_cmd_update_constant_buffer(command_list, render_state->camera_matrices_buffer, &scene_camera->matrices, sizeof(mat4_t) * 2);

                RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
                RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

                RHI_pipeline_state_t line_drawing = {};
                line_drawing.blend_enabled  = false;
                line_drawing.polygon_mode   = RENDER_PIPELINE_POLYGON_MODE_LINE;
                line_drawing.primitive_type = RENDER_PIPELINE_PRIMITIVE_TOPOLOGY_LINE_LIST;
                RHI_cmd_set_render_state(command_list, &line_drawing);
                RHI_cmd_set_line_width(command_list, 2.0f);

                RHI_cmd_use_shader_program(command_list, immediate_rectangle);

                RHI_cmd_draw(command_list, render_state->vertex_buffer.vertex_count, 0, 1, 0);
                render_state->vertex_buffer.vertex_count = 0;
            }

            // NOTE(Sleepster): Draw entities 
            {
                for(u32 entity_index = 0;
                    entity_index < game_state->entity_manager->active_entities;
                    ++entity_index)
                {
                    entity_t *entity = game_state->entity_manager->entities + entity_index;
                    if((entity->flags & ENTITY_FLAG_IS_VALID) && 
                       (entity->archetype != ENTITY_ARCHETYPE_COLLIDER))
                    {
                        // NOTE(Sleepster): Update the entitie's render position if we are SIMULATING
                        if(game_state->game_mode == GAME_MODE_NORMAL)
                        {
                            entity->render_position = vec2_lerp(entity->last_position, entity->position, game_state->render_alpha);
                        }

                        entity_render(render_state, command_list, entity);
                    }
                }

                RHI_cmd_update_buffer_contents(command_list, &render_state->vertex_buffer);
                RHI_cmd_reset_render_state(command_list);

                RHI_cmd_bind_vertex_buffer(command_list, &render_state->vertex_buffer);
                RHI_cmd_bind_index_buffer(command_list,  &render_state->index_buffer);
                RHI_cmd_use_shader_program(command_list, immediate_textured);

                RHI_render_camera_t *scene_camera = (game_state->game_mode == GAME_MODE_NORMAL) ? &game_state->game_camera : &game_state->editor_camera;
                RHI_cmd_update_constant_buffer(command_list, render_state->camera_matrices_buffer, &scene_camera->matrices, sizeof(mat4_t) * 2);

                if(game_state->game_mode == GAME_MODE_NORMAL)
                {
                    s32 window_width  = GAME_FRAMEBUFFER_WIDTH;
                    s32 window_height = GAME_FRAMEBUFFER_HEIGHT;

                    RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
                    RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));
                }
                else
                {
                    s32 window_width  = Max(render_state->RHI_context->window_size.x, 10);
                    s32 window_height = Max(render_state->RHI_context->window_size.y, 10);

                    RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
                    RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));
                }

                RHI_cmd_draw_indexed(command_list, (render_state->vertex_buffer.vertex_count * 0.25f) * 6, 0, 0, 1, 0);
                RHI_vertex_buffer_reset_count(&render_state->vertex_buffer);
            }

            // NOTE(Sleepster): Draw misc 
            {
                for(u32 entity_index = 0;
                    entity_index < game_state->entity_manager->active_entities;
                    ++entity_index)
                {
                    entity_t *entity = game_state->entity_manager->entities + entity_index;
                    render_collider(game_state, render_state, command_list, entity);
                }

                immediate_rect(command_list, &render_state->vertex_buffer, vec3(0, 0, 0.4), vec2(8, 8), vec4(0.0, 1.0f, 0.0f, 1.0f));

                RHI_cmd_update_buffer_contents(command_list, &render_state->vertex_buffer);

                RHI_cmd_bind_vertex_buffer(command_list, &render_state->vertex_buffer);
                RHI_cmd_bind_index_buffer(command_list,  &render_state->index_buffer);

                RHI_pipeline_state_t collider_blending = {};
                collider_blending.blend_enabled = true;
                RHI_cmd_set_render_state(command_list, &collider_blending);

                RHI_cmd_set_line_width(command_list, 10.0f);

                RHI_cmd_use_shader_program(command_list,  immediate_rectangle);
                RHI_cmd_draw_indexed(command_list, (render_state->vertex_buffer.vertex_count * 0.25f) * 6, 0, 0, 1, 0);
                RHI_vertex_buffer_reset_count(&render_state->vertex_buffer);
            }

            RHI_cmd_renderpass_end(command_list);
        }

        // NOTE(Sleepster): Fullscreen Renderpass 
        {
            if(game_state->game_mode == GAME_MODE_NORMAL) 
            {
                RHI_cmd_blit_renderpass(command_list, render_state->game_renderpass_ID, render_state->fullscreen_renderpass_ID);
            }

            RHI_cmd_renderpass_begin(command_list, render_state->fullscreen_renderpass_ID);

            RHI_cmd_bind_vertex_buffer(command_list, &render_state->vertex_buffer);
            RHI_cmd_bind_index_buffer(command_list,  &render_state->index_buffer);

            RHI_render_camera_t *scene_camera = &game_state->fullscreen_camera;
            RHI_cmd_update_constant_buffer(command_list, render_state->camera_matrices_buffer, &scene_camera->matrices, sizeof(mat4_t) * 2);

            RHI_pipeline_state_t font_state = {};
            font_state.blend_enabled = true;
            font_state.src_alpha_blend_mode = RBM_SrcAlpha;
            font_state.dst_alpha_blend_mode = RBM_OneMinusSrcAlpha;
            RHI_cmd_set_render_state(command_list, &font_state);

            s32 window_width  = Max(render_state->RHI_context->window_size.x, 10);
            s32 window_height = Max(render_state->RHI_context->window_size.y, 10);

            RHI_cmd_use_shader_program(command_list, immediate_font);
            RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));
            immediate_text(command_list, 
                          &render_state->vertex_buffer, 
                          asset_manager, 
                          &basic_font, 
                          STR("What's the deal?"), 
                          vec3(-300, 150, 0.0f), 
                          vec4(1.0f, 1.0f, 1.0f, 1.0f), 
                          0.0f, 
                          32);

            RHI_cmd_update_buffer_contents(command_list, &render_state->vertex_buffer);
            RHI_cmd_draw_indexed(command_list, (render_state->vertex_buffer.vertex_count * 0.25f) * 6, 0, 0, 1, 0);

            RHI_vertex_buffer_reset_count(&render_state->vertex_buffer);

            // NOTE(Sleepster): Draw UI 
            if((game_state->open_debug_menu || game_state->editor_opened) && main_ui->frame_begun)
            {
                ui_state_end_frame(main_ui, command_list);
            }
            RHI_cmd_renderpass_end(command_list);
        }
        RHI_cmd_present(command_list, &render_state->fullscreen_color_buffer);

        RHI_execute_backend_commands(render_state->RHI_context);
        RHI_vertex_buffer_reset_offsets(render_state->RHI_context, &render_state->vertex_buffer);

        RHI_vertex_buffer_reset_count(&render_state->vertex_buffer);
        RHI_vertex_buffer_reset_offsets(render_state->RHI_context, &render_state->index_buffer);

        s_asset_manager_update(asset_manager);
        c_global_context_reset_temp_arena();
        c_arena_reset(&game_state->entity_manager->transient_storage);

#ifndef RELEASE
            file_data_t file_data = c_file_get_file_system_info(gc->game_dll_path);
            if(file_data.last_modtime != gc->game_dll_data.last_modtime)
            {
                gc->game_dll_data = file_data;
                gc->should_reload = true;
            }

            if(gc->should_reload)
            {
                return(-1);
            }
#endif

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

    return(0);
}
