#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "../client/ScannerClient.hpp"

namespace {
int create_listener(uint16_t& port) {
    const int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        throw std::runtime_error("socket failed");
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);

    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(listener);
        throw std::runtime_error("bind failed");
    }

    socklen_t len = sizeof(address);
    if (getsockname(listener, reinterpret_cast<sockaddr*>(&address), &len) < 0) {
        close(listener);
        throw std::runtime_error("getsockname failed");
    }
    port = ntohs(address.sin_port);

    if (listen(listener, 1) < 0) {
        close(listener);
        throw std::runtime_error("listen failed");
    }

    return listener;
}

std::filesystem::path make_temp_file(const std::string& content) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("client_test_" + std::to_string(getpid()) + "_" + std::to_string(::time(nullptr)) + ".txt");
    std::ofstream out(path, std::ios::binary);
    out << content;
    out.close();
    return path;
}
}  // namespace

TEST(ScannerClientTests, SendsFilePayloadAndReturnsServerResponse) {
    const std::string payload = "sample payload with trojan marker";
    const std::string expected_response = "Clean\n";

    uint16_t port = 0;
    const int listener = create_listener(port);

    std::string received;
    std::thread fake_server([&]() {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(listener, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        ASSERT_GE(client_fd, 0);

        char buffer[1024];
        while (true) {
            const ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                break;
            }
            received.append(buffer, static_cast<size_t>(bytes));
        }

        const ssize_t sent = send(client_fd, expected_response.data(), expected_response.size(), 0);
        ASSERT_EQ(sent, static_cast<ssize_t>(expected_response.size()));

        close(client_fd);
        close(listener);
    });

    const auto temp_file = make_temp_file(payload);
    ScannerClient client(static_cast<int>(port));
    const std::string response = client.scan_file(temp_file.string());

    fake_server.join();
    std::filesystem::remove(temp_file);

    EXPECT_EQ(response, expected_response);
    EXPECT_EQ(received, payload);
}

TEST(ScannerClientTests, ThrowsOnMissingFile) {
    ScannerClient client(6553);
    EXPECT_THROW(client.scan_file("/tmp/definitely_missing_file_123456.txt"), std::runtime_error);
}
