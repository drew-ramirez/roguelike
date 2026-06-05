#include "save/SaveDungeon.hpp"

#include <cstdint>
#include <string>
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
    putBool(json, "provides_identify", entity.providesIdentify);
    putBool(json, "provides_remove_curse", entity.providesRemoveCurse);
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
    if (entity.ingredient.has_value()) {
        json["ingredient"] = ingredientToJson(*entity.ingredient);
    }
    if (entity.meal.has_value()) {
        json["meal"] = mealToJson(*entity.meal);
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
    if (entity.attributeBonus.has_value()) {
        json["attribute_bonus"] =
            Json{{"might", entity.attributeBonus->might},
                 {"fitness", entity.attributeBonus->fitness},
                 {"quickness", entity.attributeBonus->quickness},
                 {"intelligence", entity.attributeBonus->intelligence}};
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
    if (entity.campSite.has_value()) {
        json["camp_site"] = campSiteToJson(*entity.campSite);
    }
    if (entity.cookingStation.has_value()) {
        json["cooking_station"] = cookingStationToJson(*entity.cookingStation);
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
    entity.providesIdentify = json.value("provides_identify", false);
    entity.providesRemoveCurse = json.value("provides_remove_curse", false);
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
        entity.attributes = Attributes{attributes.value("might", 10),
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
    if (json.contains("ingredient")) {
        entity.ingredient = ingredientFromJson(json.at("ingredient"));
    }
    if (json.contains("meal")) {
        entity.meal = mealFromJson(json.at("meal"));
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
    if (json.contains("attribute_bonus")) {
        const auto& bonus = json.at("attribute_bonus");
        entity.attributeBonus = AttributeBonus{bonus.value("might", 0),
                                               bonus.value("fitness", 0),
                                               bonus.value("quickness", 0),
                                               bonus.value("intelligence", 0)};
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
        entity.itemStats = ItemStats{item.value("weight", 0),
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
    if (json.contains("camp_site")) {
        entity.campSite = campSiteFromJson(json.at("camp_site"));
    }
    if (json.contains("cooking_station")) {
        entity.cookingStation =
            cookingStationFromJson(json.at("cooking_station"));
    }
    if (json.contains("faction")) {
        entity.faction = Faction{json.at("faction").get<std::string>()};
    }
    if (json.contains("confusion")) {
        entity.confusion = Confusion{json.at("confusion").get<int>()};
    }
    if (json.contains("known_spells")) {
        entity.knownSpells = KnownSpells{
            json.at("known_spells").get<std::vector<std::string>>()};
    }
    if (json.contains("spellbook")) {
        entity.spellbook = Spellbook{json.at("spellbook").get<std::string>()};
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

} // namespace rl::save
