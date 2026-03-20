cmake_minimum_required(VERSION 3.16)
project(KasperskySafeBoard_TestTask LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(shared STATIC
    include/SharedMemory.cpp
    server/Scanner.cpp
)

target_include_directories(shared PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/server)

add_executable(server_app
    server/main.cpp
    server/EchoServer.cpp
)
target_include_directories(server_app PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/server)
target_link_libraries(server_app PRIVATE shared)

add_executable(client_app
    client/client_main.cpp
    client/ScannerClient.cpp
)
target_include_directories(client_app PRIVATE ${CMAKE_SOURCE_DIR}/client)

add_executable(stats_utility
    utility/stats_main.cpp
    utility/StatsClient.cpp
)
target_include_directories(stats_utility PRIVATE ${CMAKE_SOURCE_DIR}/utility)

