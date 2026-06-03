#include "save/SaveGame.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "map/MasterDungeonMap.hpp"
#include "map/TileType.hpp"
#include "save/SaveDungeon.hpp"
#include "save/SaveEntities.hpp"
#include "save/SaveMap.hpp"
#include "save/SaveResources.hpp"
#include "save/SaveTypes.hpp"

namespace rl {
namespace {

using save::colorFromJson;
using save::colorToJson;
using save::currentSaveVersion;
using save::dungeonFromJson;
using save::dungeonToJson;
using save::ensureSaveIds;
using save::EntityId;
using save::entityId;
using save::entityToJson;
using save::hungerStateFromString;
using save::hungerStateToString;
using save::Json;
using save::loadResources;
using save::mapFromJson;
using save::mapToJson;
using save::pointToJson;
using save::resourcesToJson;
using save::slotFromString;
using save::slotToString;
void loadEntityImmediateComponents(entt::registry& registry,
                                   entt::entity entity,
                                   EntityId id,
                                   const Json& components)
{
    registry.emplace<SaveEntityId>(entity, SaveEntityId{id});
    if (components.contains("position")) {
        const auto& json = components.at("position");
        registry.emplace<Position>(
            entity, Position{json.at("x").get<int>(), json.at("y").get<int>()});
    }
    if (components.contains("renderable")) {
        const auto& json = components.at("renderable");
        registry.emplace<Renderable>(
            entity,
            Renderable{
                static_cast<char32_t>(json.at("glyph").get<std::uint32_t>()),
                colorFromJson(json.at("foreground")),
                colorFromJson(json.at("background")),
                json.at("order").get<int>()});
    }
    if (components.contains("name")) {
        registry.emplace<Name>(entity,
                               Name{components.at("name").get<std::string>()});
    }
    if (components.contains("raw_identity")) {
        registry.emplace<RawIdentity>(
            entity,
            RawIdentity{components.at("raw_identity").get<std::string>()});
    }
    if (components.contains("player")) {
        registry.emplace<Player>(entity);
    }
    if (components.contains("monster")) {
        registry.emplace<Monster>(entity);
    }
    if (components.contains("item")) {
        registry.emplace<Item>(entity);
    }
    if (components.contains("blocks_tile")) {
        registry.emplace<BlocksTile>(entity);
    }
    if (components.contains("viewshed")) {
        const auto& json = components.at("viewshed");
        registry.emplace<Viewshed>(
            entity, Viewshed{{}, json.at("range").get<int>(), true});
    }
    if (components.contains("combat_stats")) {
        const auto& json = components.at("combat_stats");
        registry.emplace<CombatStats>(entity,
                                      CombatStats{json.at("max_hp").get<int>(),
                                                  json.at("hp").get<int>(),
                                                  json.at("defense").get<int>(),
                                                  json.at("power").get<int>()});
    }
    if (components.contains("attributes")) {
        const auto& json = components.at("attributes");
        registry.emplace<Attributes>(
            entity,
            Attributes{json.value("might", 10),
                       json.value("fitness", 10),
                       json.value("quickness", 10),
                       json.value("intelligence", 10)});
    }
    if (components.contains("pools")) {
        const auto& json = components.at("pools");
        registry.emplace<Pools>(entity,
                                Pools{json.value("hit_points", 1),
                                      json.value("max_hit_points", 1),
                                      json.value("mana", 0),
                                      json.value("max_mana", 0),
                                      json.value("level", 1),
                                      json.value("xp", 0),
                                      json.value("gold", 0)});
    }
    if (components.contains("consumable")) {
        registry.emplace<Consumable>(entity);
    }
    if (components.contains("provides_healing")) {
        registry.emplace<ProvidesHealing>(
            entity,
            ProvidesHealing{components.at("provides_healing").get<int>()});
    }
    if (components.contains("hunger_clock")) {
        const auto& json = components.at("hunger_clock");
        registry.emplace<HungerClock>(
            entity,
            HungerClock{
                hungerStateFromString(json.at("state").get<std::string>()),
                json.at("duration").get<int>()});
    }
    if (components.contains("provides_nutrition")) {
        registry.emplace<ProvidesNutrition>(
            entity,
            ProvidesNutrition{components.at("provides_nutrition").get<int>()});
    }
    if (components.contains("magic_mapper")) {
        registry.emplace<MagicMapper>(entity);
    }
    if (components.contains("ranged")) {
        registry.emplace<Ranged>(entity,
                                 Ranged{components.at("ranged").get<int>()});
    }
    if (components.contains("area_of_effect")) {
        registry.emplace<AreaOfEffect>(
            entity, AreaOfEffect{components.at("area_of_effect").get<int>()});
    }
    if (components.contains("inflicts_damage")) {
        registry.emplace<InflictsDamage>(
            entity,
            InflictsDamage{components.at("inflicts_damage").get<int>()});
    }
    if (components.contains("xp_reward")) {
        registry.emplace<XpReward>(
            entity, XpReward{components.at("xp_reward").get<int>()});
    }
    if (components.contains("item_stats")) {
        const auto& json = components.at("item_stats");
        registry.emplace<ItemStats>(
            entity,
            ItemStats{json.value("weight", 0),
                      json.value("base_value", 0),
                      json.value("initiative_penalty", 0),
                      json.value("category", std::string{})});
    }
    if (components.contains("weapon_stats")) {
        const auto& json = components.at("weapon_stats");
        registry.emplace<WeaponStats>(
            entity,
            WeaponStats{json.value("damage", std::string{"1d4"}),
                        json.value("hit_bonus", 0),
                        json.value("attribute", std::string{"might"})});
    }
    if (components.contains("ranged_weapon")) {
        const auto& json = components.at("ranged_weapon");
        registry.emplace<RangedWeapon>(
            entity,
            RangedWeapon{json.value("range", 0),
                         json.value("damage", std::string{"1d4"}),
                         json.value("hit_bonus", 0),
                         json.value("attribute", std::string{"quickness"})});
    }
    if (components.contains("equippable")) {
        registry.emplace<Equippable>(
            entity,
            Equippable{slotFromString(
                components.at("equippable").get<std::string>())});
    }
    if (components.contains("melee_power_bonus")) {
        registry.emplace<MeleePowerBonus>(
            entity,
            MeleePowerBonus{components.at("melee_power_bonus").get<int>()});
    }
    if (components.contains("defense_bonus")) {
        registry.emplace<DefenseBonus>(
            entity, DefenseBonus{components.at("defense_bonus").get<int>()});
    }
    if (components.contains("trap")) {
        const auto& json = components.at("trap");
        registry.emplace<Trap>(entity,
                               Trap{json.at("damage").get<int>(),
                                    json.at("triggered").get<bool>()});
    }
    if (components.contains("hidden")) {
        registry.emplace<Hidden>(entity);
    }
}

void loadEntityReferences(
    entt::registry& registry,
    entt::entity entity,
    const Json& components,
    const std::unordered_map<EntityId, entt::entity>& entities)
{
    if (components.contains("in_backpack")) {
        const auto ownerId = components.at("in_backpack").get<EntityId>();
        if (const auto owner = entities.find(ownerId);
            owner != entities.end()) {
            registry.emplace<InBackpack>(entity, InBackpack{owner->second});
        }
    }
    if (components.contains("equipped")) {
        const auto& json = components.at("equipped");
        const auto ownerId = json.at("owner").get<EntityId>();
        const auto owner = entities.find(ownerId);
        if (owner != entities.end()) {
            registry.emplace<Equipped>(
                entity,
                Equipped{owner->second,
                         slotFromString(json.at("slot").get<std::string>())});
        }
    }
}

} // namespace

std::filesystem::path defaultSavePath()
{
    return std::filesystem::path{"savegame.json"};
}

std::string serializeGame(entt::registry& registry,
                          const GameResources& resources,
                          const Map& map)
{
    ensureSaveIds(registry);

    Json entities = Json::array();
    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        entities.push_back(entityToJson(registry, entity));
    }

