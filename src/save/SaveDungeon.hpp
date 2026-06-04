#pragma once

#include <map>

#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json storedEntityToJson(const StoredEntity& entity);
[[nodiscard]] StoredEntity storedEntityFromJson(const Json& json);
[[nodiscard]] Json dungeonToJson(const MasterDungeonMap& dungeon);
[[nodiscard]] std::map<int, DungeonLevelState>
dungeonFromJson(const Json& json);

} // namespace rl::save