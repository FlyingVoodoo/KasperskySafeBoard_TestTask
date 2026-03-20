#pragma once

#include <string>
#include <vector>

struct StatEntry {
    std::string name;
    std::string value;
};

class StatsClient {
public:
    explicit StatsClient(const std::string& fifo_path);
    ~StatsClient();

    StatsClient(const StatsClient&) = delete;
    StatsClient& operator=(const StatsClient&) = delete;

    std::vector<StatEntry> fetch_stats();
private:
    std::string fifo_path_;
};