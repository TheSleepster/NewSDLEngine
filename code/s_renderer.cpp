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

render_quad_t*
r_create_draw_quad(render_state_t *render_state, vec2_t position, vec2_t size, vec4_t color)
{
    render_quad_t *result = render_state->quad_buffer + render_state->quad_count++;

    result->position = position;
    result->size     = size;
    result->color    = color;

    float32 top    = position.y + size.y;
    float32 left   = position.x;
    float32 bottom = position.y;
    float32 right  = position.x + size.x;

    result->bottom_left.v_position  = vec4(left, bottom,  0.0f, 1.0f);
    result->top_left.v_position     = vec4(left, top,     0.0f, 1.0f);
    result->top_right.v_position    = vec4(right, top,    0.0f, 1.0f);
    result->bottom_right.v_position = vec4(right, bottom, 0.0f, 1.0f);

    for(u32 index = 0;
        index < 4;
        ++index)
    {
        result->vertices[index].v_color = color;
    }

    return(result);
}

void
r_texture_upload(texture2D_t *texture, bool8 has_AA, filter_type_t filter_type)
{
    sg_filter filtering;
    switch(filter_type)
    {
        case TAAFT_LINEAR:
        {
            filtering = SG_FILTER_LINEAR;
        }break;
        case TAAFT_NEAREST:
        {
            filtering = SG_FILTER_NEAREST;
        }break;
        default: {filtering = SG_FILTER_NEAREST; InvalidCodePath;}break;
    }

    if(has_AA)
    {
        log_warning("We do not support AA right now... Ignoring\n");
    }

    sg_sampler_desc sampler_desc = {
        .min_filter = filtering,
        .mag_filter = filtering,
        .wrap_u     = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v     = SG_WRAP_CLAMP_TO_EDGE
    };

    sg_image_desc image_desc = {
        .render_target = false,
        .width         = texture->bitmap.width,
        .height        = texture->bitmap.height,
        .pixel_format  = SG_PIXELFORMAT_RGBA8,
        .sample_count  = 1
    };
        
    texture->texture_data.image   = sg_make_image(&image_desc);
    texture->texture_data.sampler = sg_make_sampler(&sampler_desc);
}

void
s_init_renderer(render_state_t *render_state, game_state_t *state)
{
    // NOTE(Sleepster): OpenGL 4.3 Init 
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
    }

    // NOTE(Sleepster): Sokol Init 
    {
        // NOTE(Sleepster): Sokol "context" Setup 
        {
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
        }

        // NOTE(Sleepster): Vertex and Index buffers 
        {
            sg_buffer_desc vertex_buffer_desc = {
                .type  = SG_BUFFERTYPE_VERTEXBUFFER,
                .usage = SG_USAGE_STREAM,
                .data  = {
                    .ptr  = null,
                    .size = sizeof(vertex_t) * MAX_VERTICES 
                }
            };
            render_state->bindings.vertex_buffers[0] = sg_make_buffer(&vertex_buffer_desc);

            u32 *index_buffer = AllocArray(u32, MAX_INDICES);
            u32  index_offset = 0;
            for(u32 index = 0;
                index < MAX_INDICES;
                index += 6)
            {
                // 0 1 2 0 2 3
                index_buffer[index + 0] = index_offset + 0;
                index_buffer[index + 1] = index_offset + 1;
                index_buffer[index + 2] = index_offset + 2;
                index_buffer[index + 3] = index_offset + 0;
                index_buffer[index + 4] = index_offset + 2;
                index_buffer[index + 5] = index_offset + 3;

                index_offset += 4;
            }

            sg_buffer_desc index_buffer_desc = {
                .type  = SG_BUFFERTYPE_INDEXBUFFER,
                .usage = SG_USAGE_IMMUTABLE,
                .data = {
                    .ptr  = index_buffer,
                    .size = sizeof(u32) * MAX_INDICES
                }
            };
            render_state->bindings.index_buffer = sg_make_buffer(&index_buffer_desc);
            free(index_buffer);
        }

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
            .index_type = SG_INDEXTYPE_UINT32
        };

        render_state->pipeline = sg_make_pipeline(&pipeline_desc);

        render_state->pass_action = (sg_pass_action) {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR, 
                .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f }
            },
            .depth = {
                .load_action  = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_STORE,
                .clear_value  = 0.0f,
            }
        };
    }

    render_state->vertex_buffer = AllocArray(vertex_t, MAX_VERTICES);
    render_state->vertex_count  = 0;
    Assert(render_state->vertex_buffer);

    render_state->quad_buffer  = AllocArray(render_quad_t, MAX_QUADS);
    render_state->vertex_count = 0;
    Assert(render_state->quad_buffer);
}

void
r_test_render(render_state_t *render_state)
{
    r_create_draw_quad(render_state, vec2(0.0, 0.0), vec2(200, 200), vec4(1.0, 0.0f, 0.0f, 1.0f));
}

void
s_renderer_draw_frame(game_state_t *state, render_state_t *render_state)
{
    s32 window_width;
    s32 window_height;
    SDL_GetWindowSize(state->window, &window_width, &window_height);

    r_test_render(render_state);

    render_state->view_matrix       = mat4_identity();
    render_state->projection_matrix = mat4_RHGL_ortho(window_width  * -0.5f, 
                                                      window_width  *  0.5f,
                                                      window_height * -0.5f, 
                                                      window_height *  0.5f,
                                                      -1.0f, 
                                                      1.0f);
    
    for(u32 quad_index = 0;
        quad_index < render_state->quad_count;
        ++quad_index)
    {
        render_quad_t *quad = render_state->quad_buffer + quad_index;
        if(quad)
        {
            vertex_t *vertex_buffer_ptr = render_state->vertex_buffer + render_state->vertex_count; 

            vertex_t *bottom_left  = vertex_buffer_ptr + 0;
            vertex_t *top_left     = vertex_buffer_ptr + 1;
            vertex_t *top_right    = vertex_buffer_ptr + 2;
            vertex_t *bottom_right = vertex_buffer_ptr + 3;

            *bottom_left  = quad->bottom_left;
            *top_left     = quad->top_left;
            *top_right    = quad->top_right;
            *bottom_right = quad->bottom_right;

            render_state->vertex_count += 4;
        }
    }

    sg_range vertex_buffer_range = {
        .ptr  = render_state->vertex_buffer,
        .size = render_state->vertex_count * sizeof(vertex_t)
    };
    sg_update_buffer(render_state->bindings.vertex_buffers[0], vertex_buffer_range);

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
    sg_apply_viewport(0, 0, window_width, window_height, false);

    sg_apply_pipeline(render_state->pipeline);
    sg_apply_bindings(&render_state->bindings);

    VSParams_t matrices = {};
    memcpy(matrices.uProjectionMatrix, render_state->projection_matrix.values, sizeof(float32) * 16);
    memcpy(matrices.uViewMatrix,       render_state->view_matrix.values,       sizeof(float32) * 16);

    sg_range matrix_range = {
        .ptr  = &matrices,
        .size = sizeof(VSParams_t)
    };
    sg_apply_uniforms(UB_VSParams, matrix_range);

    sg_draw(0, 6, 1);
    sg_end_pass();

    sg_commit();
    render_state->quad_count   = 0;
    render_state->vertex_count = 0;
}
