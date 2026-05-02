#include <array>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

#include "core/Game.hpp"
#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "map/Rect.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "render/ConsoleBuffer.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/RenderSystem.hpp"

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
    std::vector<bool> visited(static_cast<std::size_t>(map.width() * map.height()), false);
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

            const auto index = static_cast<std::size_t>(map.xyIdx(next.x, next.y));
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
    if (room.width() != 4 || room.height() != 6 || room.center() != rl::Point{3, 5}) {
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

    if (!sameMapTiles(first.map, second.map) || first.playerStart != second.playerStart) {
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

bool smokeEntityRendering()
{
    entt::registry registry;
    const auto player = rl::spawnPlayer(registry, rl::Point{3, 2});

    if (!registry.all_of<rl::Position, rl::Renderable, rl::Name, rl::Player, rl::BlocksTile>(player)) {
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
    if (!map.blocksMovement(0, 0) || !map.blocksMovement(7, 5) || !map.isWalkable(1, 1)) {
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
    if (!smokeEntityRendering()) {
        return 1;
    }
    if (!smokeMapAndMovement()) {
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

        rl::Game game;
        game.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}