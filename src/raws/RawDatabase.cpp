/**
 * **************************************************************************
 * @file     RawDatabase.cpp
 * @brief    Loads and validates JSON raw content
 * **************************************************************************
 */

#include "raws/RawDatabase.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "core/Dice.hpp"

namespace rl {
namespace {

using Json = nlohmann::json;

[[nodiscard]] int hexValue(char value)
{
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return 10 + (value - 'a');
    }
    if (value >= 'A' && value <= 'F') {
        return 10 + (value - 'A');
    }
    throw std::runtime_error("Invalid color hex digit");
}

[[nodiscard]] std::uint8_t parseHexByte(std::string_view text,
                                        std::size_t offset)
{
    return static_cast<std::uint8_t>((hexValue(text[offset]) << 4) |
                                     hexValue(text[offset + 1]));
}

[[nodiscard]] Color parseColor(const Json& json)
{
    const auto value = json.get<std::string>();
    if (value == "white") {
        return Color::white();
    }
    if (value == "black") {
        return Color::black();
    }
    if (value.size() == 7 && value.front() == '#') {
        return Color{parseHexByte(value, 1),
                     parseHexByte(value, 3),
                     parseHexByte(value, 5),
                     255};
    }
    throw std::runtime_error("Color must be #RRGGBB, white, or black");
}

[[nodiscard]] int parseIntLike(const Json& json)
{
    if (json.is_number_integer()) {
        return json.get<int>();
    }
    if (json.is_string()) {
        const auto text = json.get<std::string>();
        int value = 0;
        const auto* const begin = text.data();
        const auto* const end = text.data() + text.size();
        const auto [next, error] = std::from_chars(begin, end, value);
        if (error == std::errc{} && next == end) {
            return value;
        }
    }
    throw std::runtime_error("Expected integer value");
}

[[nodiscard]] char32_t parseGlyph(const Json& json)
{
    const auto text = json.get<std::string>();
    if (text.empty()) {
        throw std::runtime_error("Renderable glyph cannot be empty");
    }
    if (text.size() != 1 || static_cast<unsigned char>(text.front()) > 0x7F) {
        throw std::runtime_error(
            "Renderable glyph must be one ASCII codepoint");
    }
    return static_cast<char32_t>(text.front());
}

[[nodiscard]] RawRenderable parseRenderable(const Json& json)
{
    RawRenderable renderable;
    renderable.glyph = parseGlyph(json.at("glyph"));
    renderable.foreground = parseColor(json.at("fg"));
    renderable.background =
        json.contains("bg") ? parseColor(json.at("bg")) : Color::black();
    renderable.order = json.value("order", renderable.order);
    return renderable;
}

[[nodiscard]] EquipmentSlot parseEquipmentSlot(const Json& json)
{
    const auto text = json.get<std::string>();
    if (text == "weapon") {
        return EquipmentSlot::Weapon;
    }
    if (text == "armor") {
        return EquipmentSlot::Armor;
    }
    throw std::runtime_error("Unknown equipment slot: " + text);
}

void parseEffectsObject(const Json& json, RawEffects& effects)
{
    static const std::vector<std::string> knownKeys{
        "provides_healing",
        "provides_nutrition",
        "magic_mapping",
        "inflicts_damage",
        "area_of_effect",
        "town_portal",
        "confusion",
        "identify",
        "remove_curse",
    };

    for (const auto& [key, value] : json.items()) {
        if (std::find(knownKeys.begin(), knownKeys.end(), key) ==
            knownKeys.end()) {
            throw std::runtime_error("Unknown effect key: " + key);
        }
        if (key == "provides_healing") {
            effects.providesHealing = parseIntLike(value);
        } else if (key == "provides_nutrition") {
            effects.providesNutrition = parseIntLike(value);
        } else if (key == "magic_mapping") {
            effects.magicMapper = value.get<bool>();
        } else if (key == "inflicts_damage") {
            effects.inflictsDamage = parseIntLike(value);
        } else if (key == "area_of_effect") {
            effects.areaOfEffect = parseIntLike(value);
        } else if (key == "town_portal") {
            effects.townPortal = value.get<bool>();
        } else if (key == "confusion") {
            effects.confusionTurns = parseIntLike(value);
        } else if (key == "identify") {
            effects.identify = value.get<bool>();
        } else if (key == "remove_curse") {
            effects.removeCurse = value.get<bool>();
        }
    }
}

[[nodiscard]] RawEffects parseEffects(const Json& json)
{
    RawEffects effects;
    if (json.contains("effects")) {
        parseEffectsObject(json.at("effects"), effects);
    }
    if (json.contains("consumable")) {
        const auto& consumable = json.at("consumable");
        if (consumable.is_object() && consumable.contains("effects")) {
            parseEffectsObject(consumable.at("effects"), effects);
        }
    }
    return effects;
}

[[nodiscard]] RawCombatStats parseCombat(const Json& json)
{
    return RawCombatStats{json.at("max_hp").get<int>(),
                          json.value("hp", json.at("max_hp").get<int>()),
                          json.value("defense", 0),
                          json.value("power", 1)};
}

[[nodiscard]] RawAttributes parseAttributes(const Json& json)
{
    return RawAttributes{json.value("might", 10),
                         json.value("fitness", 10),
                         json.value("quickness", 10),
                         json.value("intelligence", 10)};
}

[[nodiscard]] RawAttributes parseAttributeBonus(const Json& json)
{
    return RawAttributes{json.value("might", 0),
                         json.value("fitness", 0),
                         json.value("quickness", 0),
                         json.value("intelligence", 0)};
}

[[nodiscard]] RawPools parsePools(const Json& json)
{
    return RawPools{json.value("hit_points", 1),
                    json.value("max_hit_points", json.value("hit_points", 1)),
                    json.value("mana", 0),
                    json.value("max_mana", json.value("mana", 0)),
                    json.value("level", 1),
                    json.value("xp", 0),
                    json.value("gold", 0)};
}

[[nodiscard]] RawItemStats parseItemStats(const Json& json)
{
    return RawItemStats{json.value("weight", 0),
                        json.value("base_value", 0),
                        json.value("initiative_penalty", 0),
                        json.value("category", std::string{})};
}

[[nodiscard]] RawWeaponStats parseWeapon(const Json& json)
{
    return RawWeaponStats{json.value("damage", std::string{"1d4"}),
                          json.value("hit_bonus", 0),
                          json.value("attribute", std::string{"might"})};
}

[[nodiscard]] RawRangedWeaponStats parseRangedWeapon(const Json& json)
{
    return RawRangedWeaponStats{
        json.value("range", 0),
        json.value("damage", std::string{"1d4"}),
        json.value("hit_bonus", 0),
        json.value("attribute", std::string{"quickness"})};
}

[[nodiscard]] RawMagicItem parseMagicItem(const Json& json)
{
    return RawMagicItem{json.value("tier", std::string{"common"}),
                        json.value("unidentified_name", std::string{}),
                        json.value("identified", true)};
}

[[nodiscard]] RawVendor parseVendor(const Json& json)
{
    return RawVendor{json.value("category", std::string{}),
                     json.value("gold", 0)};
}

void parseStringList(const Json& json, std::vector<std::string>& out)
{
    for (const auto& value : json) {
        out.push_back(value.get<std::string>());
    }
}

[[nodiscard]] RawCampSite parseCampSite(const Json& json)
{
    return RawCampSite{json.value("type", std::string{"camp"}),
                       json.value("safety_bonus", 0)};
}

[[nodiscard]] RawCookingStation parseCookingStation(const Json& json)
{
    RawCookingStation station;
    station.stationId = json.value("id", std::string{"campfire"});
    if (json.contains("methods")) {
        parseStringList(json.at("methods"), station.methods);
    }
    if (json.contains("place_tags")) {
        parseStringList(json.at("place_tags"), station.placeTags);
    }
    return station;
}

[[nodiscard]] RawSettlementService parseSettlementService(const Json& json)
{
    return RawSettlementService{
        json.value("service", std::string{"teach_recipe"}),
        json.value("recipe", std::string{})};
}

[[nodiscard]] RawEntityTemplate parseEntity(const Json& json,
                                            RawEntityKind kind)
{
    RawEntityTemplate entity;
    entity.kind = kind;
    entity.name = json.at("name").get<std::string>();
    entity.id = json.value("id", entity.name);
    entity.renderable = parseRenderable(json.at("renderable"));
    entity.blocksTile = json.value("blocks_tile", kind == RawEntityKind::Mob);
    entity.neutral = json.value("neutral", false);
    entity.bystander = json.value("bystander", false);
    entity.hidden = json.value("hidden", false);
    entity.cursed = json.value("cursed", false);
    entity.boss = json.value("boss", false);
    entity.grantsWin = json.value("grants_win", false);
    entity.faction = json.value("faction", std::string{});
    entity.viewshedRange =
        json.contains("vision_range")
            ? std::optional<int>{json.at("vision_range").get<int>()}
            : std::nullopt;
    entity.xpReward = json.contains("xp_reward")
                          ? std::optional<int>{json.at("xp_reward").get<int>()}
                          : std::nullopt;
    entity.effects = parseEffects(json);

    if (json.contains("vendor")) {
        entity.vendor = parseVendor(json.at("vendor"));
    }
    if (json.contains("camp_site")) {
        entity.campSite = parseCampSite(json.at("camp_site"));
    }
    if (json.contains("cooking_station")) {
        entity.cookingStation =
            parseCookingStation(json.at("cooking_station"));
    }
    if (json.contains("settlement_service")) {
        entity.settlementService =
            parseSettlementService(json.at("settlement_service"));
    }
    if (json.contains("combat")) {
        entity.combat = parseCombat(json.at("combat"));
    }
    if (json.contains("attributes")) {
        entity.attributes = parseAttributes(json.at("attributes"));
    }
    if (json.contains("pools")) {
        entity.pools = parsePools(json.at("pools"));
    }
    if (json.contains("equippable")) {
        entity.equippable = parseEquipmentSlot(json.at("equippable"));
    }
    if (json.contains("melee_power_bonus")) {
        entity.meleePowerBonus = json.at("melee_power_bonus").get<int>();
    }
    if (json.contains("defense_bonus")) {
        entity.defenseBonus = json.at("defense_bonus").get<int>();
    }
    if (json.contains("attribute_bonus")) {
        entity.attributeBonus = parseAttributeBonus(json.at("attribute_bonus"));
    }
    if (json.contains("item_stats")) {
        entity.itemStats = parseItemStats(json.at("item_stats"));
    }
    if (json.contains("weapon")) {
        entity.weapon = parseWeapon(json.at("weapon"));
    }
    if (json.contains("ranged_weapon")) {
        entity.rangedWeapon = parseRangedWeapon(json.at("ranged_weapon"));
    }
    if (json.contains("magic")) {
        entity.magic = parseMagicItem(json.at("magic"));
    }
    if (json.contains("equipped")) {
        parseStringList(json.at("equipped"), entity.equipped);
    }
    if (json.contains("known_spells")) {
        parseStringList(json.at("known_spells"), entity.knownSpells);
    }
    if (json.contains("spellbook")) {
        entity.spellbook = json.at("spellbook").get<std::string>();
    }
    if (json.contains("mana_cost")) {
        entity.manaCost = json.at("mana_cost").get<int>();
    }
    if (json.contains("breath_weapon")) {
        const auto& breath = json.at("breath_weapon");
        entity.breathDamage = breath.value("damage", 0);
        entity.breathRange = breath.value("range", 0);
    }

    return entity;
}

void insertEntity(std::unordered_map<std::string, RawEntityTemplate>& entities,
                  std::unordered_map<std::string, std::string>& aliases,
                  RawEntityTemplate entity)
{
    const auto id = entity.id;
    const auto name = entity.name;
    if (!entities.emplace(id, std::move(entity)).second) {
        throw std::runtime_error("Duplicate raw entity: " + id);
    }
    if (name != id) {
        if (!aliases.emplace(name, id).second) {
            throw std::runtime_error("Duplicate raw entity alias: " + name);
        }
    }
}

[[nodiscard]] SpawnEntry parseSpawnEntry(const Json& json)
{
    return SpawnEntry{
        json.value("name", json.value("id", std::string{})),
        json.value("weight", 1),
        json.value("min_depth", 1),
        json.value("max_depth", 1000),
        json.value("add_map_depth_to_weight", 0),
    };
}

void appendSpawnTable(const Json& root,
                      std::string_view key,
                      std::vector<SpawnEntry>& entries)
{
    if (!root.contains(key)) {
        return;
    }
    for (const auto& entry : root.at(key)) {
        entries.push_back(parseSpawnEntry(entry));
    }
}

[[nodiscard]] RawSpell parseSpell(const Json& json)
{
    RawSpell spell;
    spell.id = json.at("id").get<std::string>();
    spell.name = json.value("name", spell.id);
    spell.manaCost = json.value("mana_cost", 0);
    spell.effects = parseEffects(json);
    if (json.contains("range")) {
        spell.range = json.at("range").get<int>();
    }
    return spell;
}

[[nodiscard]] RawFaction parseFaction(const Json& json)
{
    RawFaction faction;
    faction.id = json.at("id").get<std::string>();
    if (json.contains("hostile_to")) {
        parseStringList(json.at("hostile_to"), faction.hostileTo);
    }
    return faction;
}

[[nodiscard]] RawIngredient parseIngredient(const Json& json)
{
    RawIngredient ingredient;
    ingredient.id = json.at("id").get<std::string>();
    ingredient.name = json.value("name", ingredient.id);
    if (json.contains("item")) {
        ingredient.item = json.at("item").get<std::string>();
    }
    if (json.contains("tags")) {
        parseStringList(json.at("tags"), ingredient.tags);
    }
    return ingredient;
}

[[nodiscard]] RawIngredientAmount parseIngredientAmount(const Json& json)
{
    return RawIngredientAmount{json.at("id").get<std::string>(),
                               json.value("count", 1)};
}

[[nodiscard]] RawRecipeOutput parseRecipeOutput(const Json& json)
{
    return RawRecipeOutput{json.at("id").get<std::string>(),
                           json.value("count", 1)};
}

[[nodiscard]] RawRecipe parseRecipe(const Json& json)
{
    RawRecipe recipe;
    recipe.id = json.at("id").get<std::string>();
    recipe.name = json.value("name", recipe.id);
    recipe.station = json.value("station", std::string{});
    recipe.method = json.value("method", std::string{});
    recipe.nutrition = json.value("nutrition", recipe.nutrition);
    if (json.contains("ingredients")) {
        for (const auto& ingredient : json.at("ingredients")) {
            recipe.ingredients.push_back(parseIngredientAmount(ingredient));
        }
    }
    if (json.contains("outputs")) {
        for (const auto& output : json.at("outputs")) {
            recipe.outputs.push_back(parseRecipeOutput(output));
        }
    }
    if (json.contains("place_tags")) {
        parseStringList(json.at("place_tags"), recipe.placeTags);
    }
    return recipe;
}

[[nodiscard]] RawBiomeResourceEntry parseBiomeResourceEntry(const Json& json)
{
    return RawBiomeResourceEntry{json.at("ingredient").get<std::string>(),
                                 json.value("weight", 1),
                                 json.value("min", 1),
                                 json.value("max", 1)};
}

[[nodiscard]] RawBiomeResourceTable parseBiomeResourceTable(const Json& json)
{
    RawBiomeResourceTable table;
    table.id = json.at("id").get<std::string>();
    if (json.contains("resources")) {
        for (const auto& resource : json.at("resources")) {
            table.resources.push_back(parseBiomeResourceEntry(resource));
        }
    }
    return table;
}

} // namespace

