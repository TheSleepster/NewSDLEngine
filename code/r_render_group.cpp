/* ========================================================================
   $File: r_render_group.cpp $
   $Date: January 14 2026 05:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_memory_arena.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_math.h>
#include <s_asset_manager.h>

#include <r_vulkan_types.h>
#include <r_render_group.h>

//vulkan_shader_data_t    *shader         = &render_context->default_shader->slot->shader.shader_data;
//render_context->test_camera = r_render_camera_create(shader->camera_matrices.view_matrix, shader->camera_matrices.projection_matrix);

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(renderer_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

// NOTE(Sleepster): CAMERA ID STUFF
//
// NOTE(Sleepster): Takes slight variations in the floating point values of a matrix and "rounds" those values so that the float value can be deterministic for ID creation. 
// The ID MUST be deterministic since the camera's ID is checked constantly by the render_groups
__m128
c_canonicalize_floats_values(__m128 values, float32 epsilon)
{
    __m128 result;

    const __m128 zero = _mm_set1_ps(0.0f);
    const __m128 epsv = _mm_set1_ps(epsilon);
    const __m128 inv_eps = _mm_set1_ps(1.0f / epsilon);

    // NOTE(Sleepster): Clear the sign value and store it
    __m128 is_zero = _mm_cmpeq_ps(values, zero);
    values = _mm_andnot_ps(is_zero, values);

    // NOTE(Sleepster): check if it's nan 
    __m128 is_nan = _mm_cmpunord_ps(values, values);


    // NOTE(Sleepster): these basically are a "truncation" which is a hack for SSE2 rounding 
    result = _mm_mul_ps(values, inv_eps);
    result = _mm_cvtepi32_ps(_mm_cvttps_epi32(result));
    result = _mm_mul_ps(result, epsv);

    const __m128 canonical_nan = _mm_castsi128_ps(_mm_set1_epi32(0x7fc00000));
    result = _mm_or_ps(_mm_and_ps(is_nan, canonical_nan), _mm_andnot_ps(is_nan, result));

    return(result);
}

void
c_canonicalize_matrix_values(u32 *temp_buffer, float32 epsilon, mat4_t *matrix)
{
    // NOTE(Sleepster): Extract the matrix values 
    __m128 v0 = matrix->SSE[0];
    __m128 v1 = matrix->SSE[1];
    __m128 v2 = matrix->SSE[2];
    __m128 v3 = matrix->SSE[3];

    // NOTE(Sleepster): "Round them" within the range of the epsilon 
    v0 = c_canonicalize_floats_values(v0, epsilon);
    v1 = c_canonicalize_floats_values(v1, epsilon);
    v2 = c_canonicalize_floats_values(v2, epsilon);
    v3 = c_canonicalize_floats_values(v3, epsilon);

    // NOTE(Sleepster): Store them back, casting to __m128i's.
    _mm_storeu_si128((__m128i*)(temp_buffer + 0),  _mm_castps_si128(v0));
    _mm_storeu_si128((__m128i*)(temp_buffer + 4),  _mm_castps_si128(v1));
    _mm_storeu_si128((__m128i*)(temp_buffer + 8),  _mm_castps_si128(v2));
    _mm_storeu_si128((__m128i*)(temp_buffer + 12), _mm_castps_si128(v3));
}

internal_api u64 
r_render_camera_create_id(render_camera_t *camera)
{
    u64 result = 0;

    u32 temp_buffer[32];
    const float32 epsilon_value = 1e-5f;
    c_canonicalize_matrix_values(temp_buffer,      epsilon_value, &camera->view_matrix);  
    c_canonicalize_matrix_values(temp_buffer + 16, epsilon_value, &camera->projection_matrix); 

    result = c_fnv_hash_value((byte*)temp_buffer, sizeof(temp_buffer));
    return(result);
}

// NOTE(Sleepster): CAMERA ID STUFF

void
r_render_state_init(render_state_t *render_state, vulkan_render_context_t *render_context)
{
    ZeroStruct(*render_state);

    render_state->render_context     = render_context;
    render_state->current_frame_data = render_context->current_frame;

    render_state->renderer_arena = c_arena_create(MB(200));
    c_hash_table_init(&render_state->render_group_hash, 
                       MAX_HASHED_RENDER_GROUPS,
                      &render_state->renderer_arena,
                       renderer_hash_arena_allocate,
                       null);

    render_state->draw_frame.used_render_groups = c_arena_push_array(&render_state->renderer_arena, render_group_t*, MAX_RENDER_GROUPS);
    render_state->is_initialized = true;

    render_state->draw_frame.state.active_shader =  render_context->default_shader;
    render_state->draw_frame.state.active_camera = &render_context->test_camera;
}

render_camera_t
r_render_camera_create(mat4_t view_matrix, mat4_t projection_matrix)
{
    render_camera_t result;

    result.view_matrix       = view_matrix;
    result.projection_matrix = projection_matrix;
    result.ID                = r_render_camera_create_id(&result);

    return(result);
}

/*===========================================
  ============== RENDER GROUPS  =============
  ===========================================*/

