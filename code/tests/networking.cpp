/* ========================================================================
   $File: networking.cpp $
   $Date: December 03 2025 03:25 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>

#include <c_dynarray.h>

#include <p_platform_data.h>
#include <p_platform_data.cpp>

#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_memory_arena.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_zone_allocator.cpp>

#if OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/mman.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> 
#include <unistd.h>
#include <dlfcn.h>
#include <string.h> 
#include <poll.h>

#endif


/* NOTE(Sleepster): Client to server
 * - [ ] When we create a server we'll use the server owner's ipv6 address and have them bind to port '27015'
 * - [ ] Have them supply thier IP to that of their friend, have the server connect to that IP on port '27015'
 * */

struct network_data_t
{
    s32              socket_id;
    struct addrinfo *info;
};
#define MAX_CLIENTS (10)

internal_api network_data_t 
init_network_data(char *ip_addr, char *port, bool8 client)
{
    network_data_t result = {};

    int status;
    struct addrinfo hints;

    u32 ai_flags = client ? 0 : AI_PASSIVE;

    memset(&hints, 0, sizeof hints); 
    hints.ai_family   = AF_INET6;     
    hints.ai_socktype = SOCK_STREAM;     
    hints.ai_flags    = ai_flags;

    if((status = getaddrinfo(ip_addr, port, &hints, &result.info)) != 0) 
    {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        Assert(false);
    }

    result.socket_id = socket(result.info->ai_family, result.info->ai_socktype, result.info->ai_protocol);
    Expect(result.socket_id != -1, "Socket creation failed: '%d'...\n", errno);

    return(result);
}

internal_api void
client(char *ip_addr, char *port)
{
    network_data_t data = init_network_data(ip_addr, port, true);

    s32 result = connect(data.socket_id, data.info->ai_addr, data.info->ai_addrlen);
    s32 error  = errno;
    Expect(result != -1, "failed to connect to socket: '%d'...\n", error);

    string_t message = STR("I am hither");
    s32 sent = send(data.socket_id, (const char*)message.data, (s32)message.count, 0);
    Expect(sent != -1, "Failed to send bytes down network pipe: '%d'...\n", errno);

    Expect(closesocket(data.socket_id), "Failed to close socket, '%d'...\n", errno);
}

internal_api void
server()
{
    network_data_t data = init_network_data(null, "27015", false);

    s32 result = bind(data.socket_id, data.info->ai_addr, data.info->ai_addrlen);
    Expect(result != -1, "Failed to bind the socket: '%d'...\n", errno);

    result = listen(data.socket_id, MAX_CLIENTS);
    Expect(result != -1, "Failed to listen to the socket: '%d'...\n", errno);

    s32 user_socket = -1;
    struct sockaddr_storage user_addr = {};
    socklen_t addr_size = sizeof(addrinfo);

    user_socket = accept(data.socket_id, (struct sockaddr *)&user_addr, &addr_size);
    Expect(user_socket != -1, "Failure to accept user connection: '%d'...\n", errno);

    printf("client connected...\n");

    char buffer[4096] = {};
    s32 received = recv(user_socket, buffer, 4096, 0);
    Expect(received != -1 || received != 0, "Failed to receive in '%d'...\n", errno);

    printf("message: '%s'...\n", buffer);

    Expect(closesocket(user_socket), "Failed to close socket, '%d'..\n", errno);
    Expect(closesocket(data.socket_id), "Failed to close socket, '%d'..\n", errno);
}

int
main(int argc, char **argv)
{
#if OS_WINDOWS
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        fprintf(stderr, "WSAStartup failed.\n");
    }

    if(LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2)
    {
        fprintf(stderr,"Version 2.2 of Winsock not available.\n");
        WSACleanup();
    }
#endif

    if(argc >= 2)
    {
        if(strcmp(argv[1], "client") == 0)
        {
            client("::1", "27015");
        }
        else if(strcmp(argv[1], "server") == 0)
        {
            server();
        }
    }
}
