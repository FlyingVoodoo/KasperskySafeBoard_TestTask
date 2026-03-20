#pragma once

#include <cstddef>

#include "../include/SharedMemory.hpp"

class Socket {
public:
    explicit Socket(int socket_fd);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    ~Socket();

    int get_fd() const;

private:
    int fd_;
};

class EchoServer {
public:
    explicit EchoServer(int port);

    EchoServer(const EchoServer&) = delete;
    EchoServer& operator=(const EchoServer&) = delete;

    void start();

private:
    bool send_all(int socket_fd, const char* data, size_t total_bytes);
    void handle_client(Socket client_socket);

    Socket server_socket_;
    int port_;
    SharedMemory stats_storage_;
};

void configure_server_signals();
