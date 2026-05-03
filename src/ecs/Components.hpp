#pragma once

#include <string>
#include <vector>

#include <entt/entt.hpp>

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

struct Player {};

struct Monster {};

struct BlocksTile {};

struct Viewshed {
    std::vector<Point> visible;
    int range = 8;
    bool dirty = true;
};

struct CombatStats {
    int maxHp = 1;
    int hp = 1;
    int defense = 0;
    int power = 1;
};

struct WantsToMelee {
    entt::entity target = entt::null;
};

struct SufferDamage {
    int amount = 0;
};

} // namespace rl
