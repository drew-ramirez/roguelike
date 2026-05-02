#pragma once

#include <vector>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/Rect.hpp"

namespace rl {

struct MapBuildResult {
    Map map;
    Point playerStart;
    std::vector<Rect> rooms;
};

} // namespace rl