RawDatabase RawDatabase::fromJson(std::string_view text)
{
    const Json root = Json::parse(text);
    RawDatabase database;

    if (root.contains("mobs")) {
        for (const auto& entity : root.at("mobs")) {
            insertEntity(database.entities_,
                         database.entityAliases_,
                         parseEntity(entity, RawEntityKind::Mob));
        }
    }
    if (root.contains("items")) {
        for (const auto& entity : root.at("items")) {
            insertEntity(database.entities_,
                         database.entityAliases_,
                         parseEntity(entity, RawEntityKind::Item));
        }
    }
    if (root.contains("props")) {
        for (const auto& entity : root.at("props")) {
            insertEntity(database.entities_,
                         database.entityAliases_,
                         parseEntity(entity, RawEntityKind::Prop));
        }
    }
    if (root.contains("spells")) {
        for (const auto& json : root.at("spells")) {
            auto spell = parseSpell(json);
            const auto id = spell.id;
            if (!database.spells_.emplace(id, std::move(spell)).second) {
                throw std::runtime_error("Duplicate raw spell: " + id);
            }
        }
    }
    if (root.contains("factions")) {
        for (const auto& json : root.at("factions")) {
            auto faction = parseFaction(json);
            const auto id = faction.id;
            if (!database.factions_.emplace(id, std::move(faction)).second) {
                throw std::runtime_error("Duplicate raw faction: " + id);
            }
        }
    }
    if (root.contains("ingredients")) {
        for (const auto& json : root.at("ingredients")) {
            auto ingredient = parseIngredient(json);
            const auto id = ingredient.id;
            if (!database.ingredients_.emplace(id, std::move(ingredient))
                     .second) {
                throw std::runtime_error("Duplicate raw ingredient: " + id);
            }
        }
    }
    if (root.contains("recipes")) {
        for (const auto& json : root.at("recipes")) {
            auto recipe = parseRecipe(json);
            const auto id = recipe.id;
            if (!database.recipes_.emplace(id, std::move(recipe)).second) {
                throw std::runtime_error("Duplicate raw recipe: " + id);
            }
        }
    }
    if (root.contains("biome_resource_tables")) {
        for (const auto& json : root.at("biome_resource_tables")) {
            auto table = parseBiomeResourceTable(json);
            const auto id = table.id;
            if (!database.biomeResourceTables_.emplace(id, std::move(table))
                     .second) {
                throw std::runtime_error("Duplicate biome resource table: " +
                                         id);
            }
        }
    }

    appendSpawnTable(root, "monster_spawn_table", database.monsterSpawnTable_);
    appendSpawnTable(root, "item_spawn_table", database.itemSpawnTable_);
    appendSpawnTable(root, "prop_spawn_table", database.propSpawnTable_);

    if (root.contains("spawn_table")) {
        for (const auto& json : root.at("spawn_table")) {
            const auto entry = parseSpawnEntry(json);
            const auto* raw = database.entity(entry.id);
            if (raw == nullptr) {
                database.itemSpawnTable_.push_back(entry);
            } else if (raw->kind == RawEntityKind::Mob) {
                database.monsterSpawnTable_.push_back(entry);
            } else if (raw->kind == RawEntityKind::Prop) {
                database.propSpawnTable_.push_back(entry);
            } else {
                database.itemSpawnTable_.push_back(entry);
            }
        }
    }

    if (const auto errors = database.validate(); !errors.empty()) {
        throw std::runtime_error(errors.front());
    }

    return database;
}

