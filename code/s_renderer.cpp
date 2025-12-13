/* ========================================================================
   $File: s_renderer.cpp $
   $Date: December 07 2025 01:09 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_renderer.h>
#include <s_asset_manager.h>

#define SOKOL_IMPL
#define SOKOL_DEBUG
#define SOKOL_GLCORE
#include <sokol/sokol_gfx.h>
#include <sokol/sokol_log.h>

#include <r_asset_texture.h>
#include <r_render_group.h>
#include <shaders/test.glsl.h>

// TODO(Sleepster): sg_compare_func
void
r_texture_upload(texture2D_t *texture, bool8 is_mutable, bool8 has_AA, filter_type_t filter_type)
{
    Assert(texture->bitmap.decompressed_data.data != null);
    Assert(texture->bitmap.decompressed_data.count > 0);

    sg_filter filtering = (filter_type == TAAFT_LINEAR) ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
    texture->view.filter_type = filtering;

    sg_range image_data = {
        .ptr  = texture->bitmap.decompressed_data.data,
        .size = texture->bitmap.decompressed_data.count,
    };

    sg_range empty_data = {};
    sg_image_desc image_desc = {
        .usage = {
            .immutable     = (bool)!is_mutable,
            .stream_update = (bool)is_mutable
        },
        .width        = texture->bitmap.width,
        .height       = texture->bitmap.height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.mip_levels[0] = is_mutable ? empty_data : image_data,
    };
    sg_init_image(texture->image, &image_desc);

    sg_view_desc image_view_desc = {
        .texture = {
            .image = texture->image,
        },
    };
    sg_init_view(texture->view.view, image_view_desc);

    texture->is_uploaded = true;
}

void
r_reset_pipeline_render_state(render_state_t *render_state)
{
    render_state->draw_frame.pipeline_state = {
        .depth_test       = true,
        .depth_writing    = true,
        .depth_func       = SG_COMPAREFUNC_GREATER,

        .blending         = false,
        .src_rgb_factor   = SG_BLENDFACTOR_ONE,
        .src_alpha_factor = SG_BLENDFACTOR_ONE,
        .dst_rgb_factor   = SG_BLENDFACTOR_ZERO,
        .dst_alpha_factor = SG_BLENDFACTOR_ZERO,

        .color_blend_op   = SG_BLENDOP_ADD,
        .alpha_blend_op   = SG_BLENDOP_ADD,

        .active_shader    =  &render_state->default_shader,
    };

    render_state->draw_frame.active_render_layer = 16;
    render_state->draw_frame.active_camera       = &render_state->default_camera;
    render_state->draw_frame.active_shader       = &render_state->default_shader;
}

render_pipeline_data_t*
r_get_or_create_pipeline(render_state_t          *render_state, 
                         render_shader_t         *shader, 
                         render_pipeline_state_t  pipeline_state)
{
    render_pipeline_data_t *result   = null;
    u64 state_hash_value = c_fnv_hash_value((byte*)&pipeline_state, sizeof(render_pipeline_state_t), default_fnv_hash_value);

    bool8 found = false;
    if(render_state->pipelines)
    {
        c_dynarray_for(render_state->pipelines, pipeline_index)
        {
            render_pipeline_data_t *pipeline_data = render_state->pipelines + pipeline_index;
            u64 this_pipeline_hash = c_fnv_hash_value((byte*)&pipeline_data->p_state, sizeof(render_pipeline_state_t), default_fnv_hash_value);
            if(this_pipeline_hash == state_hash_value)
            {
                result = pipeline_data;
                found  = true;
                break;
            }
        }
    } 

    if(!found)
    {
        render_pipeline_data_t new_pipeline_data = {};
        new_pipeline_data.p_state = pipeline_state;
        new_pipeline_data.desc    = (sg_pipeline_desc){
            .shader = shader->ID,
            .layout = {
                .buffers[0] = {
                    .stride    = sizeof(vertex_t),
                    .step_func = SG_VERTEXSTEP_PER_VERTEX
                },
                .attrs = {
                    [0] = {
                        .buffer_index = 0,
                        .offset       = offsetof(vertex_t, v_position),
                        .format       = SG_VERTEXFORMAT_FLOAT4
                    },
                    [1] = {
                .buffer_index  = 0,
                        .offset        = offsetof(vertex_t, v_color),
                        .format        = SG_VERTEXFORMAT_FLOAT4
                    },
                    [2] = {
                        .buffer_index = 0,
                        .offset       = offsetof(vertex_t, v_texcoords),
                        .format       = SG_VERTEXFORMAT_FLOAT2
                    },
                },
            },
            .depth  = {
                .compare       = (sg_compare_func)pipeline_state.depth_func,
                .write_enabled = (bool)pipeline_state.depth_writing,
            },
            .colors[0] = {
                .blend = {
                    .enabled = (bool)pipeline_state.blending,
                    .src_factor_rgb   = (sg_blend_factor)pipeline_state.src_rgb_factor,
                    .dst_factor_rgb   = (sg_blend_factor)pipeline_state.dst_rgb_factor,
                    .op_rgb           = (sg_blend_op)    pipeline_state.color_blend_op,
                    .src_factor_alpha = (sg_blend_factor)pipeline_state.src_alpha_factor,
                    .dst_factor_alpha = (sg_blend_factor)pipeline_state.dst_alpha_factor,
                    .op_alpha         = (sg_blend_op)    pipeline_state.alpha_blend_op,
                },
            },
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .index_type     = SG_INDEXTYPE_UINT32,
            .cull_mode      = SG_CULLMODE_BACK,
        };
        new_pipeline_data.pipeline = sg_make_pipeline(&new_pipeline_data.desc);

        c_dynarray_push(render_state->pipelines, new_pipeline_data);
        result = render_state->pipelines + render_state->created_pipeline_count++;
    }

    return(result);
}

void
s_init_renderer(render_state_t *render_state, asset_manager_t *asset_manager, game_state_t *state)
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
        
        r_reset_pipeline_render_state(render_state);
    }

    // NOTE(Sleepster): Default matrices 
    u32 window_width  = 1920;
    u32 window_height = 1080;
    mat4_t view_matrix       = mat4_identity();
    mat4_t projection_matrix = mat4_RHGL_ortho(window_width  *  0.5f, 
                                               window_width  *  0.5f,
                                               window_height * -0.5f, 
                                               window_height *  0.5f,
                                              -1.0f, 
                                               1.0f);

    render_state->default_camera = {
        .view_matrix            = view_matrix,
        .projection_matrix      = projection_matrix,
        .view_projection_matrix = mat4_multiply(view_matrix, projection_matrix)
    };

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
                .size = sizeof(vertex_t) * MAX_VERTICES,
                .usage = {
                    .vertex_buffer  = true,
                    .immutable      = false,
                    .dynamic_update = true,
                },
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
                .usage = {
                    .index_buffer = true,
                    .immutable    = true,
                },
                .data = {
                    .ptr  = index_buffer,
                    .size = sizeof(u32) * MAX_INDICES
                }
            };
            render_state->bindings.index_buffer = sg_make_buffer(&index_buffer_desc);
            free(index_buffer);
        }

        // NOTE(Sleepster): Nearest and Linear samplers 
        {
            sg_sampler_desc nearest_desc = {
                .min_filter = SG_FILTER_NEAREST,
                .mag_filter = SG_FILTER_NEAREST,
                .wrap_u     = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v     = SG_WRAP_CLAMP_TO_EDGE,
            };

            sg_sampler_desc linear_desc = {
                .min_filter = SG_FILTER_LINEAR,
                .mag_filter = SG_FILTER_LINEAR,
                .wrap_u     = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v     = SG_WRAP_CLAMP_TO_EDGE,
            };

            render_state->nearest_filter_sampler = sg_make_sampler(&nearest_desc);
            render_state->linear_filter_sampler  = sg_make_sampler(&linear_desc);
        }

        /*
         * - shaders
         * - depth compare func
         * - depth writing (true or false)
         * - color blending (true or false)
         * - src rgb blend factor
         * - src alpha blend factor
         * - dst rgb blend factor
         * - dst alpha blend factor
         */

        render_state->default_shader      = {};
        render_state->default_shader.ID   = sg_make_shader(test_shader_desc(sg_query_backend()));
        render_state->default_shader.desc = test_shader_desc(sg_query_backend());
        render_state->pipeline            = r_get_or_create_pipeline(render_state, &render_state->default_shader, render_state->draw_frame.pipeline_state)->pipeline;

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


    render_state->pipelines = c_dynarray_create(render_pipeline_data_t);
    render_state->pipelines = c_dynarray_reserve(render_state->pipelines, 50);

    render_state->renderer_arena   = c_arena_create(MB(100));
    render_state->draw_frame_arena = c_arena_create(MB(100));

    render_state->vertex_buffer = AllocArray(vertex_t, MAX_VERTICES);
    render_state->vertex_count  = 0;
    Assert(render_state->vertex_buffer);

    render_state->quad_buffer  = AllocArray(render_quad_t, MAX_QUADS);
    render_state->vertex_count = 0;
    Assert(render_state->quad_buffer);
}

