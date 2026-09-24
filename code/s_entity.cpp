/* ========================================================================
   $File: s_entity.cpp $
   $Date: August 22 2026 06:58 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_entity.h>

/* Min Region:
 *  vec2(-160, -90)
 * Max Region:
 *  vec2( 160,  90)
 *
 * Lookup -> vec2(128, -69) -> the above region
 *
 *
 * Min of other region:
 *  vec2(-320, -180)
 *
 * Max of other region:
 *  vec2( 320,  180)
 * 
 */

internal_api ivec2_t
get_sim_chunk_position(vec2_t world_position)
{
    ivec2_t result;

    const u32 matrix_center = (MAX_SIM_REGIONS * 0.5f);
    u32 chunk_x = floorf(((world_position.x + (SIM_REGION_WIDTH  * 0.5f)) / SIM_REGION_WIDTH)  + (matrix_center));
    u32 chunk_y = floorf(((world_position.y + (SIM_REGION_HEIGHT * 0.5f)) / SIM_REGION_HEIGHT) + (matrix_center));

    result.x = chunk_x;
    result.y = chunk_y;

    return(result);
}

world_sim_region_t*
s_entity_manager_get_sim_region(entity_manager_t *entity_manager, vec2_t world_position)
{    
    world_sim_region_t *result = null;

    ivec2_t chunk_position = get_sim_chunk_position(world_position); 

    u8 *active_region_index = &entity_manager->world_sim_region_sparse_matrix[chunk_position.x][chunk_position.y];
    if(*active_region_index != 0)
    {
        result = &entity_manager->active_sim_regions[*active_region_index];
    }

    return(result);
}

world_sim_region_t*
s_entity_manager_get_or_create_sim_region(entity_manager_t *entity_manager, vec2_t world_position)
{
    world_sim_region_t *result = null;
    
    ivec2_t chunk_position = get_sim_chunk_position(world_position); 
    u8 *active_region_index = &entity_manager->world_sim_region_sparse_matrix[chunk_position.x][chunk_position.y];
    if(*active_region_index == 0)
    {
        *active_region_index = entity_manager->active_region_count + 1;
    }

    result = entity_manager->active_sim_regions + *active_region_index;
    result->world_chunk_hash = ivec2(chunk_position.x, chunk_position.y);

    return(result);
}

entity_t*
s_entity_create(entity_manager_t *entity_manager, vec2_t world_position, u32 archetype, u32 flags)
{
    world_sim_region_t *sim_region = s_entity_manager_get_or_create_sim_region(entity_manager, world_position);

    entity_t *result = null;
    for(u32 entity_index = 0;
        entity_index < MAX_SIM_REGION_ENTITIES;
        ++entity_index)
    {
        entity_t *found = sim_region->entities + entity_index;
        if(!(found->flags & ENTITY_FLAG_IS_VALID))
        {
            result = found;
            ZeroStruct(*result);

            result->flags     = (ENTITY_FLAG_IS_VALID|flags);
            result->archetype =  archetype;
            result->ID        =  entity_index;

            ++sim_region->sim_entity_count;
            break;
        }
    }

    return(result);
}

void
s_entity_destroy(entity_manager_t *entity_manager, entity_t *entity)
{
    world_sim_region_t *sim_region = s_entity_manager_get_or_create_sim_region(entity_manager, entity->position);

    void *entity_array_end = &sim_region->entities[sim_region->sim_entity_count];
    memcpy(entity_array_end, entity, sizeof(entity_t));
    ZeroMemory(entity_array_end, sizeof(entity_t));

    --sim_region->sim_entity_count;
}

entity_query_t
s_entity_query_flags(entity_manager_t *entity_manager, u32 search_mask)
{
    entity_query_t result = {};
    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_SIM_REGION_ENTITIES);

    // NOTE(Sleepster): Active sim region 
    world_sim_region_t *sim_region = s_entity_manager_get_or_create_sim_region(entity_manager, vec2_zero());

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < sim_region->sim_entity_count;
        ++entity_index)
    {
        entity_t *entity = sim_region->entities + entity_index;
        if((entity->flags & search_mask) != 0)
        {
            result.entities[found_entity_count] = entity;
            ++found_entity_count;
        }
    }

    result.entity_count = found_entity_count;
    return(result);
}

entity_query_t
s_entity_query_flags_exact(entity_manager_t *entity_manager, u32 search_mask)
{
    entity_query_t result = {};

    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_SIM_REGION_ENTITIES);

    // NOTE(Sleepster): Active sim region 
    world_sim_region_t *sim_region = s_entity_manager_get_or_create_sim_region(entity_manager, vec2_zero());

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < sim_region->sim_entity_count;
        ++entity_index)
    {
        entity_t *entity = sim_region->entities + entity_index;
        if((entity->flags & search_mask) == search_mask)
        {
            result.entities[found_entity_count] = entity;
            ++found_entity_count;
        }
    }

    result.entity_count = found_entity_count;
    return(result);
}

