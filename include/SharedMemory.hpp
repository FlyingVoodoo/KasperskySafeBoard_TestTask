#pragma once

#include "SharedTypes.hpp"

class SharedMemory {
private:
    SharedStats* stats;

public:
    SharedMemory();
    ~SharedMemory();

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedStats* get_stats();
    const SharedStats* get_stats() const;
};
