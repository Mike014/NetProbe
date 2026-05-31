add_executable(server server.cpp)
add_executable(client client.cpp)

if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
endif()

target_compile_options(server PRIVATE -Wall -Wextra -O2)
target_compile_options(client PRIVATE -Wall -Wextra -O2)