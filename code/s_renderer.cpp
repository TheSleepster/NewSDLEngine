/* ========================================================================
   $File: s_renderer.cpp $
   $Date: December 07 2025 01:09 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_renderer.h>

#define SOKOL_IMPL
#define SOKOL_DEBUG
#define SOKOL_GLCORE
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_log.h>

#include <test.glsl.h>

void
s_init_renderer(render_state_t *render_state, game_state_t *state)
{

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    state->window = SDL_CreateWindow("Game", 
                                     (u32)state->window_size.x,
                                     (u32)state->window_size.y, 
                                     SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
    if(!state->window)
    {
        log_error("Failure to create SDL_Window... error: '%s'...\n", SDL_GetError());
        exit(0);
    }

    SDL_GLContext context = SDL_GL_CreateContext(state->window);
    if(!SDL_GL_MakeCurrent(state->window, context))
    {
        log_error("Failure to create SDL_Context... error: '%s'...\n", SDL_GetError());
        exit(0);
    }

    sg_desc sokol_desc = {
        .logger = {
            .func = slog_func
        },
        .environment = {
            .defaults = {
                .color_format = SG_PIXELFORMAT_RGBA8,
                .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
                .sample_count = 1
            },
        },
    };
    sg_setup(&sokol_desc);

    vertex_t vertices[] = {
        (vertex_t){ .v_position = {-1.0f, -1.0f, 0.0f, 1.0f}, .v_color = {1.0f, 0.0f, 0.0f, 1.0f} },
        (vertex_t){ .v_position = { 0.0f,  1.0f, 0.0f, 1.0f}, .v_color = {0.0f, 1.0f, 0.0f, 1.0f} },
        (vertex_t){ .v_position = { 1.0f, -1.0f, 0.0f, 1.0f}, .v_color = {0.0f, 0.0f, 1.0f, 1.0f} },
    };

    sg_buffer_desc buffer_desc = {
        .type  = SG_BUFFERTYPE_VERTEXBUFFER,
        .usage = SG_USAGE_IMMUTABLE,
        .data  = {
            .ptr  = vertices,
            .size = sizeof(vertices)
        },
    };
    render_state->bindings.vertex_buffers[0] = sg_make_buffer(&buffer_desc);

    sg_shader shader = sg_make_shader(test_shader_desc(sg_query_backend()));
    sg_pipeline_desc pipeline_desc = (sg_pipeline_desc){
        .shader = shader,
        .layout = {
            .attrs = {
                [ATTR_test_vPosition] = {
                    .offset = offsetof(vertex_t, v_position),
                    .format = SG_VERTEXFORMAT_FLOAT4
                },
                [ATTR_test_vColor] = {
                    .offset = offsetof(vertex_t, v_color),
                    .format = SG_VERTEXFORMAT_FLOAT4
                }
            }
        },
        .depth  = {
            .compare       = SG_COMPAREFUNC_GREATER,
            .write_enabled = true,
        },
        .colors[0] = {
            .blend = {
                .enabled = true,
            },
        },
    };

    render_state->pipeline = sg_make_pipeline(&pipeline_desc);

    render_state->pass_action = (sg_pass_action){
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR, 
            .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f }
        },
        .depth = {
            .load_action  = SG_LOADACTION_CLEAR,
            .store_action = SG_STOREACTION_STORE,
            .clear_value = 0.0f,
        }
    };
}

void
s_renderer_draw_frame(game_state_t *state, render_state_t *render_state)
{
    s32 window_width;
    s32 window_height;
    SDL_GetWindowSize(state->window, &window_width, &window_height);

    sg_pass pass_data = {
        .action     = render_state->pass_action,
        .swapchain  = {
            .width  = window_width,
            .height = window_height,
            .sample_count = 1,
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH_STENCIL,
            .gl.framebuffer = 0
        },
    };
    sg_begin_pass(&pass_data);

    sg_apply_pipeline(render_state->pipeline);
    sg_apply_bindings(&render_state->bindings);

    sg_draw(0, 3, 1);

    sg_end_pass();
    sg_commit();
}
