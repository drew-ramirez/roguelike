#pragma once

#include <vector>

#include "map/TileType.hpp"

namespace rl {

class Map {
public:
    Map(int width, int height, TileType fill = TileType::Floor);

    [[nodiscard]] static Map createStarter(int width = 80, int height = 45);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool inBounds(int x, int y) const noexcept;
    [[nodiscard]] int xyIdx(int x, int y) const;
    [[nodiscard]] TileType tileAt(int x, int y) const;
    [[nodiscard]] bool blocksMovement(int x, int y) const;
    [[nodiscard]] bool isWalkable(int x, int y) const;

    void setTile(int x, int y, TileType tile);

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<TileType> tiles_;
};

} // namespace rl
