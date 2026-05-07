/**
 * **************************************************************************
 * @file     RandomWalk.cpp
 * @brief    Implements shared random-walker movement helpers for generators
 * **************************************************************************
 */

#include "map/generators/RandomWalk.hpp"

namespace rl {

void staggerCardinal(Point& point, int width, int height, Rng& rng)
{
    switch (rng.rangeInclusive(0, 3)) {
    case 0:
        if (point.x > 2) {
            --point.x;
        }
        break;
    case 1:
        if (point.x < width - 2) {
            ++point.x;
        }
        break;
    case 2:
        if (point.y > 2) {
            --point.y;
        }
        break;
    default:
        if (point.y < height - 2) {
            ++point.y;
        }
        break;
    }
}

Point randomInteriorPoint(int width, int height, Rng& rng)
{
    return Point{rng.rangeInclusive(2, width - 2),
                 rng.rangeInclusive(2, height - 2)};
}

} // namespace rl
