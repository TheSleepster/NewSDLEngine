#if !defined(S_ENTITY_H)
/* ========================================================================
   $File: s_entity.h $
   $Date: August 22 2026 06:59 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ENTITY_H
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>

#include <s_RHI_image.h>
#include <s_RHI_core.h>
#include <c_duration_counter.h>
#include <r_immediate_rendering.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>

struct animation2D_t;

#define MAX_SIM_REGION_ENTITIES (1000)

constexpr u32 MAX_SIM_REGIONS        = 4096;
constexpr u32 SIM_REGION_WIDTH       = 320;
constexpr u32 SIM_REGION_HEIGHT      = 180;
constexpr u32 MAX_ACTIVE_SIM_REGIONS = 51;

enum entity_archetype_t
{
    ENTITY_ARCHETYPE_INVALID,
    ENTITY_ARCHETYPE_PLAYER,
    ENTITY_ARCHETYPE_COLLIDER,
    ENTITY_ARCHETYPE_TILE,
    ENTITY_ARCHETYPE_COUNT
};

enum entity_flags_t
{
    ENTITY_FLAG_NONE           = 0,
    ENTITY_FLAG_IS_VALID       = BIT(1),
    ENTITY_FLAG_USES_TRANSFORM = BIT(2),
    ENTITY_FLAG_ALIVE          = BIT(3),
    ENTITY_FLAG_GRAVITIC       = BIT(4),
    ENTITY_FLAG_ACTOR          = BIT(5),
    ENTITY_FLAG_STATIC         = BIT(6),
    ENTITY_FLAG_HAS_SPRITE     = BIT(7),
    ENTITY_FLAG_HAS_COLLIDER   = BIT(8),
    ENTITY_FLAG_ANIMATED       = BIT(9),
    ENTITY_FLAG_IS_GROUND      = BIT(10),
    ENTITY_FLAG_GROUNDED       = BIT(11),
};

// NOTE(Sleepster): owner_client_id is used to assign ownership of an entity 
// to that of a specific client 
struct entity_t
{
    // NOTE(Sleepster): Base Entity 
    u32             ID;
    u32             archetype;
    u32             flags;

    // NOTE(Sleepster): Transform data 
    vec2_t          position;
    vec2_t          last_position;
    vec2_t          render_position;

    vec2_t          editor_position;

    vec2_t          velocity;
    vec2_t          max_velocity;

    vec2_t          acceleration;
    vec2_t          max_acceleration;
    vec2_t          friction;

    vec2_t          size;
    float32         rotation;

    // NOTE(Sleepster): Sprite data 
    s32             direction_x;
    asset_handle_t  sprite;

    animation2D_t  *animations;
    u32             animation_count;
    u32             animation_state;

    // NOTE(Sleepster): Colliders
    rectangle2_t    bounding_box;
    bool8           collision;
};

struct world_sim_region_t
{
    ivec2_t   world_chunk_hash;

    array_t<entity_t, MAX_SIM_REGION_ENTITIES> entities;
    u32                                        sim_entity_count;
};

// NOTE(Sleepster): This isn't a real solution for the world sim storage... 
struct entity_manager_t
{
    memory_arena_t transient_storage;
    u8             world_sim_region_sparse_matrix[MAX_SIM_REGIONS][MAX_SIM_REGIONS];

    // NOTE(Sleepster): index 0 is always invalid 
    array_t<world_sim_region_t, MAX_ACTIVE_SIM_REGIONS> active_sim_regions;
    u32                                                 active_region_count;
};

struct entity_query_t
{
    entity_t **entities;
    u32        entity_count;

    entity_t **begin() { return(entities); }
    entity_t **end()   { return(entities + entity_count); }
};

world_sim_region_t* s_entity_manager_get_sim_region(entity_manager_t *entity_manager, vec2_t world_position);
world_sim_region_t* s_entity_manager_get_or_create_sim_region(entity_manager_t *entity_manager, vec2_t world_position);

entity_t      *s_entity_create(entity_manager_t *entity_manager, vec2_t world_position, u32 archetype, u32 flags);
void           s_entity_destroy(entity_manager_t *entity_manager, entity_t *entity);
entity_query_t s_entity_query_flags(entity_manager_t *entity_manager, u32 search_mask);
entity_query_t s_entity_query_flags_exact(entity_manager_t *entity_manager, u32 search_mask);
entity_query_t s_entity_query_archetype(entity_manager_t *entity_manager, entity_archetype_t archetype);

#endif // S_ENTITY_H

