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

namespace rl {
namespace {

using Json = nlohmann::json;
using EntityId = std::uint32_t;
constexpr int currentSaveVersion = 2;

[[nodiscard]] std::string tileToString(TileType tile)
{
    switch (tile) {
    case TileType::Wall:
        return "wall";
    case TileType::Floor:
        return "floor";
    case TileType::DownStairs:
        return "down_stairs";
    case TileType::UpStairs:
        return "up_stairs";
    case TileType::ClosedDoor:
        return "closed_door";
    case TileType::OpenDoor:
        return "open_door";
    }
    return "wall";
}

[[nodiscard]] TileType tileFromString(const std::string& value)
{
    if (value == "floor") {
        return TileType::Floor;
    }
    if (value == "down_stairs") {
        return TileType::DownStairs;
    }
    if (value == "up_stairs") {
        return TileType::UpStairs;
    }
    if (value == "closed_door") {
        return TileType::ClosedDoor;
    }
    if (value == "open_door") {
        return TileType::OpenDoor;
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

[[nodiscard]] std::string hungerStateToString(HungerState state)
{
    switch (state) {
    case HungerState::WellFed:
        return "well_fed";
    case HungerState::Normal:
        return "normal";
    case HungerState::Hungry:
        return "hungry";
    case HungerState::Starving:
        return "starving";
    }
    return "normal";
}

[[nodiscard]] HungerState hungerStateFromString(const std::string& value)
{
    if (value == "well_fed") {
        return HungerState::WellFed;
    }
    if (value == "hungry") {
        return HungerState::Hungry;
    }
    if (value == "starving") {
        return HungerState::Starving;
    }
    return HungerState::Normal;
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
    Json blood = Json::array();
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            tiles.push_back(tileToString(map.tileAt(x, y)));
            revealed.push_back(map.isRevealed(x, y));
            blood.push_back(map.hasBloodstain(x, y));
        }
    }

    return Json{{"width", map.width()},
                {"height", map.height()},
                {"tiles", std::move(tiles)},
                {"blood", std::move(blood)},
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
    const auto blood = json.value("blood", Json::array());
    const auto expected = static_cast<std::size_t>(width * height);
    if (!tiles.is_array() || tiles.size() != expected || !revealed.is_array() ||
        revealed.size() != expected) {
        throw std::runtime_error("Invalid saved map tile data");
    }
    if (!blood.empty() && (!blood.is_array() || blood.size() != expected)) {
        throw std::runtime_error("Invalid saved map blood data");
    }

    Map map{width, height, TileType::Wall};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto index = static_cast<std::size_t>(y * width + x);
            map.setTile(
                x, y, tileFromString(tiles.at(index).get<std::string>()));
            map.setRevealed(x, y, revealed.at(index).get<bool>());
            if (!blood.empty()) {
                map.setBloodstain(x, y, blood.at(index).get<bool>());
            }
        }
    }
    return map;
}

void putBool(Json& json, const char* key, bool value)
{
    if (value) {
        json[key] = true;
    }
}

[[nodiscard]] Json pointToJson(Point point)
{
    return Json{{"x", point.x}, {"y", point.y}};
}

[[nodiscard]] Point pointFromJson(const Json& json)
{
    return Point{json.at("x").get<int>(), json.at("y").get<int>()};
}

[[nodiscard]] Json storedEntityToJson(const StoredEntity& entity)
{
    Json json = Json::object();
    if (entity.rawIdentity.has_value()) {
        json["raw_identity"] = entity.rawIdentity->templateName;
    }
    if (entity.position.has_value()) {
        json["position"] = pointToJson(
            Point{entity.position->x, entity.position->y});
    }
    if (entity.renderable.has_value()) {
        json["renderable"] =
            Json{{"glyph", static_cast<std::uint32_t>(entity.renderable->glyph)},
                 {"foreground", colorToJson(entity.renderable->foreground)},
                 {"background", colorToJson(entity.renderable->background)},
                 {"order", entity.renderable->order}};
    }
    if (entity.name.has_value()) {
        json["name"] = entity.name->value;
    }

    putBool(json, "monster", entity.monster);
    putBool(json, "item", entity.item);
    putBool(json, "blocks_tile", entity.blocksTile);
    putBool(json, "hidden", entity.hidden);
    putBool(json, "consumable", entity.consumable);
    putBool(json, "magic_mapper", entity.magicMapper);
    putBool(json, "neutral", entity.neutral);
    putBool(json, "bystander", entity.bystander);
    putBool(json, "prop", entity.prop);
    putBool(json, "boss", entity.boss);
    putBool(json, "grants_win", entity.grantsWin);
    putBool(json, "identified_item", entity.identifiedItem);
    putBool(json, "cursed_item", entity.cursedItem);
    putBool(json, "town_portal", entity.townPortal);

    if (entity.viewshed.has_value()) {
        json["viewshed"] = Json{{"range", entity.viewshed->range}};
    }
    if (entity.combatStats.has_value()) {
        json["combat_stats"] = Json{{"max_hp", entity.combatStats->maxHp},
                                    {"hp", entity.combatStats->hp},
                                    {"defense", entity.combatStats->defense},
                                    {"power", entity.combatStats->power}};
    }
    if (entity.attributes.has_value()) {
        json["attributes"] = Json{
            {"might", entity.attributes->might},
            {"fitness", entity.attributes->fitness},
            {"quickness", entity.attributes->quickness},
            {"intelligence", entity.attributes->intelligence}};
    }
    if (entity.pools.has_value()) {
        json["pools"] = Json{{"hit_points", entity.pools->hitPoints},
                             {"max_hit_points", entity.pools->maxHitPoints},
                             {"mana", entity.pools->mana},
                             {"max_mana", entity.pools->maxMana},
                             {"level", entity.pools->level},
                             {"xp", entity.pools->xp},
                             {"gold", entity.pools->gold}};
    }
    if (entity.providesHealing.has_value()) {
        json["provides_healing"] = entity.providesHealing->amount;
    }
    if (entity.providesNutrition.has_value()) {
        json["provides_nutrition"] = entity.providesNutrition->amount;
    }
    if (entity.ranged.has_value()) {
        json["ranged"] = entity.ranged->range;
    }
    if (entity.areaOfEffect.has_value()) {
        json["area_of_effect"] = entity.areaOfEffect->radius;
    }
    if (entity.inflictsDamage.has_value()) {
        json["inflicts_damage"] = entity.inflictsDamage->amount;
    }
    if (entity.equippable.has_value()) {
        json["equippable"] = slotToString(entity.equippable->slot);
    }
    if (entity.meleePowerBonus.has_value()) {
        json["melee_power_bonus"] = entity.meleePowerBonus->amount;
    }
    if (entity.defenseBonus.has_value()) {
        json["defense_bonus"] = entity.defenseBonus->amount;
    }
    if (entity.trap.has_value()) {
        json["trap"] = Json{{"damage", entity.trap->damage},
                            {"triggered", entity.trap->triggered}};
    }
    if (entity.xpReward.has_value()) {
        json["xp_reward"] = entity.xpReward->amount;
    }
    if (entity.itemStats.has_value()) {
        json["item_stats"] =
            Json{{"weight", entity.itemStats->weight},
                 {"base_value", entity.itemStats->baseValue},
                 {"initiative_penalty", entity.itemStats->initiativePenalty},
                 {"category", entity.itemStats->category}};
    }
    if (entity.weaponStats.has_value()) {
        json["weapon_stats"] =
            Json{{"damage", entity.weaponStats->damage},
                 {"hit_bonus", entity.weaponStats->hitBonus},
                 {"attribute", entity.weaponStats->attribute}};
    }
    if (entity.rangedWeapon.has_value()) {
        json["ranged_weapon"] =
            Json{{"range", entity.rangedWeapon->range},
                 {"damage", entity.rangedWeapon->damage},
                 {"hit_bonus", entity.rangedWeapon->hitBonus},
                 {"attribute", entity.rangedWeapon->attribute}};
    }
    if (entity.magicItem.has_value()) {
        json["magic_item"] =
            Json{{"tier", entity.magicItem->tier},
                 {"unidentified_name", entity.magicItem->unidentifiedName},
                 {"identified", entity.magicItem->identified}};
    }
    if (entity.vendor.has_value()) {
        json["vendor"] = Json{{"category", entity.vendor->category},
                              {"gold", entity.vendor->gold}};
    }
    if (entity.faction.has_value()) {
        json["faction"] = entity.faction->id;
    }
    if (entity.confusion.has_value()) {
        json["confusion"] = entity.confusion->turns;
    }
    if (entity.knownSpells.has_value()) {
        json["known_spells"] = entity.knownSpells->ids;
    }
    if (entity.spellbook.has_value()) {
        json["spellbook"] = entity.spellbook->spellId;
    }
    if (entity.manaCost.has_value()) {
        json["mana_cost"] = entity.manaCost->amount;
    }
    if (entity.breathWeapon.has_value()) {
        json["breath_weapon"] =
            Json{{"range", entity.breathWeapon->range},
                 {"damage", entity.breathWeapon->damage},
                 {"cooldown", entity.breathWeapon->cooldown}};
    }
    return json;
}

[[nodiscard]] StoredEntity storedEntityFromJson(const Json& json)
{
    StoredEntity entity;
    if (json.contains("raw_identity")) {
        entity.rawIdentity =
            RawIdentity{json.at("raw_identity").get<std::string>()};
    }
    if (json.contains("position")) {
        const Point point = pointFromJson(json.at("position"));
        entity.position = Position{point.x, point.y};
    }
    if (json.contains("renderable")) {
        const auto& renderable = json.at("renderable");
        entity.renderable = Renderable{
            static_cast<char32_t>(renderable.at("glyph").get<std::uint32_t>()),
            colorFromJson(renderable.at("foreground")),
            colorFromJson(renderable.at("background")),
            renderable.at("order").get<int>()};
    }
    if (json.contains("name")) {
        entity.name = Name{json.at("name").get<std::string>()};
    }
    entity.monster = json.value("monster", false);
    entity.item = json.value("item", false);
    entity.blocksTile = json.value("blocks_tile", false);
    entity.hidden = json.value("hidden", false);
    entity.consumable = json.value("consumable", false);
    entity.magicMapper = json.value("magic_mapper", false);
    entity.neutral = json.value("neutral", false);
    entity.bystander = json.value("bystander", false);
    entity.prop = json.value("prop", false);
    entity.boss = json.value("boss", false);
    entity.grantsWin = json.value("grants_win", false);
    entity.identifiedItem = json.value("identified_item", false);
    entity.cursedItem = json.value("cursed_item", false);
    entity.townPortal = json.value("town_portal", false);

    if (json.contains("viewshed")) {
        entity.viewshed =
            Viewshed{{}, json.at("viewshed").value("range", 8), true};
    }
    if (json.contains("combat_stats")) {
        const auto& stats = json.at("combat_stats");
        entity.combatStats = CombatStats{stats.at("max_hp").get<int>(),
                                         stats.at("hp").get<int>(),
                                         stats.at("defense").get<int>(),
                                         stats.at("power").get<int>()};
    }
    if (json.contains("attributes")) {
        const auto& attributes = json.at("attributes");
        entity.attributes =
            Attributes{attributes.value("might", 10),
                       attributes.value("fitness", 10),
                       attributes.value("quickness", 10),
                       attributes.value("intelligence", 10)};
    }
    if (json.contains("pools")) {
        const auto& pools = json.at("pools");
        entity.pools = Pools{pools.value("hit_points", 1),
                             pools.value("max_hit_points", 1),
                             pools.value("mana", 0),
                             pools.value("max_mana", 0),
                             pools.value("level", 1),
                             pools.value("xp", 0),
                             pools.value("gold", 0)};
    }
    if (json.contains("provides_healing")) {
        entity.providesHealing =
            ProvidesHealing{json.at("provides_healing").get<int>()};
    }
    if (json.contains("provides_nutrition")) {
        entity.providesNutrition =
            ProvidesNutrition{json.at("provides_nutrition").get<int>()};
    }
    if (json.contains("ranged")) {
        entity.ranged = Ranged{json.at("ranged").get<int>()};
    }
    if (json.contains("area_of_effect")) {
        entity.areaOfEffect =
            AreaOfEffect{json.at("area_of_effect").get<int>()};
    }
    if (json.contains("inflicts_damage")) {
        entity.inflictsDamage =
            InflictsDamage{json.at("inflicts_damage").get<int>()};
    }
    if (json.contains("equippable")) {
        entity.equippable = Equippable{
            slotFromString(json.at("equippable").get<std::string>())};
    }
    if (json.contains("melee_power_bonus")) {
        entity.meleePowerBonus =
            MeleePowerBonus{json.at("melee_power_bonus").get<int>()};
    }
    if (json.contains("defense_bonus")) {
        entity.defenseBonus = DefenseBonus{json.at("defense_bonus").get<int>()};
    }
    if (json.contains("trap")) {
        const auto& trap = json.at("trap");
        entity.trap = Trap{trap.at("damage").get<int>(),
                           trap.at("triggered").get<bool>()};
    }
    if (json.contains("xp_reward")) {
        entity.xpReward = XpReward{json.at("xp_reward").get<int>()};
    }
    if (json.contains("item_stats")) {
        const auto& item = json.at("item_stats");
        entity.itemStats =
            ItemStats{item.value("weight", 0),
                      item.value("base_value", 0),
                      item.value("initiative_penalty", 0),
                      item.value("category", std::string{})};
    }
    if (json.contains("weapon_stats")) {
        const auto& weapon = json.at("weapon_stats");
        entity.weaponStats =
            WeaponStats{weapon.value("damage", std::string{"1d4"}),
                        weapon.value("hit_bonus", 0),
                        weapon.value("attribute", std::string{"might"})};
    }
    if (json.contains("ranged_weapon")) {
        const auto& weapon = json.at("ranged_weapon");
        entity.rangedWeapon =
            RangedWeapon{weapon.value("range", 0),
                         weapon.value("damage", std::string{"1d4"}),
                         weapon.value("hit_bonus", 0),
                         weapon.value("attribute", std::string{"quickness"})};
    }
    if (json.contains("magic_item")) {
        const auto& magic = json.at("magic_item");
        entity.magicItem =
            MagicItem{magic.value("tier", std::string{"common"}),
                      magic.value("unidentified_name", std::string{}),
                      magic.value("identified", true)};
    }
    if (json.contains("vendor")) {
        const auto& vendor = json.at("vendor");
        entity.vendor = Vendor{vendor.value("category", std::string{}),
                               vendor.value("gold", 0)};
    }
    if (json.contains("faction")) {
        entity.faction = Faction{json.at("faction").get<std::string>()};
    }
    if (json.contains("confusion")) {
        entity.confusion = Confusion{json.at("confusion").get<int>()};
    }
    if (json.contains("known_spells")) {
        entity.knownSpells =
            KnownSpells{
                json.at("known_spells").get<std::vector<std::string>>()};
    }
    if (json.contains("spellbook")) {
        entity.spellbook =
            Spellbook{json.at("spellbook").get<std::string>()};
    }
    if (json.contains("mana_cost")) {
        entity.manaCost = ManaCost{json.at("mana_cost").get<int>()};
    }
    if (json.contains("breath_weapon")) {
        const auto& breath = json.at("breath_weapon");
        entity.breathWeapon = BreathWeapon{breath.value("range", 0),
                                           breath.value("damage", 0),
                                           breath.value("cooldown", 0)};
    }
    return entity;
}

[[nodiscard]] Json dungeonToJson(const MasterDungeonMap& dungeon)
{
    Json levels = Json::array();
    for (const auto& [depth, level] : dungeon.levels()) {
        Json entities = Json::array();
        for (const auto& entity : level.entities) {
            entities.push_back(storedEntityToJson(entity));
        }
        levels.push_back(Json{{"depth", depth},
                              {"builder_id", level.builderId},
                              {"map", mapToJson(level.map)},
                              {"entities", std::move(entities)}});
    }
    return levels;
}

[[nodiscard]] std::map<int, DungeonLevelState> dungeonFromJson(const Json& json)
{
    std::map<int, DungeonLevelState> levels;
    if (!json.is_array()) {
        return levels;
    }
    for (const auto& saved : json) {
        DungeonLevelState level;
        level.depth = saved.at("depth").get<int>();
        level.builderId = saved.value("builder_id", std::string{});
        level.map = mapFromJson(saved.at("map"));
        for (const auto& entity : saved.value("entities", Json::array())) {
            level.entities.push_back(storedEntityFromJson(entity));
        }
        levels.emplace(level.depth, std::move(level));
    }
    return levels;
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
                {"map_builder", resources.mapBuilderId},
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
    loaded.mapBuilderId = json.value("map_builder", std::string{"simple"});
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
    if (registry.all_of<HungerClock>(entity)) {
        const auto& component = registry.get<HungerClock>(entity);
        components["hunger_clock"] =
            Json{{"state", hungerStateToString(component.state)},
                 {"duration", component.duration}};
    }
    if (registry.all_of<ProvidesNutrition>(entity)) {
        components["provides_nutrition"] =
            registry.get<ProvidesNutrition>(entity).amount;
    }
    if (registry.all_of<MagicMapper>(entity)) {
        components["magic_mapper"] = true;
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
    if (registry.all_of<Trap>(entity)) {
        const auto& component = registry.get<Trap>(entity);
        components["trap"] = Json{{"damage", component.damage},
                                  {"triggered", component.triggered}};
    }
    if (registry.all_of<Hidden>(entity)) {
        components["hidden"] = true;
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
