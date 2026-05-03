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

struct Item {};

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

struct Consumable {};

struct ProvidesHealing {
    int amount = 0;
};

struct Ranged {
    int range = 0;
};

struct AreaOfEffect {
    int radius = 0;
};

struct InflictsDamage {
    int amount = 0;
};

struct InBackpack {
    entt::entity owner = entt::null;
};

enum class EquipmentSlot { Weapon, Armor };

struct Equippable {
    EquipmentSlot slot = EquipmentSlot::Weapon;
};

struct Equipped {
    entt::entity owner = entt::null;
    EquipmentSlot slot = EquipmentSlot::Weapon;
};

struct MeleePowerBonus {
    int amount = 0;
};

struct DefenseBonus {
    int amount = 0;
};

struct WantsToPickupItem {
    entt::entity item = entt::null;
};

} // namespace rl
