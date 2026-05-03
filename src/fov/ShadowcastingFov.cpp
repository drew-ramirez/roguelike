#include "fov/ShadowcastingFov.hpp"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "map/Map.hpp"

namespace rl {
namespace {

bool lineOfSight(const Map& map, Point from, Point to)
{
    int x = from.x;
    int y = from.y;
    const int dx = std::abs(to.x - from.x);
    const int dy = -std::abs(to.y - from.y);
    const int stepX = from.x < to.x ? 1 : -1;
    const int stepY = from.y < to.y ? 1 : -1;
    int error = dx + dy;

    while (true) {
        if (x == to.x && y == to.y) {
            return true;
        }

        if ((x != from.x || y != from.y) && map.blocksSight(x, y)) {
            return false;
        }

        const int twiceError = 2 * error;
        if (twiceError >= dy) {
            error += dy;
            x += stepX;
        }
        if (twiceError <= dx) {
            error += dx;
            y += stepY;
        }
    }
}

} // namespace

std::vector<Point>
ShadowcastingFov::compute(const Map& map, Point origin, int range) const
{
    std::vector<Point> visible;
    if (range < 0 || !map.inBounds(origin.x, origin.y)) {
        return visible;
    }

    visible.reserve(
        static_cast<std::size_t>((range * 2 + 1) * (range * 2 + 1)));
    const int radiusSquared = range * range;

    for (int y = origin.y - range; y <= origin.y + range; ++y) {
        for (int x = origin.x - range; x <= origin.x + range; ++x) {
            if (!map.inBounds(x, y)) {
                continue;
            }

            const int deltaX = x - origin.x;
            const int deltaY = y - origin.y;
            if (deltaX * deltaX + deltaY * deltaY > radiusSquared) {
                continue;
            }

            const Point target{x, y};
            if (lineOfSight(map, origin, target)) {
                visible.push_back(target);
            }
        }
    }

    return visible;
}

} // namespace rl
