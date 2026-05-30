/*
 *  A server receiving and sending back a message multiple times.
 *  Usage: ./server.out -p <port> -n <message_size (bytes)>
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <format>
#include "connection.hpp"

int main(int argc, char *argv[])
{
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
        panic("WSAStartup failed");

    SOCKET sockfd, newsockfd;
    struct Config config = get_config(argc, argv);
    std::vector<uint8_t> buffer(config.n_bytes);
    struct sockaddr_in serv_addr, cli_addr;
    
    // Create listening socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
        panic("ERROR opening socket");
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(config.port);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        panic("ERROR on binding");
    std::cout << std::format("Server ready, listening on port {}\n", config.port) << std::flush;
    listen(sockfd, 5);
    int clilen = sizeof(cli_addr);
    
    // Accept connection and set nonblocking and nodelay
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd == INVALID_SOCKET)
        panic("ERROR on accept");
    u_long mode = 1;
    ioctlsocket(newsockfd, FIONBIO, &mode);
    int flag = 1;
    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(int));
     
    // Receive-send loop
    std::cout << "Connection accepted, ready to recevive!\n"
              << std::endl;
    for (int i = 0; i < N_ROUNDS; i++)
    {
        receive_message(config.n_bytes, newsockfd, buffer.data());
        send_message(config.n_bytes, newsockfd, buffer.data());
    }

    std::cout << "Done!\n"
              << std::endl;
    
    // Clean state          
    closesocket(sockfd);
    closesocket(newsockfd);

    WSACleanup();
}