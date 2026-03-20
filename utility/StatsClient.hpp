#pragma once

#include <string>
#include <vector>

struct StatEntry {
    std::string name;
    std::string value;
};

class StatsClient {
public:
    StatsClient(std::string request_fifo_path, std::string response_fifo_path);
    ~StatsClient();

    StatsClient(const StatsClient&) = delete;
    StatsClient& operator=(const StatsClient&) = delete;

    std::vector<StatEntry> fetch_stats();
private:
    std::string request_fifo_path_;
    std::string response_fifo_path_;
};