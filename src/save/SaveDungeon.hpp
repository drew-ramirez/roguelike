#pragma once

#include "map/MasterDungeonMap.hpp"
#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json storedEntityToJson(const StoredEntity& entity);
[[nodiscard]] Json dungeonToJson(const MasterDungeonMap& dungeon);

} // namespace rl::save