RawDatabase RawDatabase::loadFromFile(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error("Unable to open raw file: " + path.string());
    }
    const std::string text{std::istreambuf_iterator<char>{input},
                           std::istreambuf_iterator<char>{}};
    return fromJson(text);
}

RawDatabase RawDatabase::loadDefault()
{
    const std::filesystem::path dataPath{"data/raws/entities.json"};
    if (std::filesystem::exists(dataPath)) {
        return loadFromFile(dataPath);
    }
    return loadFromFile(std::filesystem::path{"assets/raws/entities.json"});
}

const RawEntityTemplate*
RawDatabase::entity(std::string_view name) const noexcept
{
    const std::string key{name};
    const auto found = entities_.find(key);
    if (found != entities_.end()) {
        return &found->second;
    }
    const auto alias = entityAliases_.find(key);
    if (alias == entityAliases_.end()) {
        return nullptr;
    }
    const auto aliased = entities_.find(alias->second);
    return aliased == entities_.end() ? nullptr : &aliased->second;
}

const RawSpell* RawDatabase::spell(std::string_view id) const noexcept
{
    const auto found = spells_.find(std::string{id});
    return found == spells_.end() ? nullptr : &found->second;
}

const RawFaction* RawDatabase::faction(std::string_view id) const noexcept
{
    const auto found = factions_.find(std::string{id});
    return found == factions_.end() ? nullptr : &found->second;
}

