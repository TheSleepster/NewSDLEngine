#if !defined(G_ENTITY_H)
/* ========================================================================
   $File: g_entity.h $
   $Date: December 04 2025 04:35 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define G_ENTITY_H
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>

enum entity_flags
{
    EF_Valid    = 1ul << 0,
    EF_Alive    = 1ul << 1,
    EF_Gravitic = 1ul << 2,
    EF_Actor    = 1ul << 3,
    EF_Static   = 1ul << 4,
    EF_IsGround = 1ul << 5,
};

struct entity_t
{
    u32    e_flags;
    vec2_t last_position;
    vec2_t position;
    vec2_t velocity;
};

struct entity_manager_t
{
    entity_t entities[1000];
    u32      active_entities;
};

struct game_state_t;
entity_t* entity_create(game_state_t *state);

#endif // G_ENTITY_H

