#include "save/SaveEntities.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

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

} // namespace rl::save
