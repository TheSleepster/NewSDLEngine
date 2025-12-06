/* ========================================================================
   $File: g_entity.cpp $
   $Date: December 04 2025 04:35 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <g_game_state.h>
#include <g_entity.h>

entity_t* 
entity_create(game_state_t *state)
{
    entity_t *new_entity = null;

    for(u32 entity_index = 0;
        entity_index < 1000;
        ++entity_index)
    {
        entity_t *entity = state->entity_manager.entities + entity_index;
        if(!(entity->e_flags & EF_Valid))
        {
            new_entity = entity;
            break;
        }
    }
    Assert(new_entity);
    ZeroStruct(*new_entity);

    new_entity->owner_client_id = (state->clients + state->client_id)->ID;
    new_entity->e_flags = EF_Valid;

    ++state->entity_manager.active_entities;
    return(new_entity);
}

void
entity_simulate_player(entity_t *player, input_data_t *input_data, float32 tick_rate)
{
    player->last_position = player->position;

    player->velocity.x = (input_data->input_axis.x * (200.5f * tick_rate));
    player->velocity.y = (input_data->input_axis.y * (200.5f * tick_rate));

    player->position = vec2_add(player->position, player->velocity);
    player->velocity = vec2(0.0f, 0.0f);
}
