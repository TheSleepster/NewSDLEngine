/* ========================================================================
   $File: main.cpp $
   $Date: March 27 2026 06:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_hash_table.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_global_context.h>
#include <c_zone_allocator.h>
#include <c_program_flag_handler.h>
#include <c_tokenizer.h>
#include <p_platform_data.h>

#include <r_render_image.h>
#include <s_render_RHI.h>

#include <s_input_manager.h>
#include <s_asset_manager.h>

enum entity_type 
{
    ET_Invalid,
    ET_Player,
    ET_Count
};

enum entity_flags
{
    EF_Valid    = 1ul << 0,
    EF_Alive    = 1ul << 1,
    EF_Gravitic = 1ul << 2,
    EF_Actor    = 1ul << 3,
    EF_Static   = 1ul << 4,
    EF_IsGround = 1ul << 5,
};

// NOTE(Sleepster): owner_client_id is used to assign ownership of an entity 
// to that of a specific client 
struct entity_t
{
    u32    e_type;
    u32    e_flags;
    u32    owner_client_id;
    vec2_t last_position;
    vec2_t position;
    vec2_t velocity;
};

struct entity_manager_t
{
    entity_t entities[1000];
    u32      active_entities;
};


void
game_main(void)
{
    input_manager_t *input_manager   = global_context->input_manager;
    renderer_state_t *renderer_state = global_context->renderer_state;
    asset_manager_t *asset_manager   = global_context->asset_manager;

    input_controller_t *controller = s_im_get_primary_controller(global_context->input_manager);
}
