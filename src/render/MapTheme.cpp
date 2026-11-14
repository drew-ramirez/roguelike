/**
 * **************************************************************************
 * @file     MapTheme.cpp
 * @brief    Loads and resolves gameplay map color themes
 * **************************************************************************
 */

#include "render/MapTheme.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

#include "assets/RexPaintLoader.hpp"

namespace rl {
namespace {

constexpr std::array<char32_t, mapThemeTileCount> paletteGlyphs{
    U'#',
    U'.',
    U',',
    U'>',
    U'<',
    U'+',
    U'/',
    U'\u2663',
    U'\u2248',
    U'\u25b2',
};

constexpr std::array<char32_t, 6> legacyPaletteGlyphs{
    U'#',
    U'.',
    U',',
    U'>',
    U'+',
    U'/',
};

constexpr std::array<MapThemeTile, 6> legacyPaletteTiles{
    MapThemeTile::Wall,
    MapThemeTile::Floor,
    MapThemeTile::Blood,
    MapThemeTile::DownStairs,
    MapThemeTile::ClosedDoor,
    MapThemeTile::OpenDoor,
};

constexpr int paletteHeight = 2;

[[nodiscard]] MapTileStyle styleFromCell(const RexPaintCell& cell)
{
    return MapTileStyle{cell.glyph, cell.foreground, cell.background};
}

void requireGlyphSlot(const RexPaintCell& cell,
                      char32_t expected,
                      const std::filesystem::path& path)
{
    if (cell.glyph != expected) {
        throw RexPaintError{
            "REX Paint map theme has an invalid glyph slot in " +
            path.string()};
    }
}

} // namespace

const MapTileStyle& MapTheme::style(MapThemeTile tile, bool visibleTile) const
{
    const auto index = static_cast<std::size_t>(tile);
    return visibleTile ? visible[index] : remembered[index];
}

MapTheme defaultMapTheme()
{
    MapTheme theme;
    theme.visible = std::array<MapTileStyle, mapThemeTileCount>{
        MapTileStyle{U'#', Color::white(), Color::black()},
        MapTileStyle{U'.', Color{120, 120, 120, 255}, Color::black()},
        MapTileStyle{U',', Color{150, 20, 20, 255}, Color::black()},
        MapTileStyle{U'>', Color{220, 220, 120, 255}, Color::black()},
        MapTileStyle{U'<', Color{220, 220, 120, 255}, Color::black()},
        MapTileStyle{U'+', Color{185, 130, 70, 255}, Color::black()},
        MapTileStyle{U'/', Color{170, 120, 65, 255}, Color::black()},
        MapTileStyle{U'\u2663', Color{45, 135, 55, 255}, Color::black()},
        MapTileStyle{U'\u2248', Color{65, 145, 125, 255}, Color::black()},
        MapTileStyle{U'\u25b2', Color{150, 145, 130, 255}, Color::black()},
    };
    theme.remembered = std::array<MapTileStyle, mapThemeTileCount>{
        MapTileStyle{U'#', Color{80, 80, 80, 255}, Color::black()},
        MapTileStyle{U'.', Color{45, 45, 45, 255}, Color::black()},
        MapTileStyle{U',', Color{65, 10, 10, 255}, Color::black()},
        MapTileStyle{U'>', Color{90, 90, 50, 255}, Color::black()},
        MapTileStyle{U'<', Color{90, 90, 50, 255}, Color::black()},
        MapTileStyle{U'+', Color{80, 55, 30, 255}, Color::black()},
        MapTileStyle{U'/', Color{70, 50, 30, 255}, Color::black()},
        MapTileStyle{U'\u2663', Color{25, 65, 30, 255}, Color::black()},
        MapTileStyle{U'\u2248', Color{30, 65, 60, 255}, Color::black()},
        MapTileStyle{U'\u25b2', Color{65, 65, 60, 255}, Color::black()},
    };
    return theme;
}

MapTheme loadMapThemeFromRexPaint(const std::filesystem::path& path)
{
    const auto image = loadRexPaintImage(path);
    if (image.layers.empty() ||
        (image.width != 6 && image.width != 7 &&
         image.width != static_cast<int>(mapThemeTileCount)) ||
        image.height != paletteHeight) {
        throw RexPaintError{
            "REX Paint map theme must be a 6x2, 7x2, or 10x2 palette: " +
            path.string()};
    }

    const auto& layer = image.layers.front();
    MapTheme theme = defaultMapTheme();
    for (std::size_t column = 0; column < static_cast<std::size_t>(image.width);
         ++column) {
        const auto x = static_cast<int>(column);
        const bool legacy = image.width == 6;
        const auto expectedGlyph =
            legacy ? legacyPaletteGlyphs[column] : paletteGlyphs[column];
        const auto themeIndex = static_cast<std::size_t>(
            legacy ? legacyPaletteTiles[column]
                   : static_cast<MapThemeTile>(column));
        const auto& visibleCell = layer.cellAt(x, 0);
        const auto& rememberedCell = layer.cellAt(x, 1);
        requireGlyphSlot(visibleCell, expectedGlyph, path);
        requireGlyphSlot(rememberedCell, expectedGlyph, path);
        theme.visible[themeIndex] = styleFromCell(visibleCell);
        theme.remembered[themeIndex] = styleFromCell(rememberedCell);
    }
    if (image.width == 6) {
        theme.visible[static_cast<std::size_t>(MapThemeTile::UpStairs)] =
            MapTileStyle{
                U'<',
                theme
                    .visible[static_cast<std::size_t>(MapThemeTile::DownStairs)]
                    .foreground,
                theme
                    .visible[static_cast<std::size_t>(MapThemeTile::DownStairs)]
                    .background};
        theme.remembered[static_cast<std::size_t>(
            MapThemeTile::UpStairs)] = MapTileStyle{
            U'<',
            theme.remembered[static_cast<std::size_t>(MapThemeTile::DownStairs)]
                .foreground,
            theme.remembered[static_cast<std::size_t>(MapThemeTile::DownStairs)]
                .background};
    }
    return theme;
}

MapTheme mapThemeById(std::string_view id)
{
    if (id == "default") {
        return defaultMapTheme();
    }
    if (id == "druidic") {
        return loadMapThemeFromRexPaint(
            std::filesystem::path{"assets/map-theme-druidic.xp"});
    }

    throw std::invalid_argument{"Unknown map theme: " + std::string{id}};
}

} // namespace rl
