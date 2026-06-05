/**
 * **************************************************************************
 * @file     WorldMapMenu.cpp
 * @brief    Handles the zoomed macro-world map modal
 * **************************************************************************
 */

#include "ui/WorldMapMenu.hpp"

#include <algorithm>
#include <string>

#include "core/GameResources.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

constexpr int originX = 28;
constexpr int originY = 9;
constexpr int cellSpacingX = 3;
constexpr int cellSpacingY = 2;

void drawFrame(ConsoleBuffer& console)
{
    constexpr int left = 20;
    constexpr int top = 5;
    constexpr int width = 40;
    constexpr int height = 27;
    for (int y = top; y < top + height; ++y) {
        for (int x = left; x < left + width; ++x) {
            const bool border = x == left || x == left + width - 1 ||
                                y == top || y == top + height - 1;
            console.set(x,
                        y,
                        border ? U'#' : U' ',
                        Color{160, 160, 160, 255},
                        Color::black());
        }
    }
}

[[nodiscard]] Point clampedCursor(Point cursor) noexcept
{
    return Point{std::clamp(cursor.x, 0, worldMapWidth - 1),
                 std::clamp(cursor.y, 0, worldMapHeight - 1)};
}

} // namespace

WorldMapInputResult handleWorldMapInput(GameResources& resources,
                                        const std::vector<InputEvent>& events)
{
    auto& world = resources.world;
    world.cursor = clampedCursor(world.cursor);

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape || event.key == Key::Ascend) {
            return WorldMapInputResult::Close;
        }
        if (event.key == Key::Descend || event.key == Key::Enter) {
            return WorldMapInputResult::EnterSelected;
        }

        const auto delta = movementDeltaForKey(event.key);
        if (delta.has_value()) {
            world.cursor = clampedCursor(
                Point{world.cursor.x + delta->x, world.cursor.y + delta->y});
            return WorldMapInputResult::None;
        }
    }

    return WorldMapInputResult::None;
}

void drawWorldMapMenu(ConsoleBuffer& console, const WorldMapState& world)
{
    drawFrame(console);
    console.print(originX - 3, originY - 2, "World Map", Color::yellow());

    for (int y = 0; y < worldMapHeight; ++y) {
        for (int x = 0; x < worldMapWidth; ++x) {
            const Point point{x, y};
            const auto* cell = worldCellAt(world, point);
            const bool known = cell != nullptr && cell->discovered;
            const bool cursor = point == world.cursor;
            const char32_t glyph =
                known && cell != nullptr ? cell->glyph : U'?';
            Color foreground = known && cell != nullptr
                                   ? cell->foreground
                                   : Color{100, 100, 100, 255};
            if (cursor) {
                foreground = Color::yellow();
            }
            console.set(originX + (x * cellSpacingX),
                        originY + (y * cellSpacingY),
                        glyph,
                        foreground,
                        Color::black());
        }
    }

    const auto* selected = worldCellAt(world, world.cursor);
    const int detailY = originY + (worldMapHeight * cellSpacingY) + 2;
    if (selected == nullptr || !selected->discovered) {
        console.print(
            originX - 6, detailY, "Unknown wilds", Color{160, 160, 160, 255});
    } else {
        console.print(originX - 6, detailY, selected->name, Color::white());
        console.print(originX - 6,
                      detailY + 1,
                      std::string{biomeDisplayName(selected->biome)},
                      selected->foreground);
    }
    console.print(originX - 6,
                  detailY + 4,
                  "Move cursor  > enter  < close",
                  Color{170, 170, 170, 255});
}

} // namespace rl
