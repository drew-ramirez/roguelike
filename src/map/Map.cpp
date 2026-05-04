/**
 * **************************************************************************
 * @file     Map.cpp
 * @brief    Stores tiles, visibility, and revealed map memory
 * **************************************************************************
 */

#include "map/Map.hpp"

#include <algorithm>
#include <stdexcept>

namespace rl {

Map::Map(int width, int height, TileType fill)
    : width_(width), height_(height),
      tiles_(width > 0 && height > 0 ? static_cast<std::size_t>(width * height)
                                     : 0U,
             fill),
      bloodstains_(tiles_.size(), false), visibleTiles_(tiles_.size(), false),
      revealedTiles_(tiles_.size(), false)
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

int Map::tileCount() const noexcept
{
    return static_cast<int>(tiles_.size());
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

bool Map::blocksSight(int x, int y) const
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

bool Map::isVisible(int x, int y) const
{
    if (!inBounds(x, y)) {
        return false;
    }

    return visibleTiles_[static_cast<std::size_t>(xyIdx(x, y))];
}

bool Map::isRevealed(int x, int y) const
{
    if (!inBounds(x, y)) {
        return false;
    }

    return revealedTiles_[static_cast<std::size_t>(xyIdx(x, y))];
}

bool Map::hasBloodstain(int x, int y) const
{
    if (!inBounds(x, y)) {
        return false;
    }

    return bloodstains_[static_cast<std::size_t>(xyIdx(x, y))];
}

void Map::setTile(int x, int y, TileType tile)
{
    tiles_[static_cast<std::size_t>(xyIdx(x, y))] = tile;
}

void Map::setBloodstain(int x, int y, bool bloodstained)
{
    if (!inBounds(x, y)) {
        return;
    }

    bloodstains_[static_cast<std::size_t>(xyIdx(x, y))] = bloodstained;
}

void Map::clearVisible()
{
    std::fill(visibleTiles_.begin(), visibleTiles_.end(), false);
}

void Map::revealAll()
{
    std::fill(revealedTiles_.begin(), revealedTiles_.end(), true);
}

void Map::setVisible(int x, int y, bool visible)
{
    if (!inBounds(x, y)) {
        return;
    }

    const auto index = static_cast<std::size_t>(xyIdx(x, y));
    visibleTiles_[index] = visible;
    if (visible) {
        // Seeing a tile also records it as explored map memory
        revealedTiles_[index] = true;
    }
}

void Map::setRevealed(int x, int y, bool revealed)
{
    if (!inBounds(x, y)) {
        return;
    }

    revealedTiles_[static_cast<std::size_t>(xyIdx(x, y))] = revealed;
}

} // namespace rl
