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
#include "render/WallGlyph.hpp"

namespace rl {

namespace {

void renderTile(const Map& map,
                ConsoleBuffer& console,
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
        console.set(screenX,
                    screenY,
                    wallGlyph(map, worldX, worldY),
                    visible ? Color::white() : Color{80, 80, 80, 255},
                    Color::black());
    } else if (tile == TileType::DownStairs) {
        console.set(screenX,
                    screenY,
                    U'>',
                    visible ? Color{220, 220, 120, 255}
                            : Color{90, 90, 50, 255},
                    Color::black());
    } else if (tile == TileType::ClosedDoor) {
        console.set(screenX,
                    screenY,
                    U'+',
                    visible ? Color{185, 130, 70, 255}
                            : Color{85, 55, 30, 255},
                    Color::black());
    } else if (tile == TileType::OpenDoor) {
        console.set(screenX,
                    screenY,
                    U'/',
                    visible ? Color{170, 120, 65, 255}
                            : Color{75, 50, 30, 255},
                    Color::black());
    } else {
        const bool blood = map.hasBloodstain(worldX, worldY);
        console.set(screenX,
                    screenY,
                    blood ? U',' : U'.',
                    blood ? (visible ? Color{150, 20, 20, 255}
                                     : Color{65, 10, 10, 255})
                          : (visible ? Color{120, 120, 120, 255}
                                     : Color{45, 45, 45, 255}),
                    Color::black());
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
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            renderTile(map, console, x, y, x, y);
        }
    }
}

void renderMap(const Map& map, ConsoleBuffer& console, const Camera& camera)
{
    for (int screenY = 0; screenY < camera.height(); ++screenY) {
        for (int screenX = 0; screenX < camera.width(); ++screenX) {
            const Point world = camera.screenToWorld(Point{screenX, screenY});
            if (!map.inBounds(world.x, world.y)) {
                continue;
            }
            renderTile(map, console, world.x, world.y, screenX, screenY);
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
