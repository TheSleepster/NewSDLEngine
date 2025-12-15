#if !defined(G_GAME_STATE_H)
/* ========================================================================
   $File: g_game_state.h $
   $Date: December 04 2025 04:36 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define G_GAME_STATE_H
#include <SDL3/SDL.h>

#include <c_base.h>
#include <c_types.h>
#include <c_math.h>

#include <p_platform_data.h>

#include <g_entity.h>

struct input_data_t
{
    vec2_t input_axis;
};

#define MAX_BUFFERED_INPUTS (1024)

struct client_data_t 
{
    u32                ID;
    bool32             connected;
    sockaddr_storage   address;
    socklen_t          addr_len;

    entity_t          *player;
    input_data_t       input_data_buffer[MAX_BUFFERED_INPUTS];
    u32                input_data_head;
    u32                input_data_tail;
};

struct game_state_t
{
    SDL_Window        *window;
    vec2_t             window_size;

    vec2_t             input_axis;
    entity_manager_t   entity_manager;

    entity_t          *player;

    bool8              is_host;
    s32                socket;
    struct sockaddr_in host_address_data;

    u32                client_id;
    client_data_t      clients[4];
    u32                connected_client_count;
};


#endif // G_GAME_STATE_H

