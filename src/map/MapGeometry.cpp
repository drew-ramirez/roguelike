/**
 * **************************************************************************
 * @file     MapGeometry.cpp
 * @brief    Implements shared integer geometry helpers for map generation
 * **************************************************************************
 */

#include "map/MapGeometry.hpp"

#include <cstdlib>

namespace rl {

std::vector<Point>
bresenhamLine(Point from, Point to, LineStartPolicy startPolicy)
{
    std::vector<Point> points;

    int x = from.x;
    int y = from.y;
    const int dx = std::abs(to.x - from.x);
    const int sx = from.x < to.x ? 1 : -1;
    const int dy = -std::abs(to.y - from.y);
    const int sy = from.y < to.y ? 1 : -1;
    int error = dx + dy;

    if (startPolicy == LineStartPolicy::IncludeStart) {
        points.push_back(Point{x, y});
    }

    while (x != to.x || y != to.y) {
        const int doubledError = 2 * error;
        if (doubledError >= dy) {
            error += dy;
            x += sx;
        }
        if (doubledError <= dx) {
            error += dx;
            y += sy;
        }
        points.push_back(Point{x, y});
    }

    return points;
}

} // namespace rl