const RawIngredient*
RawDatabase::ingredient(std::string_view id) const noexcept
{
    const auto found = ingredients_.find(std::string{id});
    return found == ingredients_.end() ? nullptr : &found->second;
}

const RawRecipe* RawDatabase::recipe(std::string_view id) const noexcept
{
    const auto found = recipes_.find(std::string{id});
    return found == recipes_.end() ? nullptr : &found->second;
}

const RawBiomeResourceTable*
RawDatabase::biomeResourceTable(std::string_view id) const noexcept
{
    const auto found = biomeResourceTables_.find(std::string{id});
    return found == biomeResourceTables_.end() ? nullptr : &found->second;
}

std::vector<std::string>
RawDatabase::itemIdsForCategory(std::string_view category) const
{
    std::vector<std::string> ids;
    for (const auto& [id, entityTemplate] : entities_) {
        if (entityTemplate.kind != RawEntityKind::Item ||
            !entityTemplate.itemStats.has_value()) {
            continue;
        }

        const auto& itemCategory = entityTemplate.itemStats->category;
        const bool alchemyMatch =
            category == "alchemy" &&
            (itemCategory == "potions" || itemCategory == "scrolls" ||
             itemCategory == "food" || itemCategory == "books");
        if (category.empty() || category == "all" || itemCategory == category ||
            alchemyMatch) {
            ids.push_back(id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

const std::vector<SpawnEntry>& RawDatabase::monsterSpawnTable() const
{
    return monsterSpawnTable_;
}

const std::vector<SpawnEntry>& RawDatabase::itemSpawnTable() const
{
    return itemSpawnTable_;
}

const std::vector<SpawnEntry>& RawDatabase::propSpawnTable() const
{
    return propSpawnTable_;
}

std::vector<std::string> RawDatabase::validate() const
{
    std::vector<std::string> errors;
    const auto validateSpawnTable = [&](const std::vector<SpawnEntry>& table,
                                        std::string_view label) {
        for (const auto& entry : table) {
            if (entry.id.empty() || entity(entry.id) == nullptr) {
                errors.push_back(std::string{label} +
                                 " references unknown entity: " + entry.id);
            }
            if (entry.weight <= 0) {
                errors.push_back(std::string{label} +
                                 " entry has non-positive weight: " + entry.id);
            }
        }
    };

    validateSpawnTable(monsterSpawnTable_, "monster spawn table");
    validateSpawnTable(itemSpawnTable_, "item spawn table");
    validateSpawnTable(propSpawnTable_, "prop spawn table");

    const auto validateIngredientAmounts =
        [&](const std::vector<RawIngredientAmount>& amounts,
            std::string_view label) {
            for (const auto& amount : amounts) {
                if (ingredient(amount.id) == nullptr) {
                    errors.push_back(std::string{label} +
                                     " references unknown ingredient: " +
                                     amount.id);
                }
                if (amount.count <= 0) {
                    errors.push_back(std::string{label} +
                                     " has non-positive ingredient count: " +
                                     amount.id);
                }
            }
        };

    for (const auto& [name, entityTemplate] : entities_) {
        if (entityTemplate.vendor.has_value()) {
            const auto stock =
                itemIdsForCategory(entityTemplate.vendor->category);
            if (stock.empty()) {
                errors.push_back(name + " vendor category has no stock: " +
                                 entityTemplate.vendor->category);
            }
        }
        if (entityTemplate.settlementService.has_value()) {
            const auto& service = *entityTemplate.settlementService;
            if (!service.recipeId.empty() &&
                recipe(service.recipeId) == nullptr) {
                errors.push_back(name + " service references unknown recipe: " +
                                 service.recipeId);
            }
        }
        for (const auto& item : entityTemplate.equipped) {
            if (entity(item) == nullptr) {
                errors.push_back(name + " equips unknown item: " + item);
            }
        }
        for (const auto& spellId : entityTemplate.knownSpells) {
            if (spell(spellId) == nullptr) {
                errors.push_back(name + " knows unknown spell: " + spellId);
            }
        }
        if (!entityTemplate.faction.empty() &&
            faction(entityTemplate.faction) == nullptr) {
            errors.push_back(name + " references unknown faction: " +
                             entityTemplate.faction);
        }
        if (entityTemplate.spellbook.has_value() &&
            spell(*entityTemplate.spellbook) == nullptr) {
            errors.push_back(
                name + " teaches unknown spell: " + *entityTemplate.spellbook);
        }
        if (entityTemplate.weapon.has_value() &&
            !tryParseDice(entityTemplate.weapon->damage).has_value()) {
            errors.push_back(name + " has invalid weapon dice: " +
                             entityTemplate.weapon->damage);
        }
        if (entityTemplate.rangedWeapon.has_value()) {
            if (entityTemplate.rangedWeapon->range <= 0) {
                errors.push_back(name + " has non-positive ranged range");
            }
            if (!tryParseDice(entityTemplate.rangedWeapon->damage)
                     .has_value()) {
                errors.push_back(name + " has invalid ranged dice: " +
                                 entityTemplate.rangedWeapon->damage);
            }
        }
        if (entityTemplate.breathDamage.has_value() ||
            entityTemplate.breathRange.has_value()) {
            if (entityTemplate.breathDamage.value_or(0) <= 0 ||
                entityTemplate.breathRange.value_or(0) <= 0) {
                errors.push_back(name + " has invalid breath weapon fields");
            }
        }
    }

    for (const auto& [id, factionTemplate] : factions_) {
        for (const auto& target : factionTemplate.hostileTo) {
            if (target != "player" && faction(target) == nullptr) {
                errors.push_back(id +
                                 " is hostile to unknown faction: " + target);
            }
        }
    }
    for (const auto& [id, spellTemplate] : spells_) {
        if (spellTemplate.range.has_value() && *spellTemplate.range <= 0) {
            errors.push_back(id + " spell has non-positive range");
        }
        if (spellTemplate.effects.inflictsDamage.has_value() &&
            *spellTemplate.effects.inflictsDamage <= 0) {
            errors.push_back(id + " spell has non-positive damage");
        }
    }
    for (const auto& [id, ingredientTemplate] : ingredients_) {
        if (ingredientTemplate.item.has_value() &&
            entity(*ingredientTemplate.item) == nullptr) {
            errors.push_back(id + " ingredient references unknown item: " +
                             *ingredientTemplate.item);
        }
    }
    for (const auto& [id, recipeTemplate] : recipes_) {
        if (recipeTemplate.ingredients.empty()) {
            errors.push_back(id + " recipe has no ingredients");
        }
        if (recipeTemplate.outputs.empty()) {
            errors.push_back(id + " recipe has no outputs");
        }
        validateIngredientAmounts(recipeTemplate.ingredients, id + " recipe");
        for (const auto& output : recipeTemplate.outputs) {
            if (entity(output.id) == nullptr) {
                errors.push_back(id + " recipe outputs unknown entity: " +
                                 output.id);
            }
            if (output.count <= 0) {
                errors.push_back(id +
                                 " recipe has non-positive output count: " +
                                 output.id);
            }
        }
    }
    for (const auto& [id, table] : biomeResourceTables_) {
        if (table.resources.empty()) {
            errors.push_back(id + " biome resource table has no resources");
        }
        for (const auto& resource : table.resources) {
            if (ingredient(resource.ingredient) == nullptr) {
                errors.push_back(id +
                                 " biome resource references unknown "
                                 "ingredient: " +
                                 resource.ingredient);
            }
            if (resource.weight <= 0) {
                errors.push_back(id +
                                 " biome resource has non-positive weight: " +
                                 resource.ingredient);
            }
            if (resource.minCount <= 0 ||
                resource.maxCount < resource.minCount) {
                errors.push_back(id + " biome resource has invalid count: " +
                                 resource.ingredient);
            }
        }
    }

    return errors;
}

const RawDatabase& defaultRawDatabase()
{
    static const RawDatabase database = RawDatabase::loadDefault();
    return database;
}

} // namespace rl
