#pragma once

#include <cstdint>
#include <optional>
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

struct RawIdentity {
    std::string templateName;
};

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

struct Attributes {
    int might = 10;
    int fitness = 10;
    int quickness = 10;
    int intelligence = 10;
};

struct Pools {
    int hitPoints = 1;
    int maxHitPoints = 1;
    int mana = 0;
    int maxMana = 0;
    int level = 1;
    int xp = 0;
    int gold = 0;
};

struct WantsToMelee {
    entt::entity target = entt::null;
};

struct SufferDamage {
    int amount = 0;
};

struct Particle {};

struct ParticleLifetime {
    int remainingFrames = 0;
};

struct SaveEntityId {
    std::uint32_t value = 0;
};

struct Consumable {};

struct ProvidesHealing {
    int amount = 0;
};

enum class HungerState { WellFed, Normal, Hungry, Starving };

struct HungerClock {
    HungerState state = HungerState::Normal;
    int duration = 600;
};

struct ProvidesNutrition {
    int amount = 300;
};

struct Ingredient {
    std::vector<std::string> tags;
    std::string originBiome;
    std::string originRegionId;
    int potency = 1;
    int risk = 0;
};

struct Meal {
    std::string recipeId;
};

struct MagicMapper {};

struct Ranged {
    int range = 0;
};

struct AreaOfEffect {
    int radius = 0;
};

struct InflictsDamage {
    int amount = 0;
};

struct Neutral {};

struct Bystander {};

struct Vendor {
    std::string category;
    int gold = 0;
};

struct Prop {};

struct CampSite {
    std::string campType;
    int safetyBonus = 0;
};

struct SettlementService {
    std::string serviceId;
    std::string recipeId;
};

struct Shrine {
    std::string powerId;
    std::vector<std::string> acceptedOfferingTags;
    int blessingHp = 0;
};

struct CookingStation {
    std::string stationId;
    std::vector<std::string> methods;
    std::vector<std::string> placeTags;
};

struct Faction {
    std::string id;
};

struct XpReward {
    int amount = 0;
};

struct ItemStats {
    int weight = 0;
    int baseValue = 0;
    int initiativePenalty = 0;
    std::string category;
};

struct WeaponStats {
    std::string damage = "1d4";
    int hitBonus = 0;
    std::string attribute = "might";
};

struct RangedWeapon {
    int range = 0;
    std::string damage = "1d4";
    int hitBonus = 0;
    std::string attribute = "quickness";
};

struct MagicItem {
    std::string tier = "common";
    std::string unidentifiedName;
    bool identified = true;
};

struct IdentifiedItem {};

struct CursedItem {};

struct TownPortal {};

struct ProvidesIdentify {};

struct ProvidesRemoveCurse {};

struct Confusion {
    int turns = 0;
};

struct KnownSpells {
    std::vector<std::string> ids;
};

struct Spellbook {
    std::string spellId;
};

struct ManaCost {
    int amount = 0;
};

struct Boss {};

struct BreathWeapon {
    int range = 0;
    int damage = 0;
    int cooldown = 0;
};

struct GrantsWin {};

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

struct AttributeBonus {
    int might = 0;
    int fitness = 0;
    int quickness = 0;
    int intelligence = 0;
};

struct WantsToPickupItem {
    entt::entity item = entt::null;
};

struct WantsToUseItem {
    entt::entity item = entt::null;
    std::optional<Point> target;
};

struct WantsToDropItem {
    entt::entity item = entt::null;
};

struct WantsToCastSpell {
    std::string spellId;
    std::optional<Point> target;
};

struct WantsToShoot {
    entt::entity weapon = entt::null;
    std::optional<Point> target;
    bool breath = false;
    int damage = 0;
    int radius = 0;
};

struct Trap {
    int damage = 3;
    bool triggered = false;
};

struct Hidden {};

} // namespace rl
