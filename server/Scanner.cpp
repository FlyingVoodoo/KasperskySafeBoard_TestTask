#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "Scanner.hpp"
#include "SharedTypes.hpp"

using json = nlohmann::json;

Scanner::Scanner(const std::string& patterns_file) {
    load_config(patterns_file);
}

void Scanner::load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open patterns file: " + path);
    }

    json data;
    try {
        file >> data;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }

    patterns_.clear();
    if (data.contains("patterns") && data["patterns"].is_array()) {
        for (const auto& pattern : data["patterns"]) {
            if (patterns_.size() >= kMaxPatterns) {
                throw std::runtime_error("Exceeded maximum number of patterns: " + std::to_string(kMaxPatterns));
            }
            if (pattern.is_string() && !pattern.get<std::string>().empty()) {
                patterns_.push_back(pattern.get<std::string>());
            }
        }
    } else {
        throw std::runtime_error("Invalid JSON format: 'patterns' array is missing or not an array");
    }
}

ScanResult Scanner::scan(std::string_view data) const {
    ScanResult result;
    result.infected = false;

    for (size_t i = 0; i < patterns_.size(); ++i) {
        if (data.find(patterns_[i]) != std::string_view::npos) {
            result.infected = true;
            result.matched_indices.push_back(i);
        }
    }
    return result;
}