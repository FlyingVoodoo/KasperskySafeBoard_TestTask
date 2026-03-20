#include <atomic>
#include <cstdint>

struct SharedStats {
    std::atomic<uint64_t> files_checked{0};
    std::atomic<uint64_t> threats_detected{0};
};