entity_query_t
s_entity_query_archetype(entity_manager_t *entity_manager, entity_archetype_t archetype)
{
    entity_query_t result = {};

    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_SIM_REGION_ENTITIES);

    // NOTE(Sleepster): Active sim region 
    world_sim_region_t *sim_region = s_entity_manager_get_or_create_sim_region(entity_manager, vec2_zero());

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < sim_region->sim_entity_count;
        ++entity_index)
    {
        entity_t *entity = sim_region->entities + entity_index;
        if(entity->archetype == archetype)
        {
            result.entities[found_entity_count] = entity;
            ++found_entity_count;
        }
    }

    result.entity_count = found_entity_count;
    return(result);
}

/* =======================================
 * COLLISIONS
 * =======================================
 */

struct sweep_result_t
{
    bool32  hit;
    float32 toi;
    vec2_t  normal;
};

sweep_result_t
entity_sweep_test(rectangle2_t *A, rectangle2_t *B, vec2_t velocity)
{
    sweep_result_t result = {};

    // NOTE(Sleepster): minkowski rectangle 
    float32 left   = A->min.x - B->half_size.x;
    float32 right  = A->max.x + B->half_size.x;
    float32 bottom = A->min.y - B->half_size.y;
    float32 top    = A->max.y + B->half_size.y;

    // NOTE(Sleepster): sweep test 
    {
        vec2_t initial_position = B->center;
        vec2_t displacement     = velocity;

        float32 points[4] = {left, right, bottom, top};
        vec2_t collision_normal = vec2_zero();

        float32 t_enter = -INFINITY;
        float32 t_exit  =  INFINITY;
        for(s32 axis = 0;
            axis < 2;
            ++axis)
        {
            s32 index = axis * 2;
            if(displacement.elements[axis] != 0.0f)
            {
                float32 inverse_displacement = 1.0f / displacement.elements[axis];

                float32 time1 = (points[index]     - initial_position.elements[axis]) * inverse_displacement;
                float32 time2 = (points[index + 1] - initial_position.elements[axis]) * inverse_displacement;

                float32 t_near = Min(time1, time2);
                float32 t_far  = Max(time1, time2);
                if(t_near > t_enter)
                {
                    t_enter = t_near;
                    collision_normal = vec2_zero(); 
                    collision_normal.elements[axis] = (displacement.elements[axis] > 0.0f) ? -1.0f : 1.0f;
                }

                t_exit = Min(t_exit, t_far);
                if(t_enter > t_exit)
                {
                    return(result);
                }
            }
            else
            {
                if(initial_position.elements[axis] < points[index] ||
                   initial_position.elements[axis] > points[index + 1])

                {
                    return(result);
                }
            }
        }

        if(t_enter <= t_exit && t_exit >= 0.0f && t_enter <= 1.0f)
        {    
            result.hit    = true;
            result.toi    = t_enter;
            result.normal = collision_normal;
        }
    }

    return(result);
}

struct collision_info_t
{
    bool32  hit;
    float32 toi;
    vec2_t  normal;
};

struct collision_query_result_t 
{
    bool32  hit;
    float32 depth;
    union {
        collision_info_t axis[2];
        struct {
            collision_info_t x;
            collision_info_t y;
        };
    };
};

collision_query_result_t
s_entity_test_collisions(entity_t *entity, entity_t *collider)
{
    collision_query_result_t result = {};

    vec2_t target_velocity = entity->velocity;
    rectangle2_t *collider_rect = &collider->bounding_box;

    vec2_t target_velocity_x = vec2(target_velocity.x, 0.0f);
    vec2_t target_velocity_y = vec2(0.0f, target_velocity.y);
    sweep_result_t result_x = entity_sweep_test(collider_rect, &entity->bounding_box, target_velocity_x);
    sweep_result_t result_y = entity_sweep_test(collider_rect, &entity->bounding_box, target_velocity_y);

    vec2_t center_difference = vec2_subtract(entity->bounding_box.center, collider_rect->center);
    vec2_t overlap = {
        (entity->bounding_box.half_size.x + collider_rect->half_size.x) - fabs(center_difference.x),
        (entity->bounding_box.half_size.y + collider_rect->half_size.y) - fabs(center_difference.y)
    };

    float32 depth = Min(overlap.x, overlap.y);

    result.depth = depth;
    result.hit   = (result_x.hit || result_y.hit);
    if(result_x.hit)
    {
        result.x.hit    = true;
        result.x.toi    = result_x.toi;
        result.x.normal = result_x.normal;
    }

    if(result_y.hit)
    {
        result.y.hit    = true;
        result.y.toi    = result_y.toi;
        result.y.normal = result_y.normal;
    }

    return(result);
}
