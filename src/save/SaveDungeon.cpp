#include "save/SaveDungeon.hpp"

#include <cstdint>
#include <utility>

#include "save/SaveMap.hpp"

namespace rl::save {

[[nodiscard]] Json storedEntityToJson(const StoredEntity& entity)
{
    Json json = Json::object();
    if (entity.rawIdentity.has_value()) {
        json["raw_identity"] = entity.rawIdentity->templateName;
    }
    if (entity.position.has_value()) {
        json["position"] =
            pointToJson(Point{entity.position->x, entity.position->y});
    }
    if (entity.renderable.has_value()) {
        json["renderable"] = Json{
            {"glyph", static_cast<std::uint32_t>(entity.renderable->glyph)},
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
        json["attributes"] =
            Json{{"might", entity.attributes->might},
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

} // namespace rl::save
