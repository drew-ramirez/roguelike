#pragma once

#include "map/Map.hpp"
#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json mapToJson(const Map& map);
[[nodiscard]] Map mapFromJson(const Json& json);

} // namespace rl::save
