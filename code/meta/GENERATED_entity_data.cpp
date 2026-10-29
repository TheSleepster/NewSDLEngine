
struct entity_create_info_t {
	game_state_t* game_state;
	asset_manager_t* asset_manager;
	vec2_t position;
	vec2_t initial_position;
	vec2_t size;
	u32 extra_flags;
	u32 flags;
};
entity_t *s_entity_create_of_type(u32 entity_archetype, entity_create_info_t *create_info);

entity_t *s_entity_create_of_type(u32 entity_archetype, entity_create_info_t *create_info) {
	entity_t *result = nullptr;
	switch(entity_archetype) {
		case ENTITY_ARCHETYPE_PLAYER: {
			result = s_entity_player_create(create_info->game_state, create_info->asset_manager, create_info->position);
		}break;
		case ENTITY_ARCHETYPE_COLLIDER: {
			result = s_entity_collider_create(create_info->game_state, create_info->initial_position, create_info->size, create_info->extra_flags);
		}break;
		case ENTITY_ARCHETYPE_TILE: {
			result = s_entity_tile_create(create_info->game_state, create_info->position, create_info->flags);
		}break;
	}
	return(result);
}
