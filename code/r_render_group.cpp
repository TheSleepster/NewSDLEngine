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

render_group_t*
r_render_group_begin(render_state_t *render_state)
{
    render_group_t *result = null;

    u64 render_group_ID = c_fnv_hash_value((byte*)&render_state->draw_frame.state, sizeof(render_state->draw_frame.state));
    render_group_ID %= render_state->render_group_hash.header.max_entries;

    result = render_state->render_group_hash.data + render_group_ID;
    Assert(result);

    draw_frame_t *draw_frame = &render_state->draw_frame;
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
    }

    draw_frame->state.active_render_group = result;
    return(result);
}

void
r_render_group_end(render_state_t *render_state)
{
    render_state->draw_frame.state.active_render_group = null;
}
