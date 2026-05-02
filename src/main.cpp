#include <exception>
#include <iostream>
#include <string_view>

#include "core/Game.hpp"
#include "render/ConsoleBuffer.hpp"

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

int runSmoke()
{
    rl::ConsoleBuffer console{16, 4};
    console.clear();
    console.print(0, 0, "Hello Roguelike", rl::Color::yellow());
    console.set(0, 1, U'@', rl::Color::cyan());

    if (console.cellAt(0, 0).glyph != U'H') {
        return 1;
    }
    if (console.cellAt(0, 1).glyph != U'@') {
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
