#include <atomic>
#include <cstddef>
#include <cstdint>

constexpr size_t kMaxPatterns = 100;
constexpr size_t kMaxPatternNameLength = 64;

struct PatternStat {
    char name[kMaxPatternNameLength]{};
    std::atomic<uint64_t> count{0};
};

struct SharedStats {
    std::atomic<uint64_t> files_checked{0};
    std::atomic<uint64_t> threats_detected{0};
    std::atomic<uint64_t> patterns_loaded{0};
    PatternStat pattern_stats[kMaxPatterns]{};
};