internal_api inline render_geometry_batch_t*
r_render_group_create_new_geoemetry_buffer(render_state_t *render_state)
{
    render_geometry_batch_t *result = null;
    result = c_arena_push_struct(&render_state->renderer_arena, render_geometry_batch_t);

    result->instances   = c_arena_push_array(&render_state->renderer_arena, render_geometry_instance_t, MAX_VULKAN_INSTANCES);
    result->next_buffer = null;
    result->is_valid    = true;

    return(result);
}

render_geometry_batch_t*
r_render_group_get_current_buffer(render_state_t *render_state)
{
    render_geometry_batch_t *result = null;
    render_group_t           *active_render_group = render_state->draw_frame.state.active_render_group;

    // NOTE(Sleepster): Check if the cached buffer already has our camera. 
    draw_frame_t *draw_frame = &render_state->draw_frame;
    u64 camera_ID = r_render_camera_create_id(draw_frame->state.active_camera);
    if(active_render_group->cached_buffer->camera_data.ID == camera_ID)
    {
        // NOTE(Sleepster): If it does, just set it as our result and leave. 
        result = active_render_group->cached_buffer;
    }

    // NOTE(Sleepster): If the cached one isn't the one we need, look for it in the list of currently valid buffers 
    if(!result)
    {
        render_geometry_batch_t *found       = null;
        render_geometry_batch_t *last_buffer = null;
        for(render_geometry_batch_t *current_buffer = &draw_frame->state.active_render_group->first_buffer;
            current_buffer;
            current_buffer = current_buffer->next_buffer)
        {
            if(current_buffer->camera_data.ID == camera_ID || current_buffer->camera_data.ID == 0)
            {
                // NOTE(Sleepster): If we find it, great. Leave the loop and assign this new buffer into the "cached_buffer" ptr
                found = current_buffer;
                break;
            }
            last_buffer = current_buffer;
        }

        // NOTE(Sleepster): Failed to find it? Create one. 
        result = found;
        if(!result)
        {
            result = r_render_group_create_new_geoemetry_buffer(render_state);
            last_buffer->next_buffer = result;
        }
        Assert(result);
        Assert(result->is_valid);

        // NOTE(Sleepster): Assign this different buffer into our cached_buffer
        render_state->draw_frame.state.active_render_group->cached_buffer = result;
    }

    return(result);
}

// TODO(Sleepster): r_render_batch_*
render_group_t*
r_render_group_begin(render_state_t *render_state)
{
    render_group_t *result = null;

    u64 render_group_ID = c_fnv_hash_value((byte*)&render_state->draw_frame.state, sizeof(render_state->draw_frame.state));
    render_group_ID %= render_state->render_group_hash.header.max_entries;

    result = render_state->render_group_hash.data + render_group_ID;
    Assert(result);

    draw_frame_t *draw_frame = &render_state->draw_frame;
    result->ID                     = render_group_ID;
    result->dynamic_pipeline_state = draw_frame->state.active_pipeline_state;
    result->shader                 = draw_frame->state.active_shader;

    bool8 found = false;
    for(u32 group_index = 0;
        group_index < draw_frame->used_render_group_count;
        ++group_index)
    {
        render_group_t *render_group = draw_frame->used_render_groups[group_index];
        if(render_group->ID == result->ID)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        u32 next_group_index = render_state->draw_frame.used_render_group_count++;
        render_state->draw_frame.used_render_groups[next_group_index] = result;

        result->first_buffer       = *r_render_group_create_new_geoemetry_buffer(render_state);
        result->master_batch_array =  c_arena_push_array(&render_state->renderer_arena, render_geometry_instance_t, MAX_VULKAN_INSTANCES);
        result->cached_buffer      = &result->first_buffer;
    }

    draw_frame->state.active_render_group = result;
    return(result);
}

// TODO(Sleepster): r_render_batch_*
void
r_render_group_end(render_state_t *render_state)
{
    render_state->draw_frame.state.active_render_group = null;
}

