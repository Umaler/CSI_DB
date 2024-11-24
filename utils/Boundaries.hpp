#pragma once

#include <limits>

struct Boundary {
    int64_t min = 0;
    int64_t max = std::numeric_limits<int64_t>::max();
};

struct Boundaries {
    Boundary id;
    Boundary packId;
    Boundary measId;
    Boundary numSub {0, 0};
};
