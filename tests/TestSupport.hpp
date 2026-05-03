#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/TileType.hpp"

namespace rl::test {

struct TestRunner {
    int failures = 0;

    void expect(bool condition, const std::string& message)
    {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
};

[[nodiscard]] inline Map openMap(int width = 12, int height = 8)
{
    Map map{width, height, TileType::Floor};
    for (int x = 0; x < width; ++x) {
        map.setTile(x, 0, TileType::Wall);
        map.setTile(x, height - 1, TileType::Wall);
    }
    for (int y = 0; y < height; ++y) {
        map.setTile(0, y, TileType::Wall);
        map.setTile(width - 1, y, TileType::Wall);
    }
    return map;
}

inline void revealAll(Map& map)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            map.setVisible(x, y, true);
        }
    }
}

[[nodiscard]] inline std::optional<Point> findTile(const Map& map,
                                                   TileType tile)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.tileAt(x, y) == tile) {
                return Point{x, y};
            }
        }
    }
    return std::nullopt;
}

} // namespace rl::test
