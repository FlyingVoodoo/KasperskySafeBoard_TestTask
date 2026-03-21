#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ctime>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "StatsClient.hpp"

namespace {
std::string fifo_path(const std::string& prefix) {
    return (std::filesystem::temp_directory_path() /
            (prefix + "_" + std::to_string(getpid()) + "_" + std::to_string(::time(nullptr)) + ".fifo"))
        .string();
}
}  // namespace

TEST(StatsClientTests, FetchesStatsFromFifoServer) {
    const std::string request_fifo = fifo_path("req");
    const std::string response_fifo = fifo_path("resp");

    ASSERT_EQ(mkfifo(request_fifo.c_str(), 0666), 0);
    ASSERT_EQ(mkfifo(response_fifo.c_str(), 0666), 0);

    std::thread fake_server([&]() {
        const int request_fd = open(request_fifo.c_str(), O_RDONLY);
        ASSERT_GE(request_fd, 0);

        char buffer[64]{};
        const ssize_t bytes = read(request_fd, buffer, sizeof(buffer) - 1);
        ASSERT_GT(bytes, 0);
        close(request_fd);

        const int response_fd = open(response_fifo.c_str(), O_WRONLY);
        ASSERT_GE(response_fd, 0);

        const std::string response =
            "OK\n"
            "files_checked=5\n"
            "threats_detected=2\n"
            "patterns_loaded=2\n"
            "trojan=1\n"
            "virus=1\n"
            "END\n";

        const ssize_t sent = write(response_fd, response.c_str(), response.size());
        ASSERT_EQ(sent, static_cast<ssize_t>(response.size()));
        close(response_fd);
    });

    StatsClient client(request_fifo, response_fifo);
    const std::vector<StatEntry> stats = client.fetch_stats();

    fake_server.join();

    std::filesystem::remove(request_fifo);
    std::filesystem::remove(response_fifo);

    ASSERT_FALSE(stats.empty());
    EXPECT_EQ(stats[0].name, "files_checked");
    EXPECT_EQ(stats[0].value, "5");
}
