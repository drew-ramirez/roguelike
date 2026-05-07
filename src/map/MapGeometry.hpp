/**
 * **************************************************************************
 * @file     MapGeometry.hpp
 * @brief    Declares shared integer geometry helpers for map generation
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "core/Point.hpp"

namespace rl {

enum class LineStartPolicy { IncludeStart, ExcludeStart };

[[nodiscard]] std::vector<Point>
bresenhamLine(Point from,
              Point to,
              LineStartPolicy startPolicy = LineStartPolicy::IncludeStart);

} // namespace rl
