/**
 * **************************************************************************
 * @file     SpellSystem.hpp
 * @brief    Declares raw-defined player spell casting
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

[[nodiscard]] std::vector<std::string_view>
knownSpellIds(const entt::registry& registry, entt::entity caster);

[[nodiscard]] bool castKnownSpell(entt::registry& registry,
                                  GameResources& resources,
                                  Map& map,
                                  entt::entity caster,
                                  std::string_view spellId,
                                  std::optional<Point> target,
                                  IFovProvider* fovProvider = nullptr);

[[nodiscard]] bool processSpellCasts(entt::registry& registry,
                                     GameResources& resources,
                                     Map& map,
                                     IFovProvider* fovProvider = nullptr);

} // namespace rl
