/**
 * **************************************************************************
 * @file     IFovProvider.hpp
 * @brief    Defines the engine boundary for field-of-view implementations
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "core/Point.hpp"

namespace rl {

class Map;

class IFovProvider {
  public:
    virtual ~IFovProvider() = default;

    /**
     * Computes every map cell visible from an origin point
     *
     * @param map The map to query for sight blockers
     * @param origin The viewer's current tile
     * @param range The maximum sight radius in tiles
     * @return Visible tile positions, including the origin when valid
     */
    [[nodiscard]] virtual std::vector<Point>
    compute(const Map& map, Point origin, int range) const = 0;
};

} // namespace rl
