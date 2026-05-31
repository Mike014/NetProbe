/*
 *  A client sending a message to a server and receiving it back multiple times.
 *  Usage: ./client -a <address> -p <port> -b <message_size (bytes)>
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

    socket_t sockfd;
    struct Config config = get_config(argc, argv);
    std::vector<uint8_t> buffer(config.n_bytes);
    struct sockaddr_in serv_addr;

    // Fill buffer with dummy data
    for (size_t i = 0; i < config.n_bytes; i++)
        buffer[i] = static_cast<uint8_t>(i & 0xFF);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (!SOCKET_VALID(sockfd))
        panic("ERROR opening socket");

    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(config.port);

    if (inet_pton(AF_INET, config.address.c_str(), &serv_addr.sin_addr) <= 0)
    {
        struct hostent *he = gethostbyname(config.address.c_str());
        if (he == nullptr)
            panic("ERROR resolving hostname");
        std::memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        panic("ERROR connecting to server");

    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(int));

    std::cout << std::format("Connected to {}:{}, starting send-receive loop\n",
                             config.address, config.port) << std::flush;

    // Send-receive loop
    for (int i = 0; i < N_ROUNDS; i++)
    {
        send_message(config.n_bytes, sockfd, buffer.data());
        receive_message(config.n_bytes, sockfd, buffer.data());
    }

    std::cout << "Done!\n";

    CLOSE_SOCKET(sockfd);
    WSACleanup();

    return 0;
}
