/**
 * **************************************************************************
 * @file     MapRenderSystem.cpp
 * @brief    Draws revealed map tiles with visible and remembered colors
 * **************************************************************************
 */

#include "systems/MapRenderSystem.hpp"

#include "map/Map.hpp"
#include "map/TileType.hpp"
#include "render/ConsoleBuffer.hpp"

namespace rl {

void renderMap(const Map& map, ConsoleBuffer& console)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (!map.isRevealed(x, y)) {
                continue;
            }

            const bool visible = map.isVisible(x, y);
            if (map.tileAt(x, y) == TileType::Wall) {
                console.set(x,
                            y,
                            U'#',
                            visible ? Color::white() : Color{80, 80, 80, 255},
                            Color::black());
            } else {
                console.set(x,
                            y,
                            U'.',
                            visible ? Color{120, 120, 120, 255}
                                    : Color{45, 45, 45, 255},
                            Color::black());
            }
        }
    }
}

} // namespace rl
