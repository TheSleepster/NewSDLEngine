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
#include <r_immediate_rendering.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>
#include <s_ui_core.h>

#define MAX_ENTITIES (100000)

void process_window_events(RHI_context_t *RHI_context, input_manager_t *input_manager);

/*===========================================
  =============== ANIMATION2D ===============
  =========================================== */

#if 0
            // init
            asset_handle_t player_idle    = s_asset_manager_get_animation2D(asset_manager, "player_idle");
            asset_handle_t player_running = s_asset_manager_get_animation2D(asset_manager, "player_running");
            asset_handle_t player_falling = s_asset_manager_get_animation2D(asset_manager, "player_falling");

            asset_handle_t player_animation_data = s_asset_manager_get_animation2D_info(asset_manager, "player_animation_data");

            enum entity_animation_states 
            {
                PLAYER_IDLE;
            };

            entity_animation_map_insert(&player.animation_map, PLAYER_IDLE, player_idle);
            entity_animation_map_insert(&player.animation_map, PLAYER_RUNNING, player_running);
            entity_animation_map_insert(&player.animation_map, PLAYER_FALLING, player_falling);

            animation2D_t *playing_animation = entity_animation_map_get(&player.animation_map, player.animation_state);
            // play the animation...
#endif

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

enum entity_type 
{
    ET_Invalid,
    ET_Player,
    ET_DebugCollider,
    ET_Count
};

enum entity_flags
{
    EF_Valid     = 1ul << 0,
    EF_Alive     = 1ul << 1,
    EF_Gravitic  = 1ul << 2,
    EF_Actor     = 1ul << 3,
    EF_Static    = 1ul << 4,
    EF_IsGround  = 1ul << 5,
    EF_HasSprite = 1ul << 6,
};

#if 0
typedef entity_t u64

struct transform
{
    vec2_t position;
    vec2_t previous_position;
};

struct physics 
{
    vec2_t velocity;
};

struct sprite
{
    asset_handle_t texture;
    vec2_t         render_size;
    float32        rotation;
};

struct animated_sprite
{
    animation2D_t animation_data;
};

// '->' denotes "requires" or "implies"
// sprite -> transform
// animated_sprite -> sprite
// physics -> transform
#endif

enum player_animation_state_t
{
    PLAYER_ANIMATION_STATE_INVALID,
    PLAYER_ANIMATION_STATE_IDLE,
    PLAYER_ANIMATION_STATE_RUNNING,
    PLAYER_ANIMATION_STATE_JUMPING,
    PLAYER_ANIMATION_STATE_FALLING,
    PLAYER_ANIMATION_STATE_COUNT
};

// NOTE(Sleepster): owner_client_id is used to assign ownership of an entity 
// to that of a specific client 
struct entity_t
{
    u32             type;
    u32             flags;
    vec2_t          last_position;
    vec2_t          render_position;
    vec2_t          position;
    vec2_t          size;
    vec2_t          velocity;

    float32         rotation;
    s32             direction_x;
    asset_handle_t  sprite;

    animation2D_t  *animations;
    u32             animation_count;
    u32             animation_state;

    rectangle2_t    bounding_box;
};

struct entity_manager_t
{
    entity_t entities[MAX_ENTITIES];
    u32      active_entities;
};

struct renderer_t
{
};

struct game_state_t
{
    input_controller_t *controller;
    entity_manager_t   *entity_manager;

    RHI_image_t         game_color_buffer;
    RHI_image_t         game_depth_buffer;

    RHI_image_t         fullscreen_color_buffer;
    RHI_image_t         fullscreen_depth_buffer;

    RHI_vertex_buffer_t vertex_buffer;
    RHI_index_buffer_t  index_buffer;

    u32                 game_renderpass_ID;
    u32                 fullscreen_renderpass_ID; 

    bool32              open_debug_menu;
};

// TODO(Sleepster): DEBUG CODE 
global_variable string_t global_test_textbox_string = {}; 
// TODO(Sleepster): DEBUG CODE 

