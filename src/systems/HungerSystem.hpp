/**
 * **************************************************************************
 * @file     HungerSystem.hpp
 * @brief    Declares player hunger clock turn pressure helpers
 * **************************************************************************
 */

#pragma once

#include <string_view>

#include <entt/entt.hpp>

namespace rl {

enum class HungerState;
struct GameResources;

[[nodiscard]] HungerState hungerStateForDuration(int duration) noexcept;
[[nodiscard]] std::string_view hungerStateLabel(HungerState state) noexcept;

void processHunger(entt::registry& registry, GameResources& resources);

} // namespace rl
