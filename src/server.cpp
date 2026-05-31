/*
 *  A server receiving and sending back a message multiple times.
 *  Usage: ./server -p <port> -b <message_size (bytes)>
 */
 
#include <iostream>
#include <format>
#include <cstdlib>
#include <cstring>
#include "connection.hpp"
 
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        panic("WSAStartup failed");
 
    socket_t sockfd, newsockfd;
    struct Config config = get_config(argc, argv);
    std::vector<uint8_t> buffer(config.n_bytes);
    struct sockaddr_in serv_addr, cli_addr;
 
    // Create listening socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (!SOCKET_VALID(sockfd))
        panic("ERROR opening socket");
 
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(config.port);
 
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        panic("ERROR on binding");
 
    std::cout << std::format("Server ready, listening on port {}\n", config.port) << std::flush;
    listen(sockfd, 5);
    socklen_t clilen = sizeof(cli_addr);
 
    // Accept connection and set nonblocking and nodelay
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (!SOCKET_VALID(newsockfd))
        panic("ERROR on accept");
 
    u_long mode = 1;
    ioctlsocket(newsockfd, FIONBIO, &mode);
    int flag = 1;
    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(int));
 
    // Receive-send loop
    std::cout << "Connection accepted, ready to receive!\n" << std::flush;
    for (int i = 0; i < N_ROUNDS; i++)
    {
        receive_message(config.n_bytes, newsockfd, buffer.data());
        send_message(config.n_bytes, newsockfd, buffer.data());
    }
 
    std::cout << "Done!\n";
 
    CLOSE_SOCKET(sockfd);
    CLOSE_SOCKET(newsockfd);
    WSACleanup();
 
    return 0;
}