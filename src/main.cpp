#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "core/Game.hpp"
#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "debug/MapgenViewer.hpp"
#include "ecs/Components.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "map/Map.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/Rect.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/MapTheme.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"
#include "systems/MonsterAI.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/SpatialIndexSystem.hpp"
#include "systems/VisibilitySystem.hpp"
#include "ui/Hud.hpp"
#include "world/WorldGeneration.hpp"

namespace {

bool hasArgument(int argc, char* argv[], std::string_view expected)
{
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == expected) {
            return true;
        }
    }
    return false;
}

rl::Rng::Seed parseSeedValue(std::string_view value)
{
    if (value.empty()) {
        throw std::invalid_argument("Seed value cannot be empty");
    }

    std::uint64_t parsed = 0;
    const auto* const begin = value.data();
    const auto* const end = value.data() + value.size();
    const auto [next, error] = std::from_chars(begin, end, parsed);
    if (error != std::errc{} || next != end) {
        throw std::invalid_argument("Seed must be an unsigned integer");
    }
    if (parsed > std::numeric_limits<rl::Rng::Seed>::max()) {
        throw std::out_of_range("Seed is larger than the supported range");
    }

    return static_cast<rl::Rng::Seed>(parsed);
}

std::optional<rl::Rng::Seed> parseSeedArgument(int argc, char* argv[])
{
    constexpr std::string_view prefix = "--seed=";
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == "--seed") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("Missing value for --seed");
            }
            return parseSeedValue(argv[i + 1]);
        }
        if (argument.starts_with(prefix)) {
            return parseSeedValue(argument.substr(prefix.size()));
        }
    }

    return std::nullopt;
}

std::optional<std::string_view>
parseStringArgument(int argc, char* argv[], std::string_view name)
{
    const std::string prefix = std::string{name} + "=";
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == name) {
            if (i + 1 >= argc) {
                throw std::invalid_argument("Missing value for " +
                                            std::string{name});
            }
            return std::string_view{argv[i + 1]};
        }
        if (argument.starts_with(prefix)) {
            return argument.substr(prefix.size());
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> parseBuilderArgument(int argc, char* argv[])
{
    return parseStringArgument(argc, argv, "--builder");
}

std::optional<rl::BiomeVariant> parseBiomeArgument(int argc, char* argv[])
{
    const auto biome = parseStringArgument(argc, argv, "--biome");
    if (!biome.has_value()) {
        return std::nullopt;
    }
    const auto parsed = rl::biomeVariantFromId(*biome);
    if (rl::biomeVariantId(parsed) != *biome) {
        throw std::invalid_argument("Unknown biome: " + std::string{*biome});
    }
    return parsed;
}

int parsePositiveIntValue(std::string_view value, std::string_view name)
{
    if (value.empty()) {
        throw std::invalid_argument(std::string{name} + " cannot be empty");
    }

    int parsed = 0;
    const auto* const begin = value.data();
    const auto* const end = value.data() + value.size();
    const auto [next, error] = std::from_chars(begin, end, parsed);
    if (error != std::errc{} || next != end || parsed <= 0) {
        throw std::invalid_argument(std::string{name} +
                                    " must be a positive integer");
    }
    return parsed;
}

int parsePositiveIntArgument(int argc,
                             char* argv[],
                             std::string_view name,
                             int fallback)
{
    if (const auto value = parseStringArgument(argc, argv, name)) {
        return parsePositiveIntValue(*value, name);
    }
    return fallback;
}

bool sameMapTiles(const rl::Map& left, const rl::Map& right)
{
    if (left.width() != right.width() || left.height() != right.height()) {
        return false;
    }

    for (int y = 0; y < left.height(); ++y) {
        for (int x = 0; x < left.width(); ++x) {
            if (left.tileAt(x, y) != right.tileAt(x, y)) {
                return false;
            }
        }
    }

    return true;
}

int countWalkableTiles(const rl::Map& map)
{
    int count = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.isWalkable(x, y)) {
                ++count;
            }
        }
    }
    return count;
}

