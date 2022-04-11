cmake_minimum_required(VERSION 3.17)
project(so2_2022_tp2 C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Pedantic -Werror -Wextra -Wconversion -std=gnu11")
# sudo dnf install glibc-devel
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# sudo dnf install sqlite-devel
find_package(SQLite3 REQUIRED)
set(CMAKE_CXX_CPPCHECK "cppcheck")

add_executable(server_tp2 server_tp2.c io_ops_common.c)
add_executable(tcp4_client tcp4_client.c clients_common.c clients_common.h)
add_executable(tcp6_client tcp6_client.c clients_common.c clients_common.h)
add_executable(unix_client unix_client.c clients_common.c clients_common.h)
add_executable(test_client test_client.c io_ops_common.c)
add_executable(test_server test_server.c io_ops_common.c)

target_link_libraries(server_tp2 PRIVATE Threads::Threads)