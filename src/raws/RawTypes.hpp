/**
 * **************************************************************************
 * @file     RawTypes.hpp
 * @brief    Declares data-driven entity template structures
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ecs/Components.hpp"
#include "render/Color.hpp"
#include "spawn/SpawnTable.hpp"

namespace rl {

enum class RawEntityKind { Mob, Item, Prop };

struct RawRenderable {
    char32_t glyph = U'?';
    Color foreground = Color::white();
    Color background = Color::black();
    int order = 0;
};

struct RawCombatStats {
    int maxHp = 1;
    int hp = 1;
    int defense = 0;
    int power = 1;
};

struct RawAttributes {
    int might = 10;
    int fitness = 10;
    int quickness = 10;
    int intelligence = 10;
};

struct RawPools {
    int hitPoints = 1;
    int maxHitPoints = 1;
    int mana = 0;
    int maxMana = 0;
    int level = 1;
    int xp = 0;
    int gold = 0;
};

struct RawEffects {
    std::optional<int> providesHealing;
    std::optional<int> providesNutrition;
    bool magicMapper = false;
    std::optional<int> inflictsDamage;
    std::optional<int> areaOfEffect;
    bool townPortal = false;
    std::optional<int> confusionTurns;
    bool identify = false;
    bool removeCurse = false;
};

struct RawItemStats {
    int weight = 0;
    int baseValue = 0;
    int initiativePenalty = 0;
    std::string category;
};

struct RawWeaponStats {
    std::string damage = "1d4";
    int hitBonus = 0;
    std::string attribute = "might";
};

struct RawRangedWeaponStats {
    int range = 0;
    std::string damage = "1d4";
    int hitBonus = 0;
    std::string attribute = "quickness";
};

struct RawMagicItem {
    std::string tier = "common";
    std::string unidentifiedName;
    bool identified = true;
};

struct RawVendor {
    std::string category;
    int gold = 0;
};

struct RawEntityTemplate {
    std::string id;
    std::string name;
    RawEntityKind kind = RawEntityKind::Item;
    RawRenderable renderable;
    bool blocksTile = false;
    bool neutral = false;
    bool bystander = false;
    bool hidden = false;
    bool cursed = false;
    bool boss = false;
    bool grantsWin = false;
    std::string faction;
    std::optional<RawVendor> vendor;
    std::optional<RawCombatStats> combat;
    std::optional<RawAttributes> attributes;
    std::optional<RawPools> pools;
    std::optional<int> viewshedRange;
    std::optional<int> xpReward;
    RawEffects effects;
    std::optional<EquipmentSlot> equippable;
    std::optional<int> meleePowerBonus;
    std::optional<int> defenseBonus;
    std::optional<RawAttributes> attributeBonus;
    std::optional<RawItemStats> itemStats;
    std::optional<RawWeaponStats> weapon;
    std::optional<RawRangedWeaponStats> rangedWeapon;
    std::optional<RawMagicItem> magic;
    std::vector<std::string> equipped;
    std::vector<std::string> knownSpells;
    std::optional<std::string> spellbook;
    std::optional<int> manaCost;
    std::optional<int> breathDamage;
    std::optional<int> breathRange;
};

struct RawSpell {
    std::string id;
    std::string name;
    int manaCost = 0;
    RawEffects effects;
    std::optional<int> range;
};

struct RawFaction {
    std::string id;
    std::vector<std::string> hostileTo;
};

} // namespace rl
