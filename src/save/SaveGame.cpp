#include "save/SaveGame.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
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
#include "map/TileType.hpp"

namespace rl {
namespace {

using Json = nlohmann::json;
using EntityId = std::uint32_t;

[[nodiscard]] std::string tileToString(TileType tile)
{
    switch (tile) {
    case TileType::Wall:
        return "wall";
    case TileType::Floor:
        return "floor";
    }
    return "wall";
}

[[nodiscard]] TileType tileFromString(const std::string& value)
{
    if (value == "floor") {
        return TileType::Floor;
    }
    return TileType::Wall;
}

[[nodiscard]] std::string slotToString(EquipmentSlot slot)
{
    switch (slot) {
    case EquipmentSlot::Weapon:
        return "weapon";
    case EquipmentSlot::Armor:
        return "armor";
    }
    return "weapon";
}

[[nodiscard]] EquipmentSlot slotFromString(const std::string& value)
{
    if (value == "armor") {
        return EquipmentSlot::Armor;
    }
    return EquipmentSlot::Weapon;
}

[[nodiscard]] Json colorToJson(Color color)
{
    return Json{{"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a}};
}

[[nodiscard]] Color colorFromJson(const Json& json)
{
    return Color{static_cast<std::uint8_t>(json.at("r").get<int>()),
                 static_cast<std::uint8_t>(json.at("g").get<int>()),
                 static_cast<std::uint8_t>(json.at("b").get<int>()),
                 static_cast<std::uint8_t>(json.at("a").get<int>())};
}

[[nodiscard]] EntityId entityId(const entt::registry& registry,
                                entt::entity entity)
{
    if (entity == entt::null || !registry.valid(entity) ||
        !registry.all_of<SaveEntityId>(entity)) {
        return 0;
    }
    return registry.get<SaveEntityId>(entity).value;
}

void ensureSaveIds(entt::registry& registry)
{
    EntityId next = 1;
    const auto existing = registry.view<const SaveEntityId>();
    for (const auto entity : existing) {
        next =
            std::max(next, existing.get<const SaveEntityId>(entity).value + 1);
    }

    const auto named = registry.view<const Name>();
    for (const auto entity : named) {
        if (!registry.all_of<SaveEntityId>(entity)) {
            registry.emplace<SaveEntityId>(entity, SaveEntityId{next});
            ++next;
        }
    }
}

[[nodiscard]] Json mapToJson(const Map& map)
{
    Json tiles = Json::array();
    Json revealed = Json::array();
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            tiles.push_back(tileToString(map.tileAt(x, y)));
            revealed.push_back(map.isRevealed(x, y));
        }
    }

    return Json{{"width", map.width()},
                {"height", map.height()},
                {"tiles", std::move(tiles)},
                {"revealed", std::move(revealed)}};
}

[[nodiscard]] Map mapFromJson(const Json& json)
{
    const int width = json.at("width").get<int>();
    const int height = json.at("height").get<int>();
    if (width <= 0 || height <= 0 ||
        width > std::numeric_limits<int>::max() / height) {
        throw std::runtime_error("Invalid saved map dimensions");
    }

    const auto& tiles = json.at("tiles");
    const auto& revealed = json.at("revealed");
    const auto expected = static_cast<std::size_t>(width * height);
    if (!tiles.is_array() || tiles.size() != expected || !revealed.is_array() ||
        revealed.size() != expected) {
        throw std::runtime_error("Invalid saved map tile data");
    }

    Map map{width, height, TileType::Wall};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto index = static_cast<std::size_t>(y * width + x);
            map.setTile(
                x, y, tileFromString(tiles.at(index).get<std::string>()));
            map.setRevealed(x, y, revealed.at(index).get<bool>());
        }
    }
    return map;
}

[[nodiscard]] Json resourcesToJson(const entt::registry& registry,
                                   const GameResources& resources)
{
    Json messages = Json::array();
    for (const auto message : resources.log.messages()) {
        messages.push_back(std::string{message});
    }

    return Json{{"depth", resources.depth},
                {"player", entityId(registry, resources.player)},
                {"rng_seed", resources.rng.seed()},
                {"rng_engine", resources.rng.engineState()},
                {"log", std::move(messages)}};
}

