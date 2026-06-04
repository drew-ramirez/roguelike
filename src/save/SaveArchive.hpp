#pragma once

#include "save/SaveTypes.hpp"

namespace rl::save {

[[nodiscard]] Json archiveToJson(const entt::registry& registry,
                                 const GameResources& resources,
                                 const Map& map,
                                 Json entities);
[[nodiscard]] bool isSupportedArchiveVersion(const Json& root);

} // namespace rl::save
