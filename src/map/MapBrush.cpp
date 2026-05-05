/**
 * **************************************************************************
 * @file     MapBrush.cpp
 * @brief    Implements brush and symmetry utilities for map generation
 * **************************************************************************
 */

#include "map/MapBrush.hpp"

#include <algorithm>
#include <cstdlib>

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
                // Circle uses squared radius: points inside so
                // dx^2 + dy^2 <= r^2 are painted, avoiding sqrt per tile
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

    const int centerX = map.width() / 2;
    const int centerY = map.height() / 2;
    const int distX = std::abs(centerX - point.x);
    const int distY = std::abs(centerY - point.y);
    // Mirror by preserving the absolute offset from the map center. This keeps
    // paired diggers visually symmetric even on maps with odd dimensions.
    const Point horizontalLeft{centerX - distX, point.y};
    const Point horizontalRight{centerX + distX, point.y};
    const Point verticalTop{point.x, centerY - distY};
    const Point verticalBottom{point.x, centerY + distY};

    switch (symmetry) {
    case Symmetry::None:
        addUnique(points, point);
        break;
    case Symmetry::Horizontal:
        addUnique(points, horizontalLeft);
        addUnique(points, horizontalRight);
        break;
    case Symmetry::Vertical:
        addUnique(points, verticalTop);
        addUnique(points, verticalBottom);
        break;
    case Symmetry::Both:
        addUnique(points, horizontalLeft);
        addUnique(points, horizontalRight);
        addUnique(points, verticalTop);
        addUnique(points, verticalBottom);
        break;
    case Symmetry::Quad:
        addUnique(points, horizontalLeft);
        addUnique(points, horizontalRight);
        addUnique(points, verticalTop);
        addUnique(points, verticalBottom);
        addUnique(points, Point{centerX - distX, centerY - distY});
        addUnique(points, Point{centerX + distX, centerY - distY});
        addUnique(points, Point{centerX - distX, centerY + distY});
        addUnique(points, Point{centerX + distX, centerY + distY});
        break;
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

std::vector<Point> paintBrushSize(
    Map& map, Point center, TileType tile, int brushSize, Symmetry symmetry)
{
    std::vector<Point> painted;
    const int clampedSize = std::max(1, brushSize);

    for (const Point origin : symmetricalPoints(map, center, symmetry)) {
        if (clampedSize == 1) {
            if (map.inBounds(origin.x, origin.y)) {
                map.setTile(origin.x, origin.y, tile);
                addUnique(painted, origin);
            }
            continue;
        }

        const int halfBrushSize = clampedSize / 2;
        // The brush-size variant paints a square stamp for the chunkier
        // tunnel look used by fat Drunkard and DLA.
        for (int y = origin.y - halfBrushSize; y < origin.y + halfBrushSize;
             ++y) {
            for (int x = origin.x - halfBrushSize; x < origin.x + halfBrushSize;
                 ++x) {
                if (x <= 1 || y <= 1 || x >= map.width() - 1 ||
                    y >= map.height() - 1) {
                    continue;
                }
                const Point paintedPoint{x, y};
                map.setTile(x, y, tile);
                addUnique(painted, paintedPoint);
            }
        }
    }

    return painted;
}

} // namespace rl
