#if !defined(S_NT_NETWORKING_H)
/* ========================================================================
   $File: s_nt_networking.h $
   $Date: December 06 2025 03:42 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_NT_NETWORKING_H
#include <g_game_state.h>
#include <g_entity.h>

#include <c_base.h>
#include <c_types.h>

enum packet_type_t
{
    PT_Invalid,
    PT_Connect,
    PT_ConnectAccepted,
    PT_Disconnect,
    PT_InputData,
    PT_Count,
};

#define PACKET_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
constexpr u32 cv_packet_magic_value = PACKET_MAGIC_VALUE('P', 'A', 'C', 'K');

struct packet_t
{
    u32 magic_value = htonl(cv_packet_magic_value);
    u32 type;
    u32 client_id;
    union 
    {
        input_data_t input_data;
    }payload;
};

bool8 s_nt_socket_api_init(game_state_t *state, int argc, char **argv);
void  s_nt_init_client_data(game_state_t *state, char *host_ip, u32 port);
void  s_nt_client_check_packets(game_state_t *state);
void  s_nt_client_send_packets(game_state_t *state);

#endif // S_NT_NETWORKING_H

