#include "ui/Hud.hpp"

#include <string>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int uiTop = 43;
constexpr int uiHeight = 7;
constexpr int hpBarWidth = 20;
constexpr std::size_t visibleMessages = 5;

} // namespace

void drawHud(ConsoleBuffer& console,
             const GameResources& resources,
             const entt::registry& registry,
             RunState state)
{
    drawPanel(console, uiTop, uiHeight);

    if (resources.player != entt::null && registry.valid(resources.player) &&
        registry.all_of<CombatStats>(resources.player)) {
        const auto& stats = registry.get<CombatStats>(resources.player);
        console.print(1, uiTop + 1, "HP", Color::white());
        drawBar(console,
                4,
                uiTop + 1,
                hpBarWidth,
                stats.hp,
                stats.maxHp,
                Color{220, 60, 60, 255},
                Color{70, 30, 30, 255});
        console.print(26,
                      uiTop + 1,
                      std::to_string(stats.hp) + "/" +
                          std::to_string(stats.maxHp),
                      Color::white());
    }

    console.print(40,
                  uiTop + 1,
                  "Depth " + std::to_string(resources.depth),
                  Color{180, 180, 180, 255});
    if (state == RunState::GameOver) {
        console.print(58, uiTop + 1, "GAME OVER", Color{255, 80, 80, 255});
    } else {
        console.print(58, uiTop + 1, "g/i/>/F5", Color{130, 130, 130, 255});
    }

    const auto messages = resources.log.recent(visibleMessages);
    int row = uiTop + 2;
    for (const auto message : messages) {
        console.print(1, row, message, Color{190, 190, 190, 255});
        ++row;
    }
}

} // namespace rl
