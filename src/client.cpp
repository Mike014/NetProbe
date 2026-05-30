/*
 *  A client timing the roundtrip time of a message sent to a server multiple times.
 *  Usage: ./client.out -a <address> -p <port> -b <message_size (bytes)>
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <format>
#include <vector>
#include "connection.hpp"

int main(int argc, char *argv[])
{
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
        panic("WSAStartup failed");
        
    SOCKET sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    struct Config config = get_config(argc, argv);

    // Init buffers
    std::vector<uint8_t> rbuffer(config.n_bytes);
    std::vector<uint8_t> wbuffer(config.n_bytes);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
        panic("ERROR opening socket");
    server = gethostbyname(config.address.c_str());
    if (server == NULL)
    {
        std::cerr << "ERROR, no such host\n";
        std::exit(EXIT_FAILURE);
    }
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(config.port);

    // Connect and set nonblocking and nodelay
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        panic("ERROR connecting");
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(int));
    std::cout << "Connection successful! Starting...\n"
              << std::flush;

    // Timed send-receive loop
    std::vector<uint64_t> times_send(N_ROUNDS);
    std::vector<uint64_t> times_recv(N_ROUNDS);
    for (size_t i = 0; i < N_ROUNDS; i++)
    {

        uint64_t tstart = rdtscp();

        send_message(config.n_bytes, sockfd, wbuffer.data());
        uint64_t tsend = rdtsc();
        receive_message(config.n_bytes, sockfd, rbuffer.data());

        uint64_t tend = rdtsc();

        times_send[i] = tsend - tstart;
        times_recv[i] = tend - tsend;
    }

    closesocket(sockfd);
    std::cout << "Done!\nSummary: (time_send,\ttime_recv)\n";
    for (size_t i = 0; i < N_ROUNDS; i++)
    {
        std::cout << std::format("({},\t{})\n", times_send[i], times_recv[i]);
    }

    WSACleanup();
    return 0;
}
