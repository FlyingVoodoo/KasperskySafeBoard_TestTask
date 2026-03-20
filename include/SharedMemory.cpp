#include "SharedMemory.hpp"

#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <sys/mman.h>

SharedMemory::SharedMemory() : stats(nullptr) {
    stats = static_cast<SharedStats*>(mmap(nullptr, sizeof(SharedStats), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    if (stats == MAP_FAILED) {
        throw std::runtime_error("Failed to create shared memory: " + std::string(std::strerror(errno)));
    }
    new (stats) SharedStats();
}

SharedMemory::~SharedMemory() {
    if (stats) {
        munmap(stats, sizeof(SharedStats));
    }
}

SharedStats* SharedMemory::get_stats() {
    return stats;
}

const SharedStats* SharedMemory::get_stats() const {
    return stats;
}

