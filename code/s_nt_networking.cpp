/* ========================================================================
   $File: s_nt_networking.cpp $
   $Date: December 06 2025 03:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <s_nt_networking.h>
#include <stdio.h>

bool8
s_nt_socket_api_init(void)
{
    bool8 result = true;
#if OS_WINDOWS
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        fprintf(stderr, "WSAStartup failed.\n");
        result = false;
    }

    if(LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2)
    {
        fprintf(stderr,"Version 2.2 of Winsock not available.\n");
        WSACleanup();
        result = false;
    }
#endif

    return(result);
}

void
s_nt_init_client_data(game_state_t *state, char *host_ip, u32 port)
{    
    state->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (state->socket == INVALID_SOCKET) 
    {
        fprintf(stderr, "Socket creation failed: '%d'\n", errno);
    }

    u_long mode = 1;
    ioctlsocket(state->socket, FIONBIO, &mode);

    if(state->is_host)
    {     
        struct sockaddr_in bind_addr = {0};
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        bind_addr.sin_port = htons(port);
        if(bind(state->socket, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0) 
        {
            fprintf(stderr, "Failed to bind the socket... Error: '%d'...\n", errno);
        }

        client_data_t *client = state->clients + state->connected_client_count;
        client->ID            = state->connected_client_count;

        client->ID     = state->connected_client_count;
        client->player = entity_create(state);
        client->player->e_type = ET_Player;
        client->player->owner_client_id = client->ID; 

        state->connected_client_count += 1;
        printf("[HOST]: Listening on port %d\n", ntohs(bind_addr.sin_port));
    }
    else
    {
        state->host_address_data.sin_family = AF_INET;
        state->host_address_data.sin_port   = htons(port);
        inet_pton(AF_INET, host_ip, &state->host_address_data.sin_addr);

        packet_t packet = {};
        packet.type     = PT_Connect;
        sendto(state->socket, 
               (char*)&packet, 
               sizeof(packet_t), 
               0,
               (struct sockaddr*)&state->host_address_data, 
               sizeof(state->host_address_data));

        printf("[CLIENT]: Connecting to %s:%d\n", host_ip, htons(state->host_address_data.sin_port));
    }
}

void
s_nt_client_check_packets(game_state_t *state) 
{
    packet_t packet;
    struct sockaddr_storage from;
    socklen_t sock_addr_size = sizeof(from);

    for(;;)
    {
        int bytes = recvfrom(state->socket, 
                             (char*)&packet, 
                             sizeof(packet_t), 0,
                             (struct sockaddr*)&from, 
                             &sock_addr_size);
        if(bytes > 0) 
        {
            if(packet.magic_value == htonl(cv_packet_magic_value))
            {
                switch(packet.type) 
                {
                    case PT_Connect: 
                    {
                        Assert(state->is_host);
                        Assert(state->connected_client_count < 4);

                        client_data_t *client = state->clients + state->connected_client_count;
                        client->ID        = state->connected_client_count;
                        client->address   = from;
                        client->addr_len  = sock_addr_size;
                        client->connected = true;

                        client->player                  = entity_create(state);
                        client->player->owner_client_id = client->ID;
                        client->player->e_type          = ET_Player;

                        packet_t response;
                        response.type      = PT_ConnectAccepted;
                        response.client_id = htonl(client->ID);

                        state->connected_client_count += 1;
                        sendto(state->socket, 
                               (char*)&response, 
                               sizeof(packet_t), 
                               0, 
                               (struct sockaddr*)&from, 
                               sock_addr_size);

                        printf("[HOST]: Client %d connected\n", client->ID);

                    }break;
                    case PT_ConnectAccepted:
                    {
                        Assert(!state->is_host);

                        client_data_t *host_data = state->clients;
                        host_data->ID        = 0;
                        host_data->address   = from;
                        host_data->addr_len  = sock_addr_size;
                        host_data->connected = true;

                        host_data->player                  = entity_create(state);
                        host_data->player->owner_client_id = host_data->ID;
                        host_data->player->e_type          = ET_Player;

                        state->connected_client_count += 1;
                        state->client_id               = state->connected_client_count;

                        client_data_t *client_data = state->clients + state->connected_client_count;
                        client_data->ID            = state->connected_client_count; 

                        client_data->player                  = entity_create(state); 
                        client_data->player->e_type          = ET_Player;
                        client_data->player->owner_client_id = state->client_id;

                        state->connected_client_count += 1;
                        printf("[CLIENT]: ID '%d' Connected...\n", state->client_id);
                    }break;
                    case PT_InputData:
                    {
                        client_data_t *client = state->clients + packet.client_id;
                        client->input_data_buffer[client->input_data_head] = packet.payload.input_data;
                        client->input_data_head = (client->input_data_head + 1) % MAX_BUFFERED_INPUTS;
                    }break;
                    default: 
                    {
                        InvalidCodePath;
                    }break;
                }
            }
        }
        else
        {
            break;
        }
    }
}

void
s_nt_client_send_packets(game_state_t *state)
{
    client_data_t *host = state->clients;
    client_data_t *our_client = state->clients + state->client_id;

    // NOTE(Sleepster): If we're NOT the host, send our own input data to the host
    if(state->client_id != 0)
    {
        packet_t packet  = {};
        packet.client_id = state->client_id;
        packet.type      = PT_InputData;

        if(our_client->input_data_tail != our_client->input_data_head)
        {
            // NOTE(Sleepster): DO NOT increment tail here... we need to use this "packet" later. 
            input_data_t input_data   = our_client->input_data_buffer[our_client->input_data_tail];
            packet.payload.input_data = input_data;

            sendto(state->socket,
                   (char*)&packet,
                   sizeof(packet_t),
                   0,
                   (struct sockaddr*)&host->address,
                   host->addr_len);
        }
    }

    // NOTE(Sleepster): If we're the host, echo packets from all clients to all OTHER clients
    if(state->client_id == 0)
    {
        for(u32 connected_clients = 0; 
            connected_clients < state->connected_client_count; 
            ++connected_clients)
        {
            client_data_t *source_client = state->clients + connected_clients;
            
            if(source_client->input_data_tail != source_client->input_data_head)
            {
                packet_t echo_packet  = {};
                echo_packet.client_id = connected_clients;
                echo_packet.type      = PT_InputData;
                echo_packet.payload.input_data = source_client->input_data_buffer[source_client->input_data_tail];

                for(u32 other_client_index = 0; 
                    other_client_index < state->connected_client_count; 
                    ++other_client_index)
                {
                    if(other_client_index != connected_clients)
                    {
                        client_data_t *dest_client = state->clients + other_client_index;
                        sendto(state->socket,
                               (char*)&echo_packet,
                               sizeof(packet_t),
                               0,
                               (struct sockaddr*)&dest_client->address,
                               dest_client->addr_len);
                    }
                }
            }
        }
    }
}
