/**
 * **************************************************************************
 * @file     ExperienceSystem.hpp
 * @brief    Declares XP rewards and player level advancement
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

namespace rl {

struct GameResources;

[[nodiscard]] int xpThresholdForLevel(int level) noexcept;

void awardExperienceForKill(entt::registry& registry,
                            GameResources& resources,
                            entt::entity defeated);

} // namespace rl
