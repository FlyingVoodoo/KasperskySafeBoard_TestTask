#pragma once
#include <string>
#include <vector>
#include <string_view>

struct ScanResult {
    bool infected = false;
    std::vector<size_t> matched_indices;
};

class Scanner {
public:
    explicit Scanner(const std::string& patterns_file);
    void load_config(const  std::string& path);
    ScanResult scan(std::string_view data) const;

    const std::vector<std::string>& get_patterns() const {
        return patterns_;
    }

private:
    std::vector<std::string> patterns_;
};