#include "StatsClient.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <utility>

StatsClient::StatsClient(std::string request_fifo_path, std::string response_fifo_path)
    : request_fifo_path_(std::move(request_fifo_path)), response_fifo_path_(std::move(response_fifo_path)) {}

StatsClient::~StatsClient() = default;

std::vector<StatEntry> StatsClient::fetch_stats() {
    std::vector<StatEntry> results;

    int read_fd = open(response_fifo_path_.c_str(), O_RDONLY | O_NONBLOCK);
    if (read_fd < 0) {
        std::cerr << "Failed to open response FIFO for reading: " << std::strerror(errno) << std::endl;
        return results;
    }

    int write_fd = open(request_fifo_path_.c_str(), O_WRONLY | O_NONBLOCK);
    if (write_fd < 0) {
        std::cerr << "Failed to open request FIFO for writing: " << std::strerror(errno) << std::endl;
        close(read_fd);
        return results;
    }

    const std::string cmd = "GET_STATS\n";
    const ssize_t write_result = write(write_fd, cmd.c_str(), cmd.size());
    close(write_fd);
    if (write_result < 0) {
        std::cerr << "Failed to write request: " << std::strerror(errno) << std::endl;
        close(read_fd);
        return results;
    }

    std::string payload;
    payload.reserve(4096);

    for (int attempt = 0; attempt < 100; ++attempt) {
        char buffer[1024];
        const ssize_t bytes = read(read_fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            payload.append(buffer, static_cast<size_t>(bytes));
            if (payload.find("\nEND\n") != std::string::npos ||
                (payload.size() >= 4 && payload.compare(payload.size() - 4, 4, "END\n") == 0)) {
                break;
            }
            continue;
        }

        if (bytes == 0) {
            if (!payload.empty()) {
                break;
            }
            usleep(20 * 1000);
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            usleep(20 * 1000);
            continue;
        }

        std::cerr << "Failed to read response: " << std::strerror(errno) << std::endl;
        close(read_fd);
        return results;
    }

    close(read_fd);

    if (payload.empty()) {
        return results;
    }

    std::stringstream ss(payload);
    std::string line;
    while (std::getline(ss, line)) {
        if (line == "OK" || line == "END" || line.empty()) continue;

        if (line.rfind("ERROR", 0) == 0) {
            std::cerr << "Server error: " << line << std::endl;
            results.clear();
            return results;
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            results.push_back({line.substr(0, pos), line.substr(pos + 1)});
        }
    }

    return results;
}