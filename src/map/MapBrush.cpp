/**
 * **************************************************************************
 * @file     MapBrush.cpp
 * @brief    Implements brush and symmetry utilities for map generation
 * **************************************************************************
 */

#include "map/MapBrush.hpp"

#include <algorithm>

#include "map/Map.hpp"

namespace rl {
namespace {

void addUnique(std::vector<Point>& points, Point point)
{
    if (std::find(points.begin(), points.end(), point) == points.end()) {
        points.push_back(point);
    }
}

} // namespace

std::vector<Point> brushPoints(Point center, BrushShape shape, int radius)
{
    std::vector<Point> points;
    const int clampedRadius = std::max(0, radius);

    if (shape == BrushShape::Single) {
        points.push_back(center);
        return points;
    }

    for (int y = center.y - clampedRadius; y <= center.y + clampedRadius; ++y) {
        for (int x = center.x - clampedRadius; x <= center.x + clampedRadius;
             ++x) {
            if (shape == BrushShape::Circle) {
                const int dx = x - center.x;
                const int dy = y - center.y;
                if ((dx * dx) + (dy * dy) > clampedRadius * clampedRadius) {
                    continue;
                }
            }
            points.push_back(Point{x, y});
        }
    }

    return points;
}

std::vector<Point>
symmetricalPoints(const Map& map, Point point, Symmetry symmetry)
{
    std::vector<Point> points;
    addUnique(points, point);

    const Point horizontal{map.width() - 1 - point.x, point.y};
    const Point vertical{point.x, map.height() - 1 - point.y};
    const Point both{map.width() - 1 - point.x, map.height() - 1 - point.y};

    if (symmetry == Symmetry::Horizontal || symmetry == Symmetry::Both ||
        symmetry == Symmetry::Quad) {
        addUnique(points, horizontal);
    }
    if (symmetry == Symmetry::Vertical || symmetry == Symmetry::Both ||
        symmetry == Symmetry::Quad) {
        addUnique(points, vertical);
    }
    if (symmetry == Symmetry::Both || symmetry == Symmetry::Quad) {
        addUnique(points, both);
    }

    return points;
}

std::vector<Point> paintBrush(Map& map,
                              Point center,
                              TileType tile,
                              BrushShape shape,
                              int radius,
                              Symmetry symmetry)
{
    std::vector<Point> painted;
    for (const Point origin : symmetricalPoints(map, center, symmetry)) {
        for (const Point point : brushPoints(origin, shape, radius)) {
            if (!map.inBounds(point.x, point.y)) {
                continue;
            }
            map.setTile(point.x, point.y, tile);
            addUnique(painted, point);
        }
    }
    return painted;
}

} // namespace rl
