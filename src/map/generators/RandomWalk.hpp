/**
 * **************************************************************************
 * @file     RandomWalk.hpp
 * @brief    Declares shared random-walker movement helpers for generators
 * **************************************************************************
 */

#pragma once

#include "core/Point.hpp"
#include "core/Rng.hpp"

namespace rl {

void staggerCardinal(Point& point, int width, int height, Rng& rng);
[[nodiscard]] Point randomInteriorPoint(int width, int height, Rng& rng);

} // namespace rl