void
r_test_render(render_state_t *render_state, asset_manager_t *asset_manager)
{
    asset_handle_t texture_handle = s_asset_texture_get(asset_manager, STR("outline"));
    asset_handle_t other_texture  = s_asset_texture_get(asset_manager, STR("player"));
    
    render_state->draw_frame.active_texture      = render_state->default_texture;
    render_state->draw_frame.active_render_layer = 10;
    r_renderpass_begin(render_state);
    r_draw_rect(render_state, vec2(0.0, 0.0), vec2(200, 200), {1, 1, 1, 1});
    r_renderpass_end(render_state);

    render_state->draw_frame.active_texture = texture_handle;
    r_renderpass_begin(render_state);
    r_draw_rect(render_state, vec2(-200.0, 0.0), vec2(200, 200), {1, 1, 1, 1});
    r_draw_rect(render_state, vec2( 0.0,   100.0), vec2(200, 200), {1, 1, 1, 1});
    r_renderpass_end(render_state);

    render_state->draw_frame.active_texture      = other_texture;
    render_state->draw_frame.active_render_layer = 9;
    render_state->draw_frame.pipeline_state.blending = true;
    r_renderpass_begin(render_state);
    r_draw_rect(render_state, vec2(100.0,  100.0), vec2(200, 200), {1, 1, 1, 1});
    r_renderpass_end(render_state);
    //r_create_draw_quad(render_state, vec2(0.0, 0.0), vec2(200, 200), vec4(1.0, 0.0f, 0.0f, 1.0f), texture_handle);
}