// NOTE(Sleepster): DEBUG CODE
internal_api void
handle_debug_ui_menu(ui_state_t *main_ui, RHI_context_t *RHI_context, asset_handle_t *player_sprite)
{
    // NOTE(Sleepster): DEBUG UI 
    ui_state_begin_frame(main_ui);
    ui_signal_t main_panel = ui_widget_draggable_panel(main_ui, 
                                                       STR("Test panel..."), 
                                                       vec2(20, 20), 
                                                       vec2(20, 20), 
                                                       vec2(10.0f, 10.0f), 
                                                       vec4(10, 10, 10, 10), 
                                                       vec4(0.4, 0.4, 0.4, 0.5),
                                                       0);
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

            ui_signal_t textbox = ui_widget_textbox(main_ui, STR("Information Box"), &global_test_textbox_string, {400, 20});
            if(textbox.widget->state->toggled)
            {
                main_ui->input_focused = true;
            }
            else
            {
                main_ui->input_focused = false;
            }

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

        ui_state_maybe_eat_inputs(main_ui);
    }
}

internal_api entity_t*
entity_create(game_state_t *game_state)
{
    Assert(game_state->entity_manager);

    entity_t *result = null;
    entity_t *found  = game_state->entity_manager->entities + game_state->entity_manager->active_entities;
    if((found->flags & EF_Valid) == 0)
    {
        result = found;
        result->flags = EF_Valid;

        ++game_state->entity_manager->active_entities;
    }

    Assert(result);
    return(result);
}

