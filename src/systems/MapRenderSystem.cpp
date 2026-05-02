#include "systems/MapRenderSystem.hpp"

#include "map/Map.hpp"
#include "map/TileType.hpp"
#include "render/ConsoleBuffer.hpp"

namespace rl {

void renderMap(const Map& map, ConsoleBuffer& console)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.tileAt(x, y) == TileType::Wall) {
                console.set(x, y, U'#', Color::white(), Color::black());
            } else {
                console.set(x, y, U'.', Color{90, 90, 90, 255}, Color::black());
            }
        }
    }
}

} // namespace rl
