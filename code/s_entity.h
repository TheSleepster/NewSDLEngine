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

#define MAX_ENTITIES (100000)

enum entity_archetype_t
{
    ENTITY_ARCHETYPE_INVALID,
    ENTITY_ARCHETYPE_PLAYER,
    ENTITY_ARCHETYPE_COLLIDER,
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
    ENTTIY_FLAG_STATIC         = BIT(6),
    ENTITY_FLAG_HAS_SPRITE     = BIT(7),
    ENTITY_FLAG_HAS_COLLIDER   = BIT(8),
    ENTITY_FLAG_ANIMATED       = BIT(9),
    ENTITY_FLAG_IS_GROUND      = BIT(10),
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
    vec2_t          last_position;
    vec2_t          render_position;
    vec2_t          position;
    vec2_t          velocity;

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

struct entity_manager_t
{
    memory_arena_t transient_storage;
    
    entity_t       entities[MAX_ENTITIES];
    u32            active_entities;

    // NOTE(Sleepster): Maybe instead make a hash_table that maps entity->flags to that of an array of entities 
    // do the same for entity->archetype -> entity array 
#if 0
    entity_t      *players[MAX_ENTITIES];
    entity_t      *colliders[MAX_ENTITIES];
#endif
};

struct entity_query_t
{
    entity_t **entities;
    u32        entity_count;

    entity_t **begin() { return(entities); }
    entity_t **end()   { return(entities + entity_count); }
};

entity_t*      s_entity_create(entity_manager_t *entity_manager, u32 archetype, u32 flags);
void           s_entity_destroy(entity_manager_t *entity_manager, entity_t *entity);
entity_query_t s_entity_query_flags(entity_manager_t *entity_manager, u32 search_mask);
entity_query_t s_entity_query_flags_exact(entity_manager_t *entity_manager, u32 search_mask);
entity_query_t s_entity_query_archetype(entity_manager_t *entity_manager, entity_archetype_t archetype);

#endif // S_ENTITY_H

