/* ========================================================================
   $File: s_entity.cpp $
   $Date: August 22 2026 06:58 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_entity.h>

entity_t*
s_entity_create(entity_manager_t *entity_manager, u32 archetype, u32 flags)
{
    entity_t *result = null;
    for(u32 entity_index = 0;
        entity_index < MAX_ENTITIES;
        ++entity_index)
    {
        entity_t *found = entity_manager->entities + entity_index;
        if(!(found->flags & ENTITY_FLAG_IS_VALID))
        {
            result = found;
            result->flags     = (ENTITY_FLAG_IS_VALID|flags);
            result->archetype = archetype;
            result->ID        = entity_index;

            ++entity_manager->active_entities;

            break;
        }
    }

    return(result);
}

void
s_entity_destroy(entity_manager_t *entity_manager, entity_t *entity)
{
    void *entity_array_end = &entity_manager->entities[entity_manager->active_entities];
    memcpy(entity_array_end, entity, sizeof(entity_t));
    ZeroMemory(entity_array_end, sizeof(entity_t));

    --entity_manager->active_entities;
}

entity_query_t
s_entity_query_flags(entity_manager_t *entity_manager, u32 search_mask)
{
    entity_query_t result = {};
    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_ENTITIES);

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < entity_manager->active_entities;
        ++entity_index)
    {
        entity_t *entity = entity_manager->entities + entity_index;
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
    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_ENTITIES);

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < entity_manager->active_entities;
        ++entity_index)
    {
        entity_t *entity = entity_manager->entities + entity_index;
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
    result.entities = c_arena_push_array(&entity_manager->transient_storage, entity_t*, MAX_ENTITIES);

    s32 found_entity_count = 0;
    for(u32 entity_index = 0;
        entity_index < entity_manager->active_entities;
        ++entity_index)
    {
        entity_t *entity = entity_manager->entities + entity_index;
        if(entity->archetype == archetype)
        {
            result.entities[found_entity_count] = entity;
            ++found_entity_count;
        }
    }

    result.entity_count = found_entity_count;
    return(result);
}