bool allWalkableTilesReachable(const rl::Map& map, rl::Point start)
{
    if (!map.isWalkable(start.x, start.y)) {
        return false;
    }

    const int totalWalkable = countWalkableTiles(map);
    int reached = 0;
    std::vector<bool> visited(
        static_cast<std::size_t>(map.width() * map.height()), false);
    std::vector<rl::Point> frontier{start};
    visited[static_cast<std::size_t>(map.xyIdx(start.x, start.y))] = true;

    constexpr std::array<rl::Point, 4> neighbors{
        rl::Point{1, 0},
        rl::Point{-1, 0},
        rl::Point{0, 1},
        rl::Point{0, -1},
    };

    while (!frontier.empty()) {
        const rl::Point current = frontier.back();
        frontier.pop_back();
        ++reached;

        for (const rl::Point delta : neighbors) {
            const rl::Point next{current.x + delta.x, current.y + delta.y};
            if (!map.isWalkable(next.x, next.y)) {
                continue;
            }

            const auto index =
                static_cast<std::size_t>(map.xyIdx(next.x, next.y));
            if (!visited[index]) {
                visited[index] = true;
                frontier.push_back(next);
            }
        }
    }

    return reached == totalWalkable;
}

bool smokeConsoleBuffer()
{
    rl::ConsoleBuffer console{16, 4};
    console.clear();
    console.print(0, 0, "Hello Roguelike", rl::Color::yellow());
    console.set(0, 1, U'@', rl::Color::cyan());

    if (console.cellAt(0, 0).glyph != U'H') {
        return false;
    }
    if (console.cellAt(0, 1).glyph != U'@') {
        return false;
    }
    return true;
}

bool smokeRect()
{
    const rl::Rect room{1, 2, 5, 8};
    if (room.width() != 4 || room.height() != 6 ||
        room.center() != rl::Point{3, 5}) {
        return false;
    }
    if (!room.intersects(rl::Rect{4, 7, 8, 10})) {
        return false;
    }
    return !room.intersects(rl::Rect{5, 8, 9, 12});
}

bool smokeGeneratedMap()
{
    rl::Rng firstRng{12345U};
    rl::Rng secondRng{12345U};
    auto first = rl::SimpleMapBuilder::build(40, 25, firstRng);
    auto second = rl::SimpleMapBuilder::build(40, 25, secondRng);

    if (!sameMapTiles(first.map, second.map) ||
        first.playerStart != second.playerStart) {
        return false;
    }
    if (first.rooms.size() < 2) {
        return false;
    }
    if (!first.map.isWalkable(first.playerStart.x, first.playerStart.y)) {
        return false;
    }
    return allWalkableTilesReachable(first.map, first.playerStart);
}

bool smokeBspGeneratedMaps()
{
    rl::Rng bspRng{222U};
    auto bsp = rl::BspDungeonBuilder::build(40, 25, bspRng);
    if (!allWalkableTilesReachable(bsp.map, bsp.playerStart) ||
        bsp.builderName != "bsp") {
        return false;
    }

    rl::Rng interiorRng{333U};
    auto interior = rl::BspInteriorBuilder::build(40, 25, interiorRng);
    return allWalkableTilesReachable(interior.map, interior.playerStart) &&
           interior.builderName == "bsp-interior";
}

bool smokeVisibility()
{
    rl::Map map{12, 8};
    map.setTile(3, 1, rl::TileType::Wall);

    entt::registry registry;
    const auto player = rl::spawnPlayer(registry, rl::Point{1, 1});
    rl::ShadowcastingFov fov;
    rl::updateVisibility(registry, map, player, fov);

    if (!map.isVisible(1, 1) || !map.isVisible(3, 1) || map.isVisible(4, 1)) {
        return false;
    }
    if (!map.isRevealed(1, 1)) {
        return false;
    }

    map.clearVisible();
    return map.isRevealed(1, 1) && !map.isVisible(1, 1);
}

