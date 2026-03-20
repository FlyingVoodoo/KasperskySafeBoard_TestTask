#include "EchoServer.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
volatile sig_atomic_t g_running = 1;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = 0;
    }
}

void reap_children(int) {
    int saved_errno = errno;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }
    errno = saved_errno;
}
} // namespace

void configure_server_signals() {
    struct sigaction sa_chld {};
    sa_chld.sa_handler = reap_children;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, nullptr) < 0) {
        throw std::runtime_error("sigaction(SIGCHLD) failed");
    }

    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
}

Socket::Socket(int socket_fd) : fd_(socket_fd) {
    if (fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) {
            close(fd_);
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

int Socket::get_fd() const {
    return fd_;
}

EchoServer::EchoServer(int port)
    : server_socket_(socket(AF_INET, SOCK_STREAM, 0)), port_(port), stats_storage_() {

    int opt = 1;
    if (setsockopt(server_socket_.get_fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_socket_.get_fd(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_socket_.get_fd(), SOMAXCONN) < 0) {
        throw std::runtime_error("Listen failed");
    }
}

void EchoServer::start() {
    std::cout << "Server started on port " << port_ << "..." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket_.get_fd(), &read_fds);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(server_socket_.get_fd() + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Select failed: " << std::strerror(errno) << std::endl;
            continue;
        }

        if (ready == 0) {
            continue;
        }

        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_socket_.get_fd(), reinterpret_cast<sockaddr*>(&client_address), &client_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Accept failed: " << std::strerror(errno) << std::endl;
            continue;
        }

        std::cout << "Client connected!" << std::endl;
        Socket client_socket(client_fd);
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed: " << std::strerror(errno) << std::endl;
            continue;
        }

        if (pid == 0) {
            close(server_socket_.get_fd());
            handle_client(std::move(client_socket));
            _exit(0);
        }
    }

    while (true) {
        pid_t waited = waitpid(-1, nullptr, 0);
        if (waited > 0) {
            continue;
        }
        if (waited < 0 && errno == EINTR) {
            continue;
        }
        break;
    }

    std::cout << "Server shutdown requested." << std::endl;
}

bool EchoServer::send_all(int socket_fd, const char* data, size_t total_bytes) {
    size_t sent_total = 0;
    while (sent_total < total_bytes) {
        ssize_t sent = send(socket_fd, data + sent_total, total_bytes - sent_total, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (sent == 0) {
            return false;
        }
        sent_total += static_cast<size_t>(sent);
    }
    return true;
}

void EchoServer::handle_client(Socket client_socket) {
    char buffer[1024];
    while (true) {
        ssize_t bytes_read = recv(client_socket.get_fd(), buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Receive failed: " << std::strerror(errno) << std::endl;
            break;
        }

        if (bytes_read == 0) {
            break;
        }

        if (!send_all(client_socket.get_fd(), buffer, static_cast<size_t>(bytes_read))) {
            std::cerr << "Send failed: " << std::strerror(errno) << std::endl;
            break;
        }
    }

    std::cout << "Client disconnected." << std::endl;
}
