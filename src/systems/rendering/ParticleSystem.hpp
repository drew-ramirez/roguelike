/**
 * **************************************************************************
 * @file     ParticleSystem.hpp
 * @brief    Declares transient particle request and lifetime helpers
 * **************************************************************************
 */

#pragma once

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;

void requestDamageParticle(GameResources& resources, Point position);
void requestHealingParticle(GameResources& resources, Point position);
void requestMappingParticle(GameResources& resources, Point position);
void requestTrapParticle(GameResources& resources, Point position);
void flushParticleRequests(entt::registry& registry, GameResources& resources);
void updateParticleLifetimes(entt::registry& registry);

} // namespace rl
