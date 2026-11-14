#include "TestSuites.hpp"

#include <string>
#include <string_view>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/EquipmentSystem.hpp"
#include "ui/Hud.hpp"
#include "ui/WorldMapMenu.hpp"
#include "world/WorldGeneration.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool bufferContains(const ConsoleBuffer& console,
                                  std::string_view text)
{
    std::string row;
    row.reserve(static_cast<std::size_t>(console.width()));
    for (int y = 0; y < console.height(); ++y) {
        row.clear();
        for (int x = 0; x < console.width(); ++x) {
            const auto glyph = console.cellAt(x, y).glyph;
            row.push_back(glyph < 128 ? static_cast<char>(glyph) : '?');
        }
        if (row.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

void runUiTests(TestRunner& runner)
{
    entt::registry registry;
    GameResources resources;
    resources.depth = 4;
    resources.player = spawnPlayer(registry, Point{2, 2});

    registry.get<Attributes>(resources.player) = Attributes{13, 12, 11, 10};
    registry.get<Pools>(resources.player) = Pools{17, 30, 4, 6, 2, 250, 35};

    const auto sword = spawnSword(registry, Point{1, 1});
    registry.remove<Position>(sword);
    registry.emplace<InBackpack>(sword, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, sword),
        "test sword equips for HUD display");

    const auto armor = spawnChainMail(registry, Point{1, 2});
    registry.remove<Position>(armor);
    registry.emplace<InBackpack>(armor, InBackpack{resources.player});
    runner.expect(
        equipOrUnequipItem(registry, resources.log, resources.player, armor),
        "test armor equips for HUD display");

    resources.log.add("A recent message");

    ConsoleBuffer console{80, 50};
    console.clear();
    drawHud(console, resources, registry, RunState::PlayerTurn);

    runner.expect(bufferContains(console, "17/30") &&
                      bufferContains(console, "MP 4/6") &&
                      bufferContains(console, "Lv 2") &&
                      bufferContains(console, "250/2000") &&
                      bufferContains(console, "Gold 35"),
                  "HUD renders pools and progress stats");
    runner.expect(bufferContains(console, "Mgt 13 Fit 12 Qck 11 Int 10") &&
                      bufferContains(console, "Atk 9 Def 4 Wt 10"),
                  "HUD renders attributes derived stats and weight");
    runner.expect(bufferContains(console, "W: Sword  A: Chain Mail"),
                  "HUD renders equipped weapon and armor");
    runner.expect(bufferContains(console, "A recent message"),
                  "HUD still renders recent log messages");

    auto world = generateWorldMap(Rng::Seed{101U});
    world.cursor = reedfenWorldPosition();
    console.clear();
    drawWorldMapMenu(console, world);
    runner.expect(bufferContains(console, "?") &&
                      bufferContains(console, "Unknown wilds"),
                  "world map hides undiscovered cells");

    auto* reedfen = worldCellAt(world, reedfenWorldPosition());
    runner.expect(reedfen != nullptr, "test setup finds Reedfen cell");
    reedfen->discovered = true;
    console.clear();
    drawWorldMapMenu(console, world);
    runner.expect(bufferContains(console, "Reedfen") &&
                      bufferContains(console, "Reed marsh"),
                  "world map shows discovered biome details");
}

} // namespace rl::test
