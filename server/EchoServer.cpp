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
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <vector>

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

EchoServer::EchoServer(int port, const std::string& config_path)
    : server_socket_(socket(AF_INET, SOCK_STREAM, 0)), port_(port), config_path_(config_path), stats_storage_(), scanner_(config_path) {

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

    sync_pattern_stats();
}

void EchoServer::start() {
    std::cout << "Server started on port " << port_ << "..." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;

    const char* fifo_path = "/tmp/echo_server_fifo";
    mkfifo(fifo_path, 0666);

    int fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK);
    if (fifo_fd < 0) {
        throw std::runtime_error("Failed to open FIFO: " + std::string(std::strerror(errno)));
    }

    std::vector<pid_t> child_pids;


    while (g_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        int listener_fd = server_socket_.get_fd();
        FD_SET(listener_fd, &read_fds);

        FD_SET(fifo_fd, &read_fds);

        int max_fd = std::max(listener_fd, fifo_fd);


        timeval timeout{1, 0};

        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Select failed: " << std::strerror(errno) << std::endl;
            continue;
        }

        if (ready == 0)
            continue;

        
         if (FD_ISSET(fifo_fd, &read_fds))
            process_fifo_request(fifo_fd);

        if (FD_ISSET(listener_fd, &read_fds)) {
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
                close(listener_fd);
                handle_client(std::move(client_socket));
                _exit(0);
            }

            child_pids.push_back(pid);
        }
    }

    for (pid_t pid : child_pids) {
        if (pid <= 0) {
            continue;
        }
        kill(pid, SIGTERM);
    }

    while (true) {
        const pid_t waited = waitpid(-1, nullptr, 0);
        if (waited > 0) {
            continue;
        }
        if (waited < 0 && errno == EINTR) {
            continue;
        }
        if (waited < 0 && errno == ECHILD) {
            break;
        }
        break;
    }

    close(fifo_fd);
    unlink(fifo_path);
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
    std::string file_content;
    char buffer[4096];

    while (true) {
        ssize_t bytes_read = recv(client_socket.get_fd(), buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (bytes_read < 0 && errno == EINTR) {
                continue;
            }
            break;
        }
        file_content.append(buffer, static_cast<size_t>(bytes_read));
    }
    auto stats = stats_storage_.get_stats();
    stats->files_checked.fetch_add(1, std::memory_order_relaxed);

    ScanResult result = scanner_.scan(file_content);

    std::string response;
    if (result.infected) {
        stats->threats_detected.fetch_add(1, std::memory_order_relaxed);
        
        response = "Infected:";
        for (size_t index : result.matched_indices) {
            if (index < kMaxPatterns) {
                stats->pattern_stats[index].count.fetch_add(1, std::memory_order_relaxed);
            }
            response += " " + scanner_.get_patterns()[index];
        }
    } else {
        response = "Clean";
    }
    response += "\n";

    send_all(client_socket.get_fd(), response.c_str(), response.size());
}

void EchoServer::sync_pattern_stats() {
    auto* stats = stats_storage_.get_stats();
    const auto& loaded_patterns = scanner_.get_patterns();

    stats->patterns_loaded = loaded_patterns.size();

    for (size_t i = 0; i < loaded_patterns.size() && i < kMaxPatterns; ++i) {
        std::strncpy(stats->pattern_stats[i].name, loaded_patterns[i].c_str(), kMaxPatternNameLength - 1);
        stats->pattern_stats[i].name[kMaxPatternNameLength - 1] = '\0';
        stats->pattern_stats[i].count = 0;
    }
}

void EchoServer::process_fifo_request(int fd) {
    auto write_response = [&](const std::string& response) {
        int write_fd = open("/tmp/echo_server_fifo", O_WRONLY | O_NONBLOCK);
        if (write_fd < 0) {
            return;
        }

        size_t written_total = 0;
        while (written_total < response.size()) {
            const ssize_t written = write(write_fd, response.c_str() + written_total, response.size() - written_total);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
            if (written == 0) {
                break;
            }
            written_total += static_cast<size_t>(written);
        }

        close(write_fd);
    };

    char buff[128]{};
    const ssize_t bytes_read = read(fd, buff, sizeof(buff) - 1);
    if (bytes_read < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        write_response("ERROR read_failed\n");
        return;
    }

    if (bytes_read == 0) {
        return;
    }

    buff[bytes_read] = '\0';
    std::string request(buff);
    request.erase(std::remove(request.begin(), request.end(), '\r'), request.end());
    request.erase(std::remove(request.begin(), request.end(), '\n'), request.end());

    if (request != "GET_STATS") {
        write_response("ERROR unknown_request\n");
        return;
    }

    auto* stats = stats_storage_.get_stats();

    std::string response = "OK\n";
    response += "files_checked=" + std::to_string(stats->files_checked.load()) + "\n";
    response += "threats_detected=" + std::to_string(stats->threats_detected.load()) + "\n";
    response += "patterns_loaded=" + std::to_string(stats->patterns_loaded.load()) + "\n";

    const size_t patterns_loaded = static_cast<size_t>(stats->patterns_loaded.load());
    for (size_t i = 0; i < patterns_loaded && i < kMaxPatterns; ++i) {
        response += std::string(stats->pattern_stats[i].name) + "=" + std::to_string(stats->pattern_stats[i].count.load()) + "\n";
    }

    write_response(response);
}