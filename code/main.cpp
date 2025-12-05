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

enum packet_type_t
{
    PT_Invalid,
    PT_Connect,
    PT_ConnectAccepted,
    PT_Disconnect,
    PT_Input,
    PT_Count,
};

#define PACKET_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
constexpr u32 cv_packet_magic_value = PACKET_MAGIC_VALUE('P', 'A', 'C', 'K');

struct packet_t
{
    u32 magic_value = cv_packet_magic_value;
    u32 type;
    union 
    {
        void *data;
        u32   size;
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

        printf("[HOST]: Listening on port %d\n", 6969);
    }
    else
    {
        state->host_address_data.sin_family = AF_INET;
        state->host_address_data.sin_port   = htons(6969);
        inet_pton(AF_INET, host_ip, &state->host_address_data.sin_addr);

        packet_t packet = {};
        packet.type = PT_Connect;
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
            if(packet.magic_value == cv_packet_magic_value)
            {
                switch(packet.type) 
                {
                    case PT_Connect: 
                    {
                        Assert(state->is_host);
                        client_data_t *client = state->clients + state->connected_client_count;
                        client->ID = state->connected_client_count;
                        client->address   =  from;
                        client->addr_len  =  sock_addr_size;
                        client->connected = true;

                        state->connected_client_count += 1;

                        packet_t response;
                        response.magic_value = cv_packet_magic_value;
                        response.type        = PT_ConnectAccepted;

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
                        printf("[CLIENT]: Connected...\n");
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

int
main(int argc, char **argv)
{
    game_state_t *state = Alloc(game_state_t);
    state->window_size = vec2(600, 600);
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        bool8 result = s_nt_socket_api_init();
        Expect(result, "Failure to init the Windows Socket API...\n");
        if(argc >= 2)
        {
            if(strcmp(argv[1], "--client") == 0)
            {
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
            float32 tick_rate = 1.0f / 60.0f;

            state->player = entity_create(state);

            g_running = true;
            while(g_running)
            {
                s_nt_client_check_packets(state);

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

                if(delta_time >= (tick_rate * 2.0f))
                {
                    delta_time = tick_rate * 2.0f;
                }

                dt_accumulator += delta_time;
                while(dt_accumulator >= tick_rate)
                {
                    // TODO(Sleepster): If we have any input packets form the host, reconcile here...
                    // TODO(Sleepster): Send out input packet...
                     
                    state->player->last_position = state->player->position;

                    state->player->velocity.x = (state->input_axis.x * (200.5f * tick_rate));
                    state->player->velocity.y = (state->input_axis.y * (200.5f * tick_rate));

                    state->player->position = vec2_add(state->player->position, state->player->velocity);
                    state->player->velocity = vec2(0.0f, 0.0f);

                    dt_accumulator -= tick_rate;

                }
                float32 alpha = (dt_accumulator / tick_rate);

                SDL_SetRenderDrawColor(state->renderer, 1, 0, 0, 0);
                SDL_RenderClear(state->renderer);
                
                SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 255);
                vec2_t lerped_pos = vec2_lerp(state->player->last_position, state->player->position, alpha);
                SDL_FRect rect = {
                    .x = lerped_pos.x,
                    .y = lerped_pos.y,
                    .w = 60,
                    .h = 60
                };
                SDL_RenderFillRect(state->renderer, &rect);
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
