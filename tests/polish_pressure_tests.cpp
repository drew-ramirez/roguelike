#include "TestSuites.hpp"

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "ecs/Components.hpp"
#include "input/InputEvent.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/MapTheme.hpp"
#include "render/WallGlyph.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/TrapSystem.hpp"
#include "ui/MainMenu.hpp"

namespace rl::test {
namespace {

[[nodiscard]] int countParticles(const entt::registry& registry)
{
    int count = 0;
    const auto view = registry.view<const Particle>();
    for ([[maybe_unused]] const auto entity : view) {
        ++count;
    }
    return count;
}

[[nodiscard]] bool sameColor(Color left, Color right) noexcept
{
    return left.r == right.r && left.g == right.g && left.b == right.b &&
           left.a == right.a;
}

} // namespace

void runPolishPressureTests(TestRunner& runner)
{
    {
        Map map{3, 3, TileType::Floor};
        map.setTile(1, 0, TileType::Wall);
        map.setTile(1, 1, TileType::Wall);
        map.setTile(1, 2, TileType::Wall);

        runner.expect(wallNeighborMask(map, 1, 1) == 3,
                      "wall bitmask sees north and south walls");
        runner.expect(wallGlyphFromMask(3) == U'\u2502',
                      "connected wall glyph uses vertical line");
        runner.expect(wallGlyphFromMask(3, WallGlyphStyle::AsciiFallback) ==
                          U'#',
                      "wall glyph fallback stays ASCII");
    }

    {
        Map map{7, 4, TileType::Floor};
        map.setTile(1, 1, TileType::Wall);
        map.setTile(3, 1, TileType::ClosedDoor);
        map.setTile(4, 1, TileType::OpenDoor);
        map.setTile(5, 1, TileType::DownStairs);
        map.setVisible(1, 1);
        map.setVisible(2, 1);
        map.setVisible(3, 1);
        map.setVisible(4, 1);
        map.setVisible(5, 1);

        ConsoleBuffer console{7, 4};
        console.clear();
        renderMap(map, console);
        runner.expect(
            sameColor(console.cellAt(1, 1).foreground, Color::white()) &&
                sameColor(console.cellAt(2, 1).foreground,
                          Color{120, 120, 120, 255}) &&
                sameColor(console.cellAt(3, 1).foreground,
                          Color{185, 130, 70, 255}) &&
                sameColor(console.cellAt(4, 1).foreground,
                          Color{170, 120, 65, 255}) &&
                sameColor(console.cellAt(5, 1).foreground,
                          Color{220, 220, 120, 255}),
            "default map rendering keeps grayscale yellow brown colors");

        const auto druidic = mapThemeById("druidic");
        console.clear();
        renderMap(map, console, druidic);
        runner.expect(
            sameColor(console.cellAt(1, 1).foreground,
                      druidic.style(MapThemeTile::Wall, true).foreground) &&
                sameColor(
                    console.cellAt(2, 1).foreground,
                    druidic.style(MapThemeTile::Floor, true).foreground) &&
                sameColor(
                    console.cellAt(3, 1).foreground,
                    druidic.style(MapThemeTile::ClosedDoor, true).foreground) &&
                sameColor(
                    console.cellAt(5, 1).foreground,
                    druidic.style(MapThemeTile::DownStairs, true).foreground) &&
                !sameColor(console.cellAt(2, 1).foreground,
                           Color{120, 120, 120, 255}),
            "druidic map rendering changes floor wall door and stairs colors");
    }

    {
        Map map{4, 3, TileType::Floor};
        map.setRevealed(1, 1);

        const auto druidic = mapThemeById("druidic");
        ConsoleBuffer console{4, 3};
        console.clear();
        renderMap(map, console, druidic);
        runner.expect(
            sameColor(console.cellAt(1, 1).foreground,
                      druidic.style(MapThemeTile::Floor, false).foreground) &&
                sameColor(console.cellAt(1, 1).background,
                          druidic.style(MapThemeTile::Floor, false).background),
            "remembered tiles use the darker REX palette row");
    }

    {
        Map map{4, 3, TileType::Floor};
        map.setBloodstain(1, 1);
        map.setVisible(1, 1);

        const auto druidic = mapThemeById("druidic");
        ConsoleBuffer console{4, 3};
        console.clear();
        renderMap(map, console, druidic);
        runner.expect(
            console.cellAt(1, 1).glyph == U',' &&
                sameColor(
                    console.cellAt(1, 1).foreground,
                    druidic.style(MapThemeTile::Blood, true).foreground) &&
                !sameColor(console.cellAt(1, 1).foreground,
                           druidic.style(MapThemeTile::Floor, true).foreground),
            "bloodstains override floor glyph and theme color");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto goblin = spawnGoblin(registry, Point{3, 2});

        queueDamage(registry, goblin, 2);
        RunState state = RunState::PlayerTurn;
        applyDamageAndCleanup(registry, resources, state, &map);
        runner.expect(map.hasBloodstain(3, 2),
                      "damage marks a persistent blood decal");
        runner.expect(map.isWalkable(3, 2) && !map.blocksSight(3, 2),
                      "blood does not affect movement or sight");

        const auto text = serializeGame(registry, resources, map);
        entt::registry loadedRegistry;
        GameResources loadedResources;
        Map loadedMap{1, 1};
        runner.expect(
            deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
            "blood save data loads");
        runner.expect(loadedMap.hasBloodstain(3, 2),
                      "blood survives save/load");
    }

    {
        entt::registry registry;
        GameResources resources;
        requestDamageParticle(resources, Point{1, 1});
        flushParticleRequests(registry, resources);
        runner.expect(countParticles(registry) == 1,
                      "particle requests create transient entities");
        for (int frame = 0; frame < 6; ++frame) {
            updateParticleLifetimes(registry);
        }
        runner.expect(countParticles(registry) == 0,
                      "particles expire by frame lifetime");
    }

    {
        auto map = openMap();
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        auto& clock = registry.get<HungerClock>(resources.player);
        clock.duration = 2;
        processHunger(registry, resources);
        runner.expect(clock.duration == 1,
                      "hunger decreases after a real player turn");
        processHunger(registry, resources);
        runner.expect(registry.all_of<SufferDamage>(resources.player),
                      "starvation queues gentle damage");

        const auto food = spawnFoodRation(registry, Point{1, 1});
        registry.remove<Position>(food);
        registry.emplace<InBackpack>(food, InBackpack{resources.player});
        clock.duration = 10;
        runner.expect(
            useInventoryItem(registry, resources, map, resources.player, food),
            "food use consumes a turn");
        runner.expect(clock.duration > 10 && !registry.valid(food),
                      "food restores hunger and is consumed");
    }

    {
        auto map = openMap();
        map.setVisible(2, 2, true);
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto scroll = spawnMagicMappingScroll(registry, Point{1, 1});
        registry.remove<Position>(scroll);
        registry.emplace<InBackpack>(scroll, InBackpack{resources.player});

        runner.expect(useInventoryItem(
                          registry, resources, map, resources.player, scroll),
                      "magic mapping scroll consumes a turn");
        runner.expect(map.isRevealed(1, 1) && map.isRevealed(10, 6),
                      "magic mapping reveals terrain memory");
        runner.expect(!map.isVisible(10, 6),
                      "magic mapping does not expand current FOV");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        const auto trap = spawnSpikeTrap(registry, Point{2, 2});
        runner.expect(registry.all_of<Hidden>(trap),
                      "new spike traps start hidden");
        processPlayerTraps(registry, resources);
        runner.expect(!registry.all_of<Hidden>(trap),
                      "triggered traps are revealed");
        runner.expect(registry.get<Trap>(trap).triggered,
                      "triggered trap state is durable data");
        runner.expect(registry.all_of<SufferDamage>(resources.player),
                      "player-only trap trigger queues damage");

        const auto text = serializeGame(registry, resources, openMap());
        entt::registry loadedRegistry;
        GameResources loadedResources;
        Map loadedMap{1, 1};
        runner.expect(
            deserializeGame(text, loadedRegistry, loadedResources, loadedMap),
            "trap save data loads");
        int triggeredTraps = 0;
        const auto traps = loadedRegistry.view<const Trap>();
        for (const auto entity : traps) {
            if (traps.get<const Trap>(entity).triggered) {
                ++triggeredTraps;
            }
        }
        runner.expect(triggeredTraps == 1, "trap state survives save/load");
    }

    {
        int selection = 0;
        const std::vector<InputEvent> enter{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(
            handleMainMenuInput(enter, false, MainMenuMode::Title, selection) ==
                MainMenuAction::NewGame,
            "main menu Enter starts a new game by default");

        const std::vector<InputEvent> quit{{InputEventType::KeyDown, Key::Q}};
        runner.expect(
            handleMainMenuInput(quit, true, MainMenuMode::Title, selection) ==
                MainMenuAction::Quit,
            "main menu q quits");
    }
}

} // namespace rl::test
