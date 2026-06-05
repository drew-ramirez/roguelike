/**
 * **************************************************************************
 * @file     MasterDungeonMap.hpp
 * @brief    Declares cached map/entity snapshots for level backtracking
 * **************************************************************************
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "ecs/Components.hpp"
#include "map/Map.hpp"

namespace rl {

struct StoredEntity {
    std::optional<RawIdentity> rawIdentity;
    std::optional<Position> position;
    std::optional<Renderable> renderable;
    std::optional<Name> name;
    bool monster = false;
    bool item = false;
    bool blocksTile = false;
    bool hidden = false;
    bool consumable = false;
    bool magicMapper = false;
    bool neutral = false;
    bool bystander = false;
    bool prop = false;
    bool boss = false;
    bool grantsWin = false;
    std::optional<Viewshed> viewshed;
    std::optional<CombatStats> combatStats;
    std::optional<Attributes> attributes;
    std::optional<Pools> pools;
    std::optional<ProvidesHealing> providesHealing;
    std::optional<ProvidesNutrition> providesNutrition;
    std::optional<Ingredient> ingredient;
    std::optional<Meal> meal;
    bool providesIdentify = false;
    bool providesRemoveCurse = false;
    std::optional<Ranged> ranged;
    std::optional<AreaOfEffect> areaOfEffect;
    std::optional<InflictsDamage> inflictsDamage;
    std::optional<Equippable> equippable;
    std::optional<MeleePowerBonus> meleePowerBonus;
    std::optional<DefenseBonus> defenseBonus;
    std::optional<AttributeBonus> attributeBonus;
    std::optional<Trap> trap;
    std::optional<XpReward> xpReward;
    std::optional<ItemStats> itemStats;
    std::optional<WeaponStats> weaponStats;
    std::optional<RangedWeapon> rangedWeapon;
    std::optional<MagicItem> magicItem;
    bool identifiedItem = false;
    bool cursedItem = false;
    std::optional<Vendor> vendor;
    std::optional<CampSite> campSite;
    std::optional<SettlementService> settlementService;
    std::optional<CookingStation> cookingStation;
    std::optional<Faction> faction;
    bool townPortal = false;
    std::optional<Confusion> confusion;
    std::optional<KnownSpells> knownSpells;
    std::optional<Spellbook> spellbook;
    std::optional<ManaCost> manaCost;
    std::optional<BreathWeapon> breathWeapon;
};

struct DungeonLevelState {
    int depth = 1;
    std::string builderId;
    Map map{1, 1, TileType::Wall};
    std::vector<StoredEntity> entities;
};

class MasterDungeonMap {
  public:
    void storeLevel(int depth,
                    std::string builderId,
                    const Map& map,
                    const entt::registry& registry,
                    entt::entity player);
    [[nodiscard]] bool hasLevel(int depth) const;
    [[nodiscard]] const DungeonLevelState* level(int depth) const;
    void restoreLevel(int depth, Map& map, entt::registry& registry) const;
    void clear();

    [[nodiscard]] const std::map<int, DungeonLevelState>& levels() const;
    void replaceLevels(std::map<int, DungeonLevelState> levels);

  private:
    std::map<int, DungeonLevelState> levels_;
};

[[nodiscard]] bool isPlayerPersistentEntity(const entt::registry& registry,
                                            entt::entity player,
                                            entt::entity entity);

[[nodiscard]] std::vector<StoredEntity>
captureLevelLocalEntities(const entt::registry& registry, entt::entity player);
void restoreStoredEntities(entt::registry& registry,
                           const std::vector<StoredEntity>& entities);
void destroyLevelLocalEntities(entt::registry& registry, entt::entity player);

} // namespace rl
