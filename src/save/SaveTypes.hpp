#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "core/Point.hpp"
#include "ecs/Components.hpp"
#include "map/TileType.hpp"
#include "render/Color.hpp"

namespace rl::save {

using Json = nlohmann::json;
using EntityId = std::uint32_t;
inline constexpr int currentSaveVersion = 4;

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

} // namespace rl::save