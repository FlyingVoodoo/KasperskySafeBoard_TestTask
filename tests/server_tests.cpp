#include <gtest/gtest.h>

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "Scanner.hpp"

namespace {
std::filesystem::path write_temp_config(const std::string& body, const std::string& suffix) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("scanner_test_" + std::to_string(getpid()) + suffix);
    std::ofstream out(path);
    out << body;
    out.close();
    return path;
}
}  // namespace

TEST(ServerScannerTests, LoadsPatternsAndDetectsThreats) {
    const auto path = write_temp_config(
        R"({"patterns":["trojan","virus","malware"]})",
        "_valid.json");

    Scanner scanner(path.string());
    const ScanResult result = scanner.scan("safe text with trojan and virus");

    std::filesystem::remove(path);

    EXPECT_TRUE(result.infected);
    EXPECT_EQ(result.matched_indices.size(), 2u);
    EXPECT_EQ(scanner.get_patterns().size(), 3u);
}

TEST(ServerScannerTests, ThrowsOnInvalidJson) {
    const auto path = write_temp_config("{invalid_json}", "_invalid.json");

    EXPECT_THROW({
        Scanner scanner(path.string());
        (void)scanner;
    }, std::runtime_error);

    std::filesystem::remove(path);
}
