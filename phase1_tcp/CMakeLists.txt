add_executable(tcp_probe main.cpp)
target_link_libraries(tcp_probe PRIVATE ws2_32)
target_compile_options(tcp_probe PRIVATE -Wall -Wextra -O2)