add_executable(server server.cpp)
target_link_libraries(server PRIVATE ws2_32)
target_compile_options(server PRIVATE -Wall -Wextra -O2)

add_executable(client client.cpp)
target_link_libraries(client PRIVATE ws2_32)
target_compile_options(client PRIVATE -Wall -Wextra -O2)