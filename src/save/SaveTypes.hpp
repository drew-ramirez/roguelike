#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include "core/GameResources.hpp"
#include "core/Point.hpp"
#include "ecs/Components.hpp"
#include "map/Map.hpp"
#include "map/MasterDungeonMap.hpp"
#include "map/TileType.hpp"

namespace rl::save {

using Json = nlohmann::json;
using EntityId = std::uint32_t;
inline constexpr int currentSaveVersion = 9;

[[nodiscard]] std::string tileToString(TileType tile);
[[nodiscard]] TileType tileFromString(const std::string& value);
[[nodiscard]] std::string slotToString(EquipmentSlot slot);
[[nodiscard]] EquipmentSlot slotFromString(const std::string& value);
[[nodiscard]] std::string hungerStateToString(HungerState state);
[[nodiscard]] HungerState hungerStateFromString(const std::string& value);
[[nodiscard]] Json colorToJson(Color color);
[[nodiscard]] Color colorFromJson(const Json& json);
void putBool(Json& json, const char* key, bool value);
[[nodiscard]] Json pointToJson(Point point);
[[nodiscard]] Point pointFromJson(const Json& json);
[[nodiscard]] std::string logCategoryToString(LogCategory category);
[[nodiscard]] LogCategory logCategoryFromString(const std::string& value);
[[nodiscard]] Json logEntryToJson(const LogEntry& entry);
[[nodiscard]] LogEntry logEntryFromJson(const Json& json);
[[nodiscard]] Json ingredientToJson(const Ingredient& ingredient);
[[nodiscard]] Ingredient ingredientFromJson(const Json& json);
[[nodiscard]] Json mealToJson(const Meal& meal);
[[nodiscard]] Meal mealFromJson(const Json& json);
[[nodiscard]] Json campSiteToJson(const CampSite& camp);
[[nodiscard]] CampSite campSiteFromJson(const Json& json);
[[nodiscard]] Json settlementServiceToJson(const SettlementService& service);
[[nodiscard]] SettlementService settlementServiceFromJson(const Json& json);
[[nodiscard]] Json shrineToJson(const Shrine& shrine);
[[nodiscard]] Shrine shrineFromJson(const Json& json);
[[nodiscard]] Json cookingStationToJson(const CookingStation& station);
[[nodiscard]] CookingStation cookingStationFromJson(const Json& json);

[[nodiscard]] EntityId entityId(const entt::registry& registry,
                                entt::entity entity);
void ensureSaveIds(entt::registry& registry);

} // namespace rl::save
