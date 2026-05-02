#include <exception>
#include <iostream>
#include <string_view>

#include <entt/entt.hpp>

#include "core/Game.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "spawn/Spawner.hpp"
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

int runSmoke()
{
    if (!smokeConsoleBuffer()) {
        return 1;
    }
    if (!smokeEntityRendering()) {
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