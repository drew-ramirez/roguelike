#pragma once

#include <string>
#include <vector>

#include "core/Point.hpp"
#include "render/Color.hpp"

namespace rl {

struct Position {
    int x = 0;
    int y = 0;
};

struct Renderable {
    char32_t glyph = U'?';
    Color foreground = Color::white();
    Color background = Color::black();
    int order = 0;
};

struct Name {
    std::string value;
};

struct Player {
};

struct BlocksTile {
};

struct Viewshed {
    std::vector<Point> visible;
    int range = 8;
    bool dirty = true;
};

} // namespace rl