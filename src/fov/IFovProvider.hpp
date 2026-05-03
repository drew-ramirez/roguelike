#pragma once

#include <vector>

#include "core/Point.hpp"

namespace rl {

class Map;

class IFovProvider {
  public:
    virtual ~IFovProvider() = default;

    [[nodiscard]] virtual std::vector<Point>
    compute(const Map& map, Point origin, int range) const = 0;
};

} // namespace rl