bool smokeVisibleEntityRendering()
{
    rl::Map map{8, 6};
    map.setVisible(1, 1);

    entt::registry registry;
    [[maybe_unused]] const auto player =
        rl::spawnPlayer(registry, rl::Point{1, 1});
    [[maybe_unused]] const auto goblin =
        rl::spawnGoblin(registry, rl::Point{2, 1});

    rl::ConsoleBuffer console{8, 6};
    console.clear();
    rl::renderEntities(registry, map, console);
    if (console.cellAt(1, 1).glyph != U'@' ||
        console.cellAt(2, 1).glyph != U' ') {
        return false;
    }

    map.setVisible(2, 1);
    console.clear();
    rl::renderEntities(registry, map, console);
    return console.cellAt(2, 1).glyph == U'g' ||
           console.cellAt(2, 1).glyph == U'o';
}

bool smokeSeedParsing()
{
    char executable[] = "roguelike";
    char seedFlag[] = "--seed";
    char seedValue[] = "12345";
    char seedEquals[] = "--seed=67890";
    char* splitSeedArgs[] = {executable, seedFlag, seedValue};
    char* equalsSeedArgs[] = {executable, seedEquals};
    char* noSeedArgs[] = {executable};

    const auto splitSeed = parseSeedArgument(3, splitSeedArgs);
    const auto equalsSeed = parseSeedArgument(2, equalsSeedArgs);
    const auto noSeed = parseSeedArgument(1, noSeedArgs);

    return splitSeed == 12345U && equalsSeed == 67890U && !noSeed.has_value();
}

bool smokeMapThemeParsing()
{
    char executable[] = "roguelike";
    char themeFlag[] = "--map-theme";
    char themeValue[] = "druidic";
    char themeEquals[] = "--map-theme=druidic";
    char* splitThemeArgs[] = {executable, themeFlag, themeValue};
    char* equalsThemeArgs[] = {executable, themeEquals};
    char* noThemeArgs[] = {executable};

    const auto splitTheme =
        parseStringArgument(3, splitThemeArgs, "--map-theme");
    const auto equalsTheme =
        parseStringArgument(2, equalsThemeArgs, "--map-theme");
    const auto noTheme = parseStringArgument(1, noThemeArgs, "--map-theme");

    return splitTheme.has_value() && *splitTheme == "druidic" &&
           equalsTheme.has_value() && *equalsTheme == "druidic" &&
           !noTheme.has_value();
}

bool smokeEntityRendering()
{
    entt::registry registry;
    const auto player = rl::spawnPlayer(registry, rl::Point{3, 2});

    if (!registry.all_of<rl::Position,
                         rl::Renderable,
                         rl::Name,
                         rl::Player,
                         rl::BlocksTile>(player)) {
        return false;
    }

    rl::ConsoleBuffer console{16, 4};
    console.clear();
    rl::renderEntities(registry, console);
    if (console.cellAt(3, 2).glyph != U'@') {
        return false;
    }

    registry.remove<rl::Renderable>(player);
    console.clear();
    rl::renderEntities(registry, console);
    return console.cellAt(3, 2).glyph == U' ';
}

bool smokeMapAndMovement()
{
    rl::Map map = rl::Map::createStarter(8, 6);
    if (!map.blocksMovement(0, 0) || !map.blocksMovement(7, 5) ||
        !map.isWalkable(1, 1)) {
        return false;
    }

    entt::registry registry;
    rl::GameResources resources;
    resources.player = rl::spawnPlayer(registry, rl::Point{1, 1});

    if (!rl::tryMoveEntity(registry, resources.player, map, rl::Point{1, 0})) {
        return false;
    }

    const auto& moved = registry.get<rl::Position>(resources.player);
    if (moved.x != 2 || moved.y != 1) {
        return false;
    }

    if (rl::tryMoveEntity(registry, resources.player, map, rl::Point{0, -1})) {
        return false;
    }

    const auto& blocked = registry.get<rl::Position>(resources.player);
    return blocked.x == 2 && blocked.y == 1;
}

