#include "save/SaveTypes.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace rl::save {

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

[[nodiscard]] std::string logCategoryToString(LogCategory category)
{
    switch (category) {
    case LogCategory::Combat:
        return "combat";
    case LogCategory::Items:
        return "items";
    case LogCategory::Progression:
        return "progression";
    case LogCategory::System:
        return "system";
    case LogCategory::General:
        return "general";
    }
    return "general";
}

[[nodiscard]] LogCategory logCategoryFromString(const std::string& value)
{
    if (value == "combat") {
        return LogCategory::Combat;
    }
    if (value == "items") {
        return LogCategory::Items;
    }
    if (value == "progression") {
        return LogCategory::Progression;
    }
    if (value == "system") {
        return LogCategory::System;
    }
    return LogCategory::General;
}

[[nodiscard]] Json logEntryToJson(const LogEntry& entry)
{
    Json fragments = Json::array();
    for (const auto& fragment : entry.fragments) {
        fragments.push_back(
            Json{{"text", fragment.text},
                 {"foreground", colorToJson(fragment.foreground)}});
    }
    return Json{{"message", entry.message},
                {"category", logCategoryToString(entry.category)},
                {"sequence", entry.sequence},
                {"fragments", std::move(fragments)}};
}

[[nodiscard]] LogEntry logEntryFromJson(const Json& json)
{
    LogEntry entry;
    entry.message = json.value("message", std::string{});
    entry.category =
        logCategoryFromString(json.value("category", std::string{"general"}));
    entry.sequence = json.value("sequence", 0);
    for (const auto& fragment : json.value("fragments", Json::array())) {
        entry.fragments.push_back(
            LogFragment{fragment.value("text", std::string{}),
                        fragment.contains("foreground")
                            ? colorFromJson(fragment.at("foreground"))
                            : defaultLogColor(entry.category)});
    }
    return entry;
}

} // namespace rl::save
