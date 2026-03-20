#include "StatsClient.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <string>

StatsClient::StatsClient(const std::string& fifo_path) : fifo_path_(fifo_path) {}

StatsClient::~StatsClient() = default;

std::vector<StatEntry> StatsClient::fetch_stats() {
    std::vector<StatEntry> results;

    int write_fd = open(fifo_path_.c_str(), O_WRONLY | O_NONBLOCK);
    if (write_fd < 0) {
        std::cerr << "Failed to open FIFO for writing: " << std::strerror(errno) << std::endl;
        return results;
    }

    const std::string cmd = "GET_STATS\n";
    const ssize_t write_result = write(write_fd, cmd.c_str(), cmd.size());
    close(write_fd);
    if (write_result < 0) {
        std::cerr << "Failed to write request: " << std::strerror(errno) << std::endl;
        return results;
    }

    int read_fd = open(fifo_path_.c_str(), O_RDONLY | O_NONBLOCK);
    if (read_fd < 0) {
        std::cerr << "Failed to open FIFO for reading: " << std::strerror(errno) << std::endl;
        return results;
    }

    char buffer[4096];
    ssize_t bytes = read(read_fd, buffer, sizeof(buffer) - 1);
    close(read_fd);

    if (bytes <= 0) {
        return results;
    }

    buffer[bytes] = '\0';

    std::stringstream ss(buffer);
    std::string line;
    while (std::getline(ss, line)) {
        if (line == "OK" || line.empty()) continue;

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            results.push_back({line.substr(0, pos), line.substr(pos + 1)});
        }
    }

    return results;
}