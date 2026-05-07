/**
 * **************************************************************************
 * @file     RawDatabase.cpp
 * @brief    Loads JSON raw content into the raw database
 * **************************************************************************
 */

#include "raws/RawDatabase.hpp"

#include <charconv>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

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

void parseEffectsObject(const Json& json, RawEffects& effects)
{
    for (const auto& [key, value] : json.items()) {
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
        } else {
            throw std::runtime_error("Unknown effect key: " + key);
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

void parseStringList(const Json& json, std::vector<std::string>& out)
{
    for (const auto& value : json) {
        out.push_back(value.get<std::string>());
    }
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

    if (json.contains("combat")) {
        entity.combat = parseCombat(json.at("combat"));
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

    appendSpawnTable(root, "monster_spawn_table", database.monsterSpawnTable_);
    appendSpawnTable(root, "item_spawn_table", database.itemSpawnTable_);
    appendSpawnTable(root, "prop_spawn_table", database.propSpawnTable_);

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
    return {};
}

const RawDatabase& defaultRawDatabase()
{
    static const RawDatabase database = RawDatabase::loadDefault();
    return database;
}

} // namespace rl