bool smokeMonsterSpawning()
{
    rl::Rng rng{12345U};
    auto build = rl::SimpleMapBuilder::build(40, 25, rng);

    entt::registry registry;
    [[maybe_unused]] const auto player =
        rl::spawnPlayer(registry, build.playerStart);
    const auto spawned = rl::spawnMonsters(
        registry, build.spawnRegions, build.map, rng, build.playerStart);
    if (spawned < 2) {
        return false;
    }

    std::vector<rl::Point> occupied;
    const auto view = registry.view<const rl::Monster, const rl::Position>();
    for (const auto entity : view) {
        const auto& position = view.get<const rl::Position>(entity);
        const rl::Point point{position.x, position.y};
        if (point == build.playerStart ||
            !build.map.isWalkable(point.x, point.y)) {
            return false;
        }
        if (std::find(occupied.begin(), occupied.end(), point) !=
            occupied.end()) {
            return false;
        }
        occupied.push_back(point);
    }

    return occupied.size() == spawned;
}

bool smokeCombatAndDeath()
{
    rl::Map map = rl::Map::createStarter(8, 6);
    entt::registry registry;
    rl::GameResources resources;
    resources.player = rl::spawnPlayer(registry, rl::Point{1, 1});
    const auto goblin = rl::spawnGoblin(registry, rl::Point{2, 1});

    const std::vector<rl::InputEvent> attackInput{
        {rl::InputEventType::KeyDown, rl::Key::Right}};
    if (!rl::handlePlayerInput(registry, resources, map, attackInput)) {
        return false;
    }

    const auto& playerPosition = registry.get<rl::Position>(resources.player);
    if (playerPosition.x != 1 || playerPosition.y != 1 ||
        !registry.all_of<rl::WantsToMelee>(resources.player)) {
        return false;
    }

    rl::RunState state = rl::RunState::PlayerTurn;
    rl::processMeleeCombat(registry, resources.log);
    rl::applyDamageAndCleanup(registry, resources, state);
    if (!registry.valid(goblin)) {
        return false;
    }

    registry.emplace_or_replace<rl::WantsToMelee>(resources.player,
                                                  rl::WantsToMelee{goblin});
    rl::processMeleeCombat(registry, resources.log);
    rl::applyDamageAndCleanup(registry, resources, state);
    if (registry.valid(goblin) ||
        rl::blockingEntityAt(registry, rl::Point{2, 1}).has_value()) {
        return false;
    }

    entt::registry monsterRegistry;
    rl::GameResources monsterResources;
    monsterResources.player = rl::spawnPlayer(monsterRegistry, rl::Point{1, 1});
    [[maybe_unused]] const auto attackingGoblin =
        rl::spawnGoblin(monsterRegistry, rl::Point{2, 1});
    auto& playerStats =
        monsterRegistry.get<rl::CombatStats>(monsterResources.player);
    playerStats.hp = 1;
    playerStats.defense = 0;

    rl::RunState monsterState = rl::RunState::MonsterTurn;
    rl::runMonsterAi(monsterRegistry, monsterResources, map);
    rl::processMeleeCombat(monsterRegistry, monsterResources.log);
    rl::applyDamageAndCleanup(monsterRegistry, monsterResources, monsterState);
    return monsterState == rl::RunState::GameOver;
}

bool smokeHudAndLog()
{
    rl::ConsoleBuffer console{80, 50};
    entt::registry registry;
    rl::GameResources resources;
    resources.player = rl::spawnPlayer(registry, rl::Point{1, 1});

    resources.log.add("one");
    resources.log.add("two");
    resources.log.add("three");
    resources.log.add("four");
    resources.log.add("five");
    resources.log.add("six");

    const auto recent = resources.log.recent(5);
    if (recent.size() != 5 || recent.front() != "two" ||
        recent.back() != "six") {
        return false;
    }

    console.clear();
    rl::drawHud(console, resources, registry, rl::RunState::AwaitingInput);
    return console.cellAt(1, 44).glyph == U'H' &&
           console.cellAt(1, 45).glyph == U'M' &&
           console.cellAt(28, 47).glyph == U'f';
}