void loadResources(const Json& json,
                   const std::unordered_map<EntityId, entt::entity>& entities,
                   GameResources& resources)
{
    GameResources loaded;
    loaded.depth = json.at("depth").get<int>();
    if (loaded.depth < 1) {
        throw std::runtime_error("Invalid saved dungeon depth");
    }

    loaded.rng.reseed(json.at("rng_seed").get<Rng::Seed>());
    loaded.rng.restoreEngineState(json.at("rng_engine").get<std::string>());
    const auto playerId = json.at("player").get<EntityId>();
    const auto player = entities.find(playerId);
    if (player == entities.end()) {
        throw std::runtime_error("Saved player entity is missing");
    }
    loaded.player = player->second;

    for (const auto& message : json.value("log", Json::array())) {
        loaded.log.add(message.get<std::string>());
    }

    resources = std::move(loaded);
}

[[nodiscard]] Json entityToJson(const entt::registry& registry,
                                entt::entity entity)
{
    Json components = Json::object();

    if (registry.all_of<Position>(entity)) {
        const auto& component = registry.get<Position>(entity);
        components["position"] = Json{{"x", component.x}, {"y", component.y}};
    }
    if (registry.all_of<Renderable>(entity)) {
        const auto& component = registry.get<Renderable>(entity);
        components["renderable"] =
            Json{{"glyph", static_cast<std::uint32_t>(component.glyph)},
                 {"foreground", colorToJson(component.foreground)},
                 {"background", colorToJson(component.background)},
                 {"order", component.order}};
    }
    if (registry.all_of<Name>(entity)) {
        components["name"] = registry.get<Name>(entity).value;
    }
    if (registry.all_of<Player>(entity)) {
        components["player"] = true;
    }
    if (registry.all_of<Monster>(entity)) {
        components["monster"] = true;
    }
    if (registry.all_of<Item>(entity)) {
        components["item"] = true;
    }
    if (registry.all_of<BlocksTile>(entity)) {
        components["blocks_tile"] = true;
    }
    if (registry.all_of<Viewshed>(entity)) {
        const auto& component = registry.get<Viewshed>(entity);
        components["viewshed"] =
            Json{{"range", component.range}, {"dirty", true}};
    }
    if (registry.all_of<CombatStats>(entity)) {
        const auto& component = registry.get<CombatStats>(entity);
        components["combat_stats"] = Json{{"max_hp", component.maxHp},
                                          {"hp", component.hp},
                                          {"defense", component.defense},
                                          {"power", component.power}};
    }
    if (registry.all_of<Consumable>(entity)) {
        components["consumable"] = true;
    }
    if (registry.all_of<ProvidesHealing>(entity)) {
        components["provides_healing"] =
            registry.get<ProvidesHealing>(entity).amount;
    }
    if (registry.all_of<Ranged>(entity)) {
        components["ranged"] = registry.get<Ranged>(entity).range;
    }
    if (registry.all_of<AreaOfEffect>(entity)) {
        components["area_of_effect"] =
            registry.get<AreaOfEffect>(entity).radius;
    }
    if (registry.all_of<InflictsDamage>(entity)) {
        components["inflicts_damage"] =
            registry.get<InflictsDamage>(entity).amount;
    }
    if (registry.all_of<Equippable>(entity)) {
        components["equippable"] =
            slotToString(registry.get<Equippable>(entity).slot);
    }
    if (registry.all_of<MeleePowerBonus>(entity)) {
        components["melee_power_bonus"] =
            registry.get<MeleePowerBonus>(entity).amount;
    }
    if (registry.all_of<DefenseBonus>(entity)) {
        components["defense_bonus"] = registry.get<DefenseBonus>(entity).amount;
    }
    if (registry.all_of<InBackpack>(entity)) {
        components["in_backpack"] =
            entityId(registry, registry.get<InBackpack>(entity).owner);
    }
    if (registry.all_of<Equipped>(entity)) {
        const auto& component = registry.get<Equipped>(entity);
        components["equipped"] =
            Json{{"owner", entityId(registry, component.owner)},
                 {"slot", slotToString(component.slot)}};
    }

    return Json{{"id", entityId(registry, entity)},
                {"components", std::move(components)}};
}

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
    if (components.contains("consumable")) {
        registry.emplace<Consumable>(entity);
    }
    if (components.contains("provides_healing")) {
        registry.emplace<ProvidesHealing>(
            entity,
            ProvidesHealing{components.at("provides_healing").get<int>()});
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

    Json root{{"version", 1},
              {"map", mapToJson(map)},
              {"resources", resourcesToJson(registry, resources)},
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
        if (root.value("version", 0) != 1) {
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
