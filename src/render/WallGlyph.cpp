/**
 * **************************************************************************
 * @file     WallGlyph.cpp
 * @brief    Chooses connected wall glyphs without changing tile data
 * **************************************************************************
 */

#include "render/WallGlyph.hpp"

#include "map/Map.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int northBit = 1;
constexpr int southBit = 2;
constexpr int westBit = 4;
constexpr int eastBit = 8;

[[nodiscard]] bool isWall(const Map& map, int x, int y)
{
    return map.inBounds(x, y) && map.tileAt(x, y) == TileType::Wall;
}

[[nodiscard]] int wallNeighborMask(const Map& map, int x, int y)
{
    int mask = 0;
    if (isWall(map, x, y - 1)) {
        mask |= northBit;
    }
    if (isWall(map, x, y + 1)) {
        mask |= southBit;
    }
    if (isWall(map, x - 1, y)) {
        mask |= westBit;
    }
    if (isWall(map, x + 1, y)) {
        mask |= eastBit;
    }
    return mask;
}

[[nodiscard]] char32_t wallGlyphFromMask(int mask,
                                         WallGlyphStyle style) noexcept
{
    if (style == WallGlyphStyle::AsciiFallback) {
        return U'#';
    }

    switch (mask) {
    case northBit:
    case southBit:
    case northBit | southBit:
        return U'\u2502';
    case westBit:
    case eastBit:
    case westBit | eastBit:
        return U'\u2500';
    case northBit | eastBit:
        return U'\u2514';
    case northBit | westBit:
        return U'\u2518';
    case southBit | eastBit:
        return U'\u250c';
    case southBit | westBit:
        return U'\u2510';
    case northBit | southBit | eastBit:
        return U'\u251c';
    case northBit | southBit | westBit:
        return U'\u2524';
    case northBit | westBit | eastBit:
        return U'\u2534';
    case southBit | westBit | eastBit:
        return U'\u252c';
    case northBit | southBit | westBit | eastBit:
        return U'\u253c';
    default:
        return U'#';
    }
}

} // namespace

char32_t wallGlyph(const Map& map, int x, int y, WallGlyphStyle style)
{
    return wallGlyphFromMask(wallNeighborMask(map, x, y), style);
}

} // namespace rl
