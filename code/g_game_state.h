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

#include <g_entity.h>

#if OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#include <windows.h>

#undef errno
#define errno WSAGetLastError()
#include <winsock2.h>
#include <ws2tcpip.h>

#define SOCKET int
#else
#error "window only..."
#endif

struct client_data_t 
{
    u32                ID;
    bool32             connected;
    struct sockaddr_in address;
    socklen_t          addr_len;
};

struct game_state_t
{
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    vec2_t           window_size;

    vec2_t           input_axis;
    entity_manager_t entity_manager;

    entity_t        *player;

    bool8              is_host;
    s32                socket;
    struct sockaddr_in host_address_data;

    client_data_t      clients[4];
    u32                connected_client_count;
};


#endif // G_GAME_STATE_H

