#pragma once

#include "save/SaveTypes.hpp"

namespace rl::save {

void ensureSaveIds(entt::registry& registry);
[[nodiscard]] Json entityToJson(const entt::registry& registry,
                                entt::entity entity);

} // namespace rl::save
