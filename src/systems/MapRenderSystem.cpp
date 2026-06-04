/**
 * **************************************************************************
 * @file     MapRenderSystem.cpp
 * @brief    Draws revealed map tiles with visible and remembered colors
 * **************************************************************************
 */

#include "systems/MapRenderSystem.hpp"

#include "map/Map.hpp"
#include "map/TileType.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/MapTheme.hpp"
#include "render/WallGlyph.hpp"

namespace rl {
namespace {

void renderTile(const Map& map,
                ConsoleBuffer& console,
                const MapTheme& theme,
                int worldX,
                int worldY,
                int screenX,
                int screenY)
{
    if (!map.isRevealed(worldX, worldY)) {
        return;
    }

    const bool visible = map.isVisible(worldX, worldY);
    const auto tile = map.tileAt(worldX, worldY);
    if (tile == TileType::Wall) {
        const auto& style = theme.style(MapThemeTile::Wall, visible);
        console.set(screenX,
                    screenY,
                    wallGlyph(map, worldX, worldY),
                    style.foreground,
                    style.background);
    } else if (tile == TileType::DownStairs) {
        const auto& style = theme.style(MapThemeTile::DownStairs, visible);
        console.set(
            screenX, screenY, style.glyph, style.foreground, style.background);
    } else if (tile == TileType::UpStairs) {
        const auto& style = theme.style(MapThemeTile::UpStairs, visible);
        console.set(
            screenX, screenY, style.glyph, style.foreground, style.background);
    } else if (tile == TileType::ClosedDoor) {
        const auto& style = theme.style(MapThemeTile::ClosedDoor, visible);
        console.set(
            screenX, screenY, style.glyph, style.foreground, style.background);
    } else if (tile == TileType::OpenDoor) {
        const auto& style = theme.style(MapThemeTile::OpenDoor, visible);
        console.set(
            screenX, screenY, style.glyph, style.foreground, style.background);
    } else {
        const bool blood = map.hasBloodstain(worldX, worldY);
        const auto& style = theme.style(
            blood ? MapThemeTile::Blood : MapThemeTile::Floor, visible);
        console.set(
            screenX, screenY, style.glyph, style.foreground, style.background);
    }
}

void renderDebugTile(const Map& map,
                     ConsoleBuffer& console,
                     int worldX,
                     int worldY,
                     int screenX,
                     int screenY)
{
    const auto tile = map.tileAt(worldX, worldY);
    if (tile == TileType::Wall) {
        console.set(
            screenX, screenY, wallGlyph(map, worldX, worldY), Color::white());
    } else if (tile == TileType::DownStairs) {
        console.set(screenX, screenY, U'>', Color{220, 220, 120, 255});
    } else if (tile == TileType::UpStairs) {
        console.set(screenX, screenY, U'<', Color{220, 220, 120, 255});
    } else if (tile == TileType::ClosedDoor) {
        console.set(screenX, screenY, U'+', Color{185, 130, 70, 255});
    } else if (tile == TileType::OpenDoor) {
        console.set(screenX, screenY, U'/', Color{170, 120, 65, 255});
    } else {
        console.set(screenX, screenY, U'.', Color{120, 120, 120, 255});
    }
}

} // namespace

void renderMap(const Map& map, ConsoleBuffer& console)
{
    renderMap(map, console, defaultMapTheme());
}

void renderMap(const Map& map, ConsoleBuffer& console, const MapTheme& theme)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            renderTile(map, console, theme, x, y, x, y);
        }
    }
}

void renderMap(const Map& map, ConsoleBuffer& console, const Camera& camera)
{
    renderMap(map, console, camera, defaultMapTheme());
}

void renderMap(const Map& map,
               ConsoleBuffer& console,
               const Camera& camera,
               const MapTheme& theme)
{
    for (int screenY = 0; screenY < camera.height(); ++screenY) {
        for (int screenX = 0; screenX < camera.width(); ++screenX) {
            const Point world = camera.screenToWorld(Point{screenX, screenY});
            if (!map.inBounds(world.x, world.y)) {
                continue;
            }
            renderTile(map, console, theme, world.x, world.y, screenX, screenY);
        }
    }
}

void renderMapDebug(const Map& map,
                    ConsoleBuffer& console,
                    const Camera& camera)
{
    for (int screenY = 0; screenY < camera.height(); ++screenY) {
        for (int screenX = 0; screenX < camera.width(); ++screenX) {
            const Point world = camera.screenToWorld(Point{screenX, screenY});
            if (!map.inBounds(world.x, world.y)) {
                continue;
            }
            renderDebugTile(map, console, world.x, world.y, screenX, screenY);
        }
    }
}

} // namespace rl