int runSmoke()
{
    if (!smokeConsoleBuffer()) {
        return 1;
    }
    if (!smokeRect()) {
        return 1;
    }
    if (!smokeGeneratedMap()) {
        return 1;
    }
    if (!smokeBspGeneratedMaps()) {
        return 1;
    }
    if (!smokeVisibility()) {
        return 1;
    }
    if (!smokeVisibleEntityRendering()) {
        return 1;
    }
    if (!smokeSeedParsing()) {
        return 1;
    }
    if (!smokeMapThemeParsing()) {
        return 1;
    }
    if (!smokeEntityRendering()) {
        return 1;
    }
    if (!smokeMapAndMovement()) {
        return 1;
    }
    if (!smokeMonsterSpawning()) {
        return 1;
    }
    if (!smokeCombatAndDeath()) {
        return 1;
    }
    if (!smokeHudAndLog()) {
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        if (hasArgument(argc, argv, "--smoke")) {
            return runSmoke();
        }

        const bool mapgenMode = hasArgument(argc, argv, "--mapgen");
        const bool galleryMode = hasArgument(argc, argv, "--gallery");
        const bool loadExisting = hasArgument(argc, argv, "--load");
        const bool visualMapgen = hasArgument(argc, argv, "--visual-mapgen");
        if (galleryMode && (mapgenMode || loadExisting || visualMapgen)) {
            throw std::invalid_argument(
                "--gallery cannot be combined with --mapgen, --load, or "
                "--visual-mapgen");
        }

        if (mapgenMode) {
            rl::MapgenViewerOptions options;
            if (const auto builder = parseBuilderArgument(argc, argv)) {
                options.builderId = std::string{*builder};
            }
            options.seed = parseSeedArgument(argc, argv);
            options.showSnapshots = hasArgument(argc, argv, "--snapshots");
            options.width = parsePositiveIntArgument(argc, argv, "--width", 40);
            options.height =
                parsePositiveIntArgument(argc, argv, "--height", 25);
            options.biomeVariant = parseBiomeArgument(argc, argv);
            return rl::runMapgenViewer(options, std::cout);
        }

        const auto requestedBuilder = parseBuilderArgument(argc, argv);
        const std::string galleryBuilderId =
            galleryMode && requestedBuilder.has_value()
                ? std::string{*requestedBuilder}
                : "simple";
        const std::string builderId =
            !galleryMode && requestedBuilder.has_value()
                ? std::string{*requestedBuilder}
                : "normal";
        if (builderId != "normal" && !rl::isKnownMapBuilder(builderId)) {
            throw std::invalid_argument("Unknown map builder: " + builderId);
        }
        if (galleryMode && galleryBuilderId == "normal") {
            throw std::invalid_argument(
                "Gallery requires a registered builder, not normal routing");
        }
        if (galleryMode && !rl::isKnownMapBuilder(galleryBuilderId)) {
            throw std::invalid_argument("Unknown map builder: " +
                                        galleryBuilderId);
        }

        const auto galleryBiome =
            galleryMode ? parseBiomeArgument(argc, argv) : std::nullopt;

        std::string mapThemeId = "default";
        if (const auto mapTheme =
                parseStringArgument(argc, argv, "--map-theme")) {
            mapThemeId = std::string{*mapTheme};
        }

        rl::Game game{parseSeedArgument(argc, argv),
                      loadExisting,
                      builderId,
                      parsePositiveIntArgument(argc, argv, "--width", 80),
                      parsePositiveIntArgument(argc, argv, "--height", 43),
                      visualMapgen,
                      rl::mapThemeById(mapThemeId),
                      galleryMode,
                      galleryBiome.value_or(rl::BiomeVariant::DryWoods),
                      galleryBuilderId};
        game.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