    Json root{{"version", currentSaveVersion},
              {"map", mapToJson(map)},
              {"resources", resourcesToJson(registry, resources)},
              {"dungeon", dungeonToJson(resources.dungeon)},
              {"entities", std::move(entities)}};
    return root.dump(2);
}

bool deserializeGame(std::string_view text,
                     entt::registry& registry,
                     GameResources& resources,
                     Map& map)
{
    try {
        const Json root = Json::parse(text);
        const int version = root.value("version", 0);
        if (version < 1 || version > currentSaveVersion) {
            return false;
        }

        Map loadedMap = mapFromJson(root.at("map"));
        entt::registry loadedRegistry;
        std::unordered_map<EntityId, entt::entity> entityMap;
        std::vector<std::pair<entt::entity, Json>> pendingReferences;

        for (const auto& saved : root.at("entities")) {
            const auto id = saved.at("id").get<EntityId>();
            if (id == 0 || entityMap.contains(id)) {
                return false;
            }
            const auto entity = loadedRegistry.create();
            const auto& components = saved.at("components");
            loadEntityImmediateComponents(
                loadedRegistry, entity, id, components);
            pendingReferences.emplace_back(entity, components);
            entityMap.emplace(id, entity);
        }

        for (const auto& [entity, components] : pendingReferences) {
            loadEntityReferences(loadedRegistry, entity, components, entityMap);
        }

        GameResources loadedResources;
        loadResources(root.at("resources"), entityMap, loadedResources);
        loadedResources.dungeon.replaceLevels(
            dungeonFromJson(root.value("dungeon", Json::array())));

        map = std::move(loadedMap);
        registry = std::move(loadedRegistry);
        resources = std::move(loadedResources);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void saveGame(const std::filesystem::path& path,
              entt::registry& registry,
              const GameResources& resources,
              const Map& map)
{
    std::ofstream output{path};
    if (!output) {
        throw std::runtime_error("Unable to open save file for writing");
    }
    output << serializeGame(registry, resources, map);
}

bool loadGame(const std::filesystem::path& path,
              entt::registry& registry,
              GameResources& resources,
              Map& map)
{
    std::ifstream input{path};
    if (!input) {
        return false;
    }

    const std::string text{std::istreambuf_iterator<char>{input},
                           std::istreambuf_iterator<char>{}};
    return deserializeGame(text, registry, resources, map);
}

} // namespace rl