internal_api entity_t*
entity_player_create(game_state_t *game_state, asset_manager_t *asset_manager)
{
    entity_t *result = null;
    result = entity_create(game_state);
    
    asset_handle_t player_sprite = s_asset_manager_acquire_asset_handle(asset_manager, STR("player_sprite_sheet"));

    result->sprite = player_sprite;
    result->type   = ET_Player;
    result->flags |= EF_HasSprite;
    result->direction_x = 1;

    result->position = vec2(0,  40);
    result->size     = vec2(15, 18);
    result->animation_state = PLAYER_ANIMATION_STATE_RUNNING;

    result->animations      = c_arena_push_array(&gc->context_arena, animation2D_t, PLAYER_ANIMATION_STATE_COUNT);
    result->animation_count = PLAYER_ANIMATION_STATE_COUNT;

    result->bounding_box = rect2_create(result->position, result->size);

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
entity_test_collider_create(game_state_t *game_state, vec2_t position, vec2_t size)
{
    entity_t *result = entity_create(game_state);
    result->type     = ET_DebugCollider;
    result->position = position;
    result->size     = size;

    result->bounding_box = rect2_create(position, size);

    return(result);
}

internal_api void
render_collider(game_state_t *game_state, RHI_command_list_t *command_list, entity_t *entity)
{
    immediate_rect(command_list, 
                  &game_state->vertex_buffer,
                   vec2_expand_vec3(entity->bounding_box.min, 0.6f),
                   vec2_multiply(entity->bounding_box.half_size, vec2(2, 2)),
                   vec4(0.5f, 0.0, 0.0, 0.2f),
                   vec2_zero(),
                   vec2_zero(),
                   vec2_zero(),
                   vec2_zero(),
                   vec2_zero());
}

internal_api void
entity_render(game_state_t *game_state, RHI_command_list_t *command_list, entity_t *entity)
{
    texture2D_t *texture = null;
    vec2_t       uv_min  = vec2_zero();
    vec2_t       uv_max  = vec2_zero();

    if(entity->flags & EF_HasSprite)
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
                     &game_state->vertex_buffer, 
                      vec2_expand_vec3(entity->position, 0.8f), 
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
    entity_t *top_wall    = entity_test_collider_create(game_state, vec2(-160,  80), vec2(320, 20));
    entity_t *bottom_wall = entity_test_collider_create(game_state, vec2(-160, -90), vec2(320, 20));
    entity_t *left_wall   = entity_test_collider_create(game_state, vec2(-160, -80), vec2(20,  180));
    entity_t *right_wall  = entity_test_collider_create(game_state, vec2( 140, -80), vec2(20,  180));

    (void)top_wall;
    (void)bottom_wall;
    (void)left_wall;
    (void)right_wall;
}

int
game_main(void)
{
    game_state_t game_state = {};
    srand(rdtsc());

    input_manager_t  *input_manager = gc->input_manager;
    RHI_context_t    *RHI_context   = gc->RHI_context;
    asset_manager_t  *asset_manager = gc->asset_manager;
    ui_state_t       *main_ui       = Alloc(ui_state_t);

    game_state.controller = s_im_get_primary_controller(gc->input_manager);
    game_state.entity_manager = c_arena_push_struct(&gc->context_arena, entity_manager_t);

    // NOTE(Sleepster): Clear colors 
    RHI_clear_value_t color_buffer_clear_value = {
        .float_color = {0.1f, 0.1f, 0.8f, 1.0f},
    };

    RHI_clear_value_t depth_buffer_clear_value = {
        .depth   = 1.0f,
        .stencil = 0
    };

    // NOTE(Sleepster): Game Renderpass
    RHI_renderpass_desc_t game_renderpass_desc;
    game_state.game_color_buffer = {};
    game_state.game_depth_buffer = {};
    {
        RHI_image_create_info_t primary_game_color_buffer_create_info = {
            .width  = 320,
            .height = 180,
            .format = BMF_RGBA32_UNORM,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT
        };

        RHI_image_create_info_t primary_game_depth_buffer_create_info = {
            .width  = 320,
            .height = 180,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT 
        };

        game_state.game_color_buffer = RHI_image_create(RHI_context, &primary_game_color_buffer_create_info);
        game_state.game_depth_buffer = RHI_image_create(RHI_context, &primary_game_depth_buffer_create_info);

        game_renderpass_desc = {
            .render_width           = 320,
            .render_height          = 180,
            .resize_with_window     = false,
            .color_attachment_count = 1,
            .color_attachments = {
                [0] = {
                    .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                    .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                    .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                    .image           = &game_state.game_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                .image           = &game_state.game_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    // NOTE(Sleepster): Fullscreen Renderpass
    RHI_renderpass_desc_t fullscreen_renderpass_desc;
    game_state.fullscreen_color_buffer = {};
    game_state.fullscreen_depth_buffer = {};
    {
        RHI_image_create_info_t fullscreen_color_buffer_create_info = {
            .width  = (u32)RHI_context->window_size.x,
            .height = (u32)RHI_context->window_size.y,
            .format = BMF_RGBA32_UNORM,
            .usage  = (RHI_image_usage_t)(RHI_IMAGE_USAGE_RENDERPASS_COLOR_ATTACHMENT|RHI_IMAGE_USAGE_BLIT_SOURCE),
        };

        RHI_image_create_info_t fullscreen_depth_buffer_create_info = {
            .width  = (u32)RHI_context->window_size.x,
            .height = (u32)RHI_context->window_size.y,
            .format = BMF_D32_SFLOAT_S8_UINT,
            .usage  = RHI_IMAGE_USAGE_RENDERPASS_DEPTH_ATTACHMENT,
        };

        game_state.fullscreen_color_buffer = RHI_image_create(RHI_context, &fullscreen_color_buffer_create_info);
        game_state.fullscreen_depth_buffer = RHI_image_create(RHI_context, &fullscreen_depth_buffer_create_info);

        fullscreen_renderpass_desc = {
            .render_width           = (u32)RHI_context->window_size.x,
            .render_height          = (u32)RHI_context->window_size.y,
            .resize_with_window     = true,
            .color_attachment_count = 1,
            .color_attachments = {
                [0] = {
                    .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                    .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_LOAD,
                    .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                    .image           = &game_state.fullscreen_color_buffer,
                    .clear_value     =  color_buffer_clear_value
                },
            },
            .depth_stencil_attachment = {
                .access          = RHI_RENDERPASS_ATTACHMENT_ACCESS_WRITE,
                .load_operation  = RHI_RENDERPASS_ATTACHMENT_LOAD_OPERATION_CLEAR,
                .store_operation = RHI_RENDERPASS_ATTACHMENT_STORE_OPERATION_STORE,

                .image           = &game_state.fullscreen_depth_buffer,
                .clear_value     =  depth_buffer_clear_value
            },
        };
    }

    game_state.game_renderpass_ID       = RHI_build_renderpass(RHI_context, &game_renderpass_desc);
    game_state.fullscreen_renderpass_ID = RHI_build_renderpass(RHI_context, &fullscreen_renderpass_desc);

    ui_state_init(main_ui, input_manager, asset_manager, RHI_context, game_state.fullscreen_renderpass_ID);

    u32 *indices = c_arena_push_array(&RHI_context->transient_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
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
    immediate_vertex_t *vertices = c_arena_push_array(&RHI_context->RHI_arena, immediate_vertex_t, VERTEX_BUFFER_SIZE);
    game_state.vertex_buffer = RHI_vertex_buffer_create(RHI_context, 
                                                        RHI_RENDER_BUFFER_ALLOCATION_TYPE_MAPPED, 
                                                        RHI_RENDER_BUFFER_ADVANCE_RATE_PER_ELEMENT, 
                                                        (byte*)vertices, 
                                                        sizeof(immediate_vertex_t), 
                                                        VERTEX_BUFFER_SIZE);
    game_state.index_buffer  = RHI_index_buffer_create(RHI_context,  
                                                       RHI_RENDER_BUFFER_ALLOCATION_TYPE_GPU_ONLY, 
                                                       sizeof(u32),
                                                       indices, 
                                                       (sizeof(u32) * (6 * MAX_ENTITIES)));

    RHI_uniform_constant_buffer_t *camera_matrices_buffer = RHI_get_constant_buffer(RHI_context, STR("CameraMatrices"));

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

    global_test_textbox_string.data  = c_arena_push_array(&gc->context_arena, byte, 256);
    global_test_textbox_string.count = 0;

    // GAME INIT
    entity_t *player = entity_player_create(&game_state, asset_manager);
    create_test_environment(&game_state, asset_manager);
    // GAME INIT


    u64 perf_count_freq = SDL_GetPerformanceFrequency();
    u64 last_tsc        = SDL_GetPerformanceCounter();
    u64 current_tsc     = 0;
    u64 delta_tsc       = 0;

    float32 delta_time    = 0;
    float64 dt_accumulator = 0.0f;
    //float32 delta_time_ms = 0;
    while(gc->running)
    {
        s_im_reset_controller_states(input_manager);
        process_window_events(gc->RHI_context, input_manager);
        c_file_watcher_process_changes(&gc->file_watcher);

        if(game_state.open_debug_menu)
        {
            handle_debug_ui_menu(main_ui, RHI_context, &player_sprite);
        }

        vec2_t input_axis = {};
        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_W))
        {
            input_axis.y += 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_A))
        {
            input_axis.x -= 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_S))
        {
            input_axis.y -= 1.0f;
        }

        if(s_im_is_keyboard_key_down(game_state.controller, SDL_SCANCODE_D))
        {
            input_axis.x += 1.0f;
        }

        // NOTE(Sleepster): Debug menu 
        if(s_im_is_keyboard_key_pressed(game_state.controller, SDL_SCANCODE_SEMICOLON))
        {
            game_state.open_debug_menu = !game_state.open_debug_menu;
        }

        // NOTE(Sleepster): Simulate loop 
        const float64 TICK_RATE = 1.0 / 60.0;
        const float64 TICK_RATE_MS = TICK_RATE * 1000;
        if(delta_time >= (TICK_RATE * 2.0f))
        {
            delta_time = TICK_RATE * 2.0f;
        }

        dt_accumulator += delta_time;
        while(dt_accumulator >= TICK_RATE)
        {
            input_axis = vec2_normalize(input_axis);
            player->velocity = vec2_scale(vec2_scale(input_axis, 100), TICK_RATE);

            // NOTE(Sleepster): Collision detection 
            for(u32 entity_index = 0;
                entity_index < game_state.entity_manager->active_entities;
                ++entity_index)
            {
                entity_t *entity = game_state.entity_manager->entities + entity_index;
                if(entity->type == ET_Player)
                {
                    vec2_t current_velocity = player->velocity;
                    for(u32 test_index = 0;
                        test_index < game_state.entity_manager->active_entities;
                        ++test_index)
                    {
                        entity_t *test_entity = game_state.entity_manager->entities + test_index;
                        if(test_entity != entity)
                        {
                            // NOTE(Sleepster): Sweep Response
                            {
                                raytest_t sweep = rect2_sweep_test(entity->bounding_box, current_velocity, test_entity->bounding_box);
                                if(sweep.hit)
                                {
                                    current_velocity = vec2_scale(current_velocity, sweep.time);

                                    if(sweep.normal.x != 0.0f) entity->velocity.x = 0.0f;
                                    if(sweep.normal.y != 0.0f) entity->velocity.y = 0.0f;
                                }
                            }

                            // NOTE(Sleepster): Stationary Response
                            {
                                rectangle2_t predicted_hitbox = entity->bounding_box;
                                rect2_shift_by(&predicted_hitbox, current_velocity);

                                // TODO(Sleepster): Epsilon 
                                rectangle2_t minkowski = rect2_minkowski_difference(predicted_hitbox, test_entity->bounding_box);
                                if(minkowski.min.x <= 0 && minkowski.max.x >= 0 && 
                                   minkowski.min.y <= 0 && minkowski.max.y >= 0)
                                {
                                    vec2_t overlap_vector = rect2_get_vector_depth(minkowski);
                                    current_velocity      = vec2_add(current_velocity, overlap_vector);

                                    if(overlap_vector.x != 0) entity->velocity.x = 0.0f;
                                    if(overlap_vector.y != 0) entity->velocity.y = 0.0f;
                                }
                            }

                            entity->velocity = current_velocity;
                        }
                    }
                }
            }

            player->last_position = player->position;
            player->position = vec2_add(player->position, player->velocity);

            rect2_shift_by(&player->bounding_box, vec2_subtract(player->position, player->last_position));
            if(input_axis.x > 0.0)
            {
                player->direction_x = 1;
                player->animation_state = PLAYER_ANIMATION_STATE_RUNNING;
            }
            else if(input_axis.x < 0.0)
            {
                player->direction_x = -1;
                player->animation_state = PLAYER_ANIMATION_STATE_RUNNING;
            }
            else if(input_axis.x == 0)
            {
                player->animation_state = PLAYER_ANIMATION_STATE_IDLE;
            }

            animation2D_t *animation = &player->animations[player->animation_state];
            if(c_duration_counter_advance(&animation->frame_timer, TICK_RATE_MS))
            {
                animation->current_frame = (animation->current_frame + 1) % animation->animation_info.frame_count;
            }

            dt_accumulator -= TICK_RATE;
        }
        float32 alpha = (float32)(dt_accumulator / TICK_RATE);
        // NOTE(Sleepster): Simulate loop 

        // NOTE(Sleepster): Game renderpass
        RHI_command_list_t *command_list = RHI_get_command_list(RHI_context, RHI_RENDER_COMMAND_LIST_TYPE_GRAPHICS);
        {
            // NOTE(Sleepster): Draw entities 
            {
                for(u32 entity_index = 0;
                    entity_index < game_state.entity_manager->active_entities;
                    ++entity_index)
                {
                    entity_t *entity = game_state.entity_manager->entities + entity_index;
                    Assert(entity->flags & EF_Valid);

                    entity->render_position = vec2_lerp(entity->last_position, entity->position, alpha);
                    entity_render(&game_state, command_list, entity);
                }

                RHI_cmd_update_buffer_contents(command_list, &game_state.vertex_buffer);
                RHI_cmd_renderpass_begin(command_list, game_state.game_renderpass_ID);

                RHI_cmd_bind_vertex_buffer(command_list, &game_state.vertex_buffer);
                RHI_cmd_bind_index_buffer(command_list,  &game_state.index_buffer);
                RHI_cmd_use_shader_program(command_list,  immediate_textured);

                s32 window_width  = Max(game_renderpass_desc.render_width, 10);
                s32 window_height = Max(game_renderpass_desc.render_height, 10);

                camera_matrices_t camera_matrix_buffer_data = {
                    .view_matrix       = mat4_identity(),
                    .projection_matrix = mat4_RHDX_ortho(window_width * -0.5f, window_width * 0.5, window_height * -0.5, window_height * 0.5, -1, 1)
                };
                RHI_cmd_update_constant_buffer(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

                RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
                RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

                RHI_cmd_draw_indexed(command_list, (game_state.vertex_buffer.vertex_count * 0.25f) * 6, 0, 1, 0);

                game_state.vertex_buffer.vertex_count = 0;
            }

            // NOTE(Sleepster): Draw misc 
            {
                for(u32 entity_index = 0;
                    entity_index < game_state.entity_manager->active_entities;
                    ++entity_index)
                {
                    entity_t *entity = game_state.entity_manager->entities + entity_index;
                    render_collider(&game_state, command_list, entity);
                }

                RHI_cmd_update_buffer_contents(command_list, &game_state.vertex_buffer);

                RHI_cmd_bind_vertex_buffer(command_list, &game_state.vertex_buffer);
                RHI_cmd_bind_index_buffer(command_list,  &game_state.index_buffer);
                RHI_cmd_use_shader_program(command_list,  immediate_rectangle);

                RHI_pipeline_state_t collider_blending = {};
                collider_blending.blend_enabled = false;
                collider_blending.src_alpha_blend_mode = RBM_SrcAlpha;
                collider_blending.dst_alpha_blend_mode = RBM_OneMinusSrcAlpha;
                RHI_cmd_set_render_state(command_list, &collider_blending);

                s32 window_width  = Max(game_renderpass_desc.render_width, 10);
                s32 window_height = Max(game_renderpass_desc.render_height, 10);

                RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
                RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));

                RHI_cmd_draw_indexed(command_list, (game_state.vertex_buffer.vertex_count * 0.25f) * 6, 0, 1, 0);
                RHI_cmd_renderpass_end(command_list);

                game_state.vertex_buffer.vertex_count = 0;
            }
        }

        // NOTE(Sleepster): Fullscreen Renderpass 
        {
            RHI_cmd_blit_renderpass(command_list, game_state.game_renderpass_ID, game_state.fullscreen_renderpass_ID);
            RHI_cmd_renderpass_begin(command_list, game_state.fullscreen_renderpass_ID);

            RHI_cmd_bind_vertex_buffer(command_list, &game_state.vertex_buffer);
            RHI_cmd_bind_index_buffer(command_list,  &game_state.index_buffer);

            s32 window_width  = Max(RHI_context->window_size.x, 10);
            s32 window_height = Max(RHI_context->window_size.y, 10);

            s32 half_window_width  = window_width  * 0.5f;
            s32 half_window_height = window_height * 0.5f;

            camera_matrices_t camera_matrix_buffer_data = {
                .view_matrix       = mat4_identity(),
                .projection_matrix = mat4_RHDX_ortho(-half_window_width, half_window_width, -half_window_height, half_window_height, -1, 1)
            };
            RHI_cmd_update_constant_buffer(command_list, camera_matrices_buffer, &camera_matrix_buffer_data, sizeof(camera_matrix_buffer_data));

            RHI_pipeline_state_t font_state = {};
            font_state.blend_enabled = false;
            font_state.src_alpha_blend_mode = RBM_SrcAlpha;
            font_state.dst_alpha_blend_mode = RBM_OneMinusSrcAlpha;
            RHI_cmd_set_render_state(command_list, &font_state);

            RHI_cmd_use_shader_program(command_list, immediate_font);
            RHI_cmd_set_viewport(command_list, vec2(0, window_height), vec2(window_width, -window_height));
            RHI_cmd_set_scissor(command_list,  vec2(0, 0),             vec2(window_width,  window_height));
            immediate_text(command_list, 
                          &game_state.vertex_buffer, 
                          asset_manager, 
                          &basic_font, 
                          STR("This is a test string..."), 
                          vec3(-300, 150, 0.0f), 
                          vec4(1.0f, 1.0f, 1.0f, 1.0f), 
                          0.0f, 
                          32);

            RHI_cmd_update_buffer_contents(command_list, &game_state.vertex_buffer);
            RHI_cmd_draw_indexed(command_list, (game_state.vertex_buffer.vertex_count * 0.25f) * 6, 0, 1, 0);

            // NOTE(Sleepster): Draw UI 
            if(game_state.open_debug_menu && main_ui->frame_begun)
            {
                ui_state_end_frame(main_ui, command_list);
            }
            RHI_cmd_renderpass_end(command_list);
        }
        RHI_cmd_present(command_list, &game_state.fullscreen_color_buffer);

        RHI_execute_backend_commands(RHI_context);
        RHI_buffer_reset(RHI_context, &game_state.vertex_buffer);
        RHI_buffer_reset(RHI_context, &game_state.index_buffer);

        s_asset_manager_update(asset_manager);
        c_global_context_reset_temporary_data();

        current_tsc = SDL_GetPerformanceCounter();
        delta_tsc   = current_tsc - last_tsc;
        last_tsc    = current_tsc;

        delta_time = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);
    }

    return(0);
}