void
s_renderer_draw_frame(game_state_t *state, asset_manager_t *asset_manager, render_state_t *render_state)
{
    render_state->default_texture = s_asset_texture_get(asset_manager, STR("null_sprite"));
    asset_handle_t texture_handle = s_asset_texture_get(asset_manager, STR("outline"));
    s_atlas_packer_pack_textures(asset_manager, &asset_manager->texture_catalog.primary_packer);

    render_state->bindings.views[VIEW_uTexture]   = texture_handle.texture->view;
    render_state->bindings.samplers[SMP_uSampler] = render_state->nearest_filter_sampler;

    s32 window_width;
    s32 window_height;
    SDL_GetWindowSize(state->window, &window_width, &window_height);

    render_state->draw_frame.main_vertex_buffer = c_arena_push_array(&render_state->draw_frame_arena, 
                                                                      vertex_t, 
                                                                      render_state->draw_frame.total_vertex_count);
    Assert(render_state->draw_frame.main_vertex_buffer);

    r_test_render(render_state, asset_manager);
    r_renderpass_update_render_buffers(render_state);

    mat4_t view_matrix       = mat4_identity();
    mat4_t projection_matrix = mat4_RHGL_ortho(window_width  * -0.5f, 
                                               window_width  *  0.5f,
                                               window_height * -0.5f, 
                                               window_height *  0.5f,
                                               -1.0f, 
                                               1.0f); 
    render_state->default_camera = {
        .view_matrix            = view_matrix,
        .projection_matrix      = projection_matrix,
        .view_projection_matrix = mat4_multiply(view_matrix, projection_matrix)
    };

    sg_pass renderpass_desc = {
        .action = render_state->pass_action,
        .swapchain = {
            .width  = window_width,
            .height = window_height,
            .gl.framebuffer = 0,
        },
    };
    sg_begin_pass(&renderpass_desc);

    Assert(render_state->draw_frame.used_render_group_count > 0);
    Assert(render_state->draw_frame.total_vertex_count > 0);

    sg_range vertex_buffer_range = {
        .ptr  = render_state->draw_frame.main_vertex_buffer,
        .size = render_state->draw_frame.total_vertex_count * sizeof(vertex_t)
    };
    sg_update_buffer(render_state->bindings.vertex_buffers[0], vertex_buffer_range);

    for(u32 render_group_index = 0;
        render_group_index < render_state->draw_frame.used_render_group_count;
        ++render_group_index)
    {
        u32 ID = render_state->draw_frame.used_render_groups[render_group_index];
        render_group_t *render_group = render_state->render_groups + ID;

        r_render_group_to_output(render_state, render_group);
    }
    sg_end_pass();
    sg_commit();

    render_state->draw_frame.used_render_group_count = 0;
    render_state->draw_frame.total_vertex_count      = 0;
    c_arena_reset(&render_state->draw_frame_arena);
    ZeroMemory(render_state->draw_frame.used_render_groups,
               sizeof(u32) * render_state->draw_frame.used_render_group_count);
}
