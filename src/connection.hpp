#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <winsock2.h>
#include <format>
#include <cstring>

constexpr int DEFAULT_N_BYTES = 1;
constexpr int DEFAULT_PORT = 8080;
constexpr int N_ROUNDS = 1000000;
constexpr const char *DEFAULT_ADDRESS = "127.0.0.1";

struct Config
{
    std::string address;
    uint16_t port;
    uint32_t n_bytes;
};

void print_config(const Config &config) noexcept
{
    std::cout << std::format("Address: {}, Port: {}, N_bytes: {}\n",
                             config.address, config.port, config.n_bytes);
}

struct Config get_config(int argc, char* argv[])
{
    struct Config config;
    config.n_bytes = DEFAULT_N_BYTES;
    config.port = DEFAULT_PORT;
    config.address = DEFAULT_ADDRESS;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-a" && i + 1 < argc)
            config.address = argv[++i];
        else if (arg == "-p" && i + 1 < argc)
            config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        else if (arg == "-b" && i + 1 < argc)
            config.n_bytes = static_cast<uint32_t>(std::stoul(argv[++i]));
        else
        {
            std::cerr << "Usage: " << argv[0] << " [-b bytes] [-a address] [-p port]\n";
            std::exit(EXIT_FAILURE);
        }
    }
    print_config(config);
    return config;
}

[[noreturn]] void panic(const std::string &msg)
{
    std::cerr << msg << ": " << std::strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

[[nodiscard]] uint64_t rdtsc() noexcept
{
#ifdef _MSC_VER
    return _rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#endif
}

[[nodiscard]] uint64_t rdtscp() noexcept
{
#ifdef _MSC_VER
    unsigned int ui;
    return __rdtscp(&ui);
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#endif
}

int receive_message(size_t n_bytes, SOCKET sockfd, uint8_t *buffer)
{
    int bytes_read = 0;
    int r;

    while (bytes_read < n_bytes)
    {
        r = recv(sockfd, reinterpret_cast<char *>(buffer + bytes_read), n_bytes - bytes_read, 0);
        if (r == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            panic("ERROR reading from socket");
        }
        if (r > 0)
        {
            bytes_read += r;
        }
    }

    return bytes_read;
}

int send_message(size_t n_bytes, SOCKET sockfd, uint8_t *buffer)
{
    int bytes_sent = 0;
    int r;
    while (bytes_sent < n_bytes)
    {
        // Make sure we write exactly n_bytes
        r = send(sockfd, reinterpret_cast<const char *>(buffer + bytes_sent), n_bytes - bytes_sent, 0);
        if (r == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            panic("ERROR writing to socket");
        }
        if (r > 0)
        {
            bytes_sent += r;
        }
    }
    return bytes_sent;
}