#include "save/SaveEntities.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rl::save {

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
    if (registry.all_of<RawIdentity>(entity)) {
        components["raw_identity"] =
            registry.get<RawIdentity>(entity).templateName;
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
    if (registry.all_of<Attributes>(entity)) {
        const auto& component = registry.get<Attributes>(entity);
        components["attributes"] =
            Json{{"might", component.might},
                 {"fitness", component.fitness},
                 {"quickness", component.quickness},
                 {"intelligence", component.intelligence}};
    }
    if (registry.all_of<Pools>(entity)) {
        const auto& component = registry.get<Pools>(entity);
        components["pools"] = Json{{"hit_points", component.hitPoints},
                                   {"max_hit_points", component.maxHitPoints},
                                   {"mana", component.mana},
                                   {"max_mana", component.maxMana},
                                   {"level", component.level},
                                   {"xp", component.xp},
                                   {"gold", component.gold}};
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
    if (registry.all_of<XpReward>(entity)) {
        components["xp_reward"] = registry.get<XpReward>(entity).amount;
    }
    if (registry.all_of<ItemStats>(entity)) {
        const auto& component = registry.get<ItemStats>(entity);
        components["item_stats"] =
            Json{{"weight", component.weight},
                 {"base_value", component.baseValue},
                 {"initiative_penalty", component.initiativePenalty},
                 {"category", component.category}};
    }
    if (registry.all_of<WeaponStats>(entity)) {
        const auto& component = registry.get<WeaponStats>(entity);
        components["weapon_stats"] = Json{{"damage", component.damage},
                                          {"hit_bonus", component.hitBonus},
                                          {"attribute", component.attribute}};
    }
    if (registry.all_of<RangedWeapon>(entity)) {
        const auto& component = registry.get<RangedWeapon>(entity);
        components["ranged_weapon"] = Json{{"range", component.range},
                                           {"damage", component.damage},
                                           {"hit_bonus", component.hitBonus},
                                           {"attribute", component.attribute}};
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
} // namespace rl::save
