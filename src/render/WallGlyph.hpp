/**
 * **************************************************************************
 * @file     WallGlyph.hpp
 * @brief    Declares render-time connected wall glyph selection
 * **************************************************************************
 */

#pragma once

namespace rl {

class Map;

enum class WallGlyphStyle { Connected, AsciiFallback };

[[nodiscard]] int wallNeighborMask(const Map& map, int x, int y);
[[nodiscard]] char32_t
wallGlyphFromMask(int mask,
                  WallGlyphStyle style = WallGlyphStyle::Connected) noexcept;
[[nodiscard]] char32_t
wallGlyph(const Map& map,
          int x,
          int y,
          WallGlyphStyle style = WallGlyphStyle::Connected);

} // namespace rl