void
r_render_group_fill_master_buffer(render_state_t *render_state, render_group_t *render_group)
{
    for(render_geometry_batch_t *current_buffer = &render_group->first_buffer;
        current_buffer;
        current_buffer = current_buffer->next_buffer)
    {
        render_geometry_instance_t *master_offset = render_group->master_batch_array + render_group->total_primitive_count;
        memcpy(master_offset, current_buffer->instances, sizeof(render_geometry_instance_t) * current_buffer->primitive_count);

        render_group->total_primitive_count += current_buffer->primitive_count;
    }
}

void
r_render_group_update_used_groups(render_state_t *render_state)
{
    draw_frame_t *draw_frame = &render_state->draw_frame;

    for(u32 group_index = 0;
        group_index < draw_frame->used_render_group_count;
        ++group_index)
    {
        render_group_t *render_group = draw_frame->used_render_groups[group_index];
        r_render_group_fill_master_buffer(render_state, render_group);
    }
}

/*===========================================
  =============== DRAWING API ===============
  ===========================================*/

void
r_set_active_render_layer(render_state_t *render_state, u32 render_layer)
{
    Assert(render_layer >  0);
    Assert(render_layer <= MAX_RENDER_LAYERS);

    draw_frame_t *draw_frame = &render_state->draw_frame;
    draw_frame->state.active_render_layer = render_layer;
}

internal_api bool8 
r_add_texture_to_group_array(render_group_t *render_group, texture2D_t *texture)
{
    bool8 result = false;

    bool8 found = false;
    for(u32 texture_index = 0;
        texture_index < render_group->current_texture_count;
        ++texture_index)
    {
        texture2D_t *current_texture = render_group->textures[texture_index];
        // NOTE(Sleepster): Just compare the pointers... 
        if(current_texture == texture)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        Assert(render_group->current_texture_count + 1 < MAX_RENDER_GROUP_BOUND_TEXTURES);

        render_group->textures[render_group->current_texture_count++] = texture;
        result = true;
    }

    return(result);
}

void
r_draw_texture_ex(render_state_t    *render_state, 
                  vec2_t             position, 
                  vec2_t             size, 
                  vec4_t             color, 
                  float32            rotation, 
                  subtexture_data_t *subtexture_data)
{
    render_geometry_batch_t *buffer = r_render_group_get_current_buffer(render_state);
    Assert(buffer->primitive_count + 4 < MAX_VULKAN_INSTANCES);
    render_geometry_instance_t *instance = buffer->instances + buffer->primitive_count++;

    const float32 near_value = 0;
    const float32 far_value  = 1;
    float32 depth_step       = (far_value - near_value) / MAX_RENDER_LAYERS;

    // NOTE(Sleepster): Current "depth" of the object, this is for z-layering 
    float32 layer_depth  = near_value + (render_state->draw_frame.state.active_render_layer * depth_step);

    mat4_t transform = mat4_identity();
    transform = mat4_translate(transform, vec2_expand_vec3(position, layer_depth));
    transform = mat4_scale(transform, vec2_expand_vec3(size, 1.0f));
    transform = mat4_transpose(transform);

    // transform = mat4_rotate(transform, {1.0, 1.0, 0.0}, rotation);

    subtexture_data_t *uv_data = subtexture_data;
    if(uv_data)
    {
        render_group_t *active_group = render_state->draw_frame.state.active_render_group;
        r_add_texture_to_group_array(active_group, &uv_data->atlas->texture);

        vec2_t uv_min = vec2_reduce(uv_data->uv_min, subtexture_data->atlas->atlas_size);
        vec2_t uv_max = vec2_reduce(uv_data->uv_max, subtexture_data->atlas->atlas_size);

        instance->uv_min        = uv_min;
        instance->uv_max        = uv_max;
        instance->texture_index = active_group->current_texture_count - 1;

        Assert(instance->texture_index >= 0);
    }
    else
    {
        instance->uv_min = vec2_create(0.0);
        instance->uv_max = vec2_create(1.0);

        instance->texture_index = -1;
    }

    instance->transform     = transform;
    instance->color         = color;
    instance->camera_index  = 0;
}

void
r_draw_texture(render_state_t *render_state, 
               vec2_t          position, 
               vec2_t          size, 
               vec4_t          color, 
               float32         rotation, 
               asset_handle_t *texture_handle)
{
    subtexture_data_t *subtexture_data = null; 
    if(texture_handle)
    {
        subtexture_data = texture_handle->subtexture_data;
    }
    r_draw_texture_ex(render_state, position, size, color, rotation, subtexture_data);
}

void
r_draw_rect(render_state_t *render_state, 
            vec2_t          position, 
            vec2_t          size, 
            vec4_t          color, 
            float32         rotation)
{
    r_draw_texture(render_state, position, size, color, rotation, null);
}
