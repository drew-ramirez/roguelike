/**
 * **************************************************************************
 * @file     MapTheme.hpp
 * @brief    Declares gameplay map color themes loaded from REX Paint palettes
 * **************************************************************************
 */

#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string_view>

#include "render/Color.hpp"

namespace rl {

enum class MapThemeTile : std::size_t {
    Wall = 0,
    Floor,
    Blood,
    DownStairs,
    UpStairs,
    ClosedDoor,
    OpenDoor
};

inline constexpr std::size_t mapThemeTileCount = 7;

struct MapTileStyle {
    char32_t glyph = U' ';
    Color foreground = Color::white();
    Color background = Color::black();
};

struct MapTheme {
    std::array<MapTileStyle, mapThemeTileCount> visible{};
    std::array<MapTileStyle, mapThemeTileCount> remembered{};

    [[nodiscard]] const MapTileStyle& style(MapThemeTile tile,
                                            bool visibleTile) const;
};

/**
 * Returns the existing built-in gameplay terrain colors.
 */
[[nodiscard]] MapTheme defaultMapTheme();

/**
 * Loads a 6x2 REX Paint palette sheet for gameplay map rendering.
 */
[[nodiscard]] MapTheme
loadMapThemeFromRexPaint(const std::filesystem::path& path);

/**
 * Resolves a stable map theme id used by CLI and tests.
 */
[[nodiscard]] MapTheme mapThemeById(std::string_view id);

} // namespace rl
