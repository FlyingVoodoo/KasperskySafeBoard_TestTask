#pragma once

#include <string>

#include "SharedMemory.hpp"
#include "Scanner.hpp"

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

class Server {
public:
    explicit Server(int port, const std::string& config_path = "patterns.json");

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();

private:
    bool send_all(int socket_fd, const char* data, size_t total_bytes);
    void handle_client(Socket client_socket);
    void sync_pattern_stats();
    void process_fifo_request(int fd);

    Socket server_socket_;
    int port_;
    SharedMemory stats_storage_;
    std::string config_path_;
    Scanner scanner_;
};

void configure_server_signals();
