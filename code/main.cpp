/* ========================================================================
   $File: main.cpp $
   $Date: November 28 2025 06:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>

#include <g_game_state.h>
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

global_variable bool8 g_running = false;

constexpr float32 gcv_tick_rate = 1.0f / 60.0f;

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

bool8
s_nt_socket_api_init(void)
{
    bool8 result = true;
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

    return(result);
}

void
s_nt_init_client_data(game_state_t *state, char *host_ip)
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
        bind_addr.sin_port = htons(6969);
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
        printf("[HOST]: Listening on port %d\n", 6969);
    }
    else
    {
        state->host_address_data.sin_family = AF_INET;
        state->host_address_data.sin_port   = htons(6969);
        inet_pton(AF_INET, host_ip, &state->host_address_data.sin_addr);

        packet_t packet = {};
        packet.type     = PT_Connect;
        sendto(state->socket, 
               (char*)&packet, 
               sizeof(packet_t), 
               0,
               (struct sockaddr*)&state->host_address_data, 
               sizeof(state->host_address_data));

        printf("[CLIENT]: Connecting to %s:%d\n", host_ip, 6969);
    }
}

void
s_nt_client_check_packets(game_state_t *state) 
{
    packet_t packet;
    struct sockaddr_in from;
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

                        vec2_t input_axis = packet.payload.input_data.input_axis; 
                        if(input_axis.x != 0.0f)
                        {
                            printf("Input axis: '%.02f', '%.02f'...\n", input_axis.x, input_axis.y);
                        }
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

int
main(int argc, char **argv)
{
    game_state_t *state = Alloc(game_state_t);
    ZeroStruct(*state);

    state->window_size = vec2(600, 600);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        bool8 result = s_nt_socket_api_init();
        Expect(result, "Failure to init the Windows Socket API...\n");
        if(argc >= 2)
        {
            if(strcmp(argv[1], "--client") == 0)
            {
                state->is_host = false;
                if(argc < 3) 
                {
                    fprintf(stderr, "Please pass the ip to connect too...\n");
                    exit(0);
                }
                char *host_ip = argv[2];
                s_nt_init_client_data(state, host_ip);
            }
            else if(strcmp(argv[1], "--host") == 0)
            {
                state->is_host = true;
                s_nt_init_client_data(state, null);
            }
        }

        if(SDL_CreateWindowAndRenderer("Game", 
                                       (u32)state->window_size.x,
                                       (u32)state->window_size.y, 
                                       SDL_WINDOW_RESIZABLE, 
                                      &state->window, 
                                      &state->renderer))
        {
            u64 perf_count_freq = SDL_GetPerformanceFrequency();
            u64 last_tsc        = SDL_GetPerformanceCounter();
            u64 current_tsc     = 0;
            u64 delta_tsc       = 0;

            float32 delta_time    = 0;
            //float32 delta_time_ms = 0;
            float64 dt_accumulator = 0.0f;

            g_running = true;
            while(g_running)
            {
                state->input_axis = {};

                SDL_Event event;
                while(SDL_PollEvent(&event))
                {
                    switch(event.type)
                    {
                        case SDL_EVENT_QUIT:
                        {
                            g_running = false;
                        }break;
                    }
                }
                const bool *keyboard_state = SDL_GetKeyboardState(null);
                if(keyboard_state[SDL_SCANCODE_W]) 
                {
                    state->input_axis.y = -1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_A])
                {
                    state->input_axis.x = -1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_S])
                {
                    state->input_axis.y =  1.0f;
                }
                if(keyboard_state[SDL_SCANCODE_D])
                {
                    state->input_axis.x =  1.0f;
                }

                if(delta_time >= (gcv_tick_rate * 2.0f))
                {
                    delta_time = gcv_tick_rate * 2.0f;
                }

                dt_accumulator += delta_time;
                while(dt_accumulator >= gcv_tick_rate)
                {
                    client_data_t *client   = state->clients + state->client_id;
                    input_data_t input_data = {.input_axis = state->input_axis};

                    client->input_data_buffer[client->input_data_head] = input_data;
                    client->input_data_head = (client->input_data_head + 1) % MAX_BUFFERED_INPUTS;

                    // TODO(Sleepster): If we have any input packets form the host, reconcile here... 
                    // If we are the host update the client's players...
                    s_nt_client_send_packets(state);
                    s_nt_client_check_packets(state);

                    for(u32 entity_index = 0;
                        entity_index < state->entity_manager.active_entities;
                        ++entity_index)
                    {
                        entity_t *entity = state->entity_manager.entities + entity_index;
                        switch(entity->e_type)
                        {
                            case ET_Player:
                            {
                                client_data_t *entity_client = state->clients + entity->owner_client_id;
                                while(entity_client->input_data_tail != entity_client->input_data_head)
                                {
                                    input_data_t *input_data = entity_client->input_data_buffer + entity_client->input_data_tail;
                                    entity_client->input_data_tail = (entity_client->input_data_tail + 1) % MAX_BUFFERED_INPUTS;
                                    entity_simulate_player(entity, input_data, gcv_tick_rate);
                                }
                            }break;
                        }
                    }
                    
                    // TODO(Sleepster): Send out input packets from both the host and the other clients.
                    dt_accumulator -= gcv_tick_rate;
                }
                float32 alpha = (dt_accumulator / gcv_tick_rate);

                SDL_SetRenderDrawColor(state->renderer, 1, 0, 0, 0);
                SDL_RenderClear(state->renderer);
                
                for(u32 entity_index = 0;
                    entity_index < state->entity_manager.active_entities;
                    ++entity_index)
                {
                    entity_t *entity = state->entity_manager.entities + entity_index;
                    SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
                    vec2_t lerped_pos = vec2_lerp(entity->last_position, entity->position, alpha);
                    SDL_FRect rect = {
                        .x = lerped_pos.x,
                        .y = lerped_pos.y,
                        .w = 60,
                        .h = 60
                    };
                    SDL_RenderFillRect(state->renderer, &rect);
                }

                SDL_RenderPresent(state->renderer);

                current_tsc = SDL_GetPerformanceCounter();
                delta_tsc   = current_tsc - last_tsc;
                last_tsc    = current_tsc;

                delta_time    = (float32)(((float64)delta_tsc) / (float64)perf_count_freq);

                //delta_time_ms = delta_time * 1000.0f;
                //printf("delta time: '%.02f'...\n", delta_time_ms);
            }
        }
        else
        {
            Assert(false);
        }
    }
    else
    {
        Assert(false);
    }
}
