#include "map/Map.hpp"

#include <stdexcept>

namespace rl {

Map::Map(int width, int height, TileType fill)
    : width_(width), height_(height),
      tiles_(width > 0 && height > 0 ? static_cast<std::size_t>(width * height)
                                     : 0U,
             fill)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Map dimensions must be positive");
    }
}

Map Map::createStarter(int width, int height)
{
    Map map{width, height};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
                map.setTile(x, y, TileType::Wall);
            }
        }
    }

    for (int y = 8; y <= 14 && y < height - 1; ++y) {
        if (12 < width - 1) {
            map.setTile(12, y, TileType::Wall);
        }
    }

    return map;
}

int Map::width() const noexcept
{
    return width_;
}

int Map::height() const noexcept
{
    return height_;
}

bool Map::inBounds(int x, int y) const noexcept
{
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

int Map::xyIdx(int x, int y) const
{
    if (!inBounds(x, y)) {
        throw std::out_of_range("Map coordinate is out of bounds");
    }

    return y * width_ + x;
}

TileType Map::tileAt(int x, int y) const
{
    return tiles_[static_cast<std::size_t>(xyIdx(x, y))];
}

bool Map::blocksMovement(int x, int y) const
{
    if (!inBounds(x, y)) {
        return true;
    }

    return tileAt(x, y) == TileType::Wall;
}

bool Map::isWalkable(int x, int y) const
{
    return !blocksMovement(x, y);
}

void Map::setTile(int x, int y, TileType tile)
{
    tiles_[static_cast<std::size_t>(xyIdx(x, y))] = tile;
}

} // namespace rl
