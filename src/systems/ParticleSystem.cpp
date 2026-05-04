/**
 * **************************************************************************
 * @file     ParticleSystem.cpp
 * @brief    Creates and expires non-durable visual particles
 * **************************************************************************
 */

#include "systems/ParticleSystem.hpp"

#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"

namespace rl {
namespace {

void requestParticle(GameResources& resources,
                     Point position,
                     char32_t glyph,
                     Color foreground,
                     int lifetimeFrames)
{
    resources.particleRequests.push_back(ParticleRequest{
        position, glyph, foreground, Color::black(), lifetimeFrames});
}

} // namespace

void requestDamageParticle(GameResources& resources, Point position)
{
    requestParticle(resources, position, U'*', Color{255, 80, 80, 255}, 6);
}

void requestHealingParticle(GameResources& resources, Point position)
{
    requestParticle(resources, position, U'+', Color{80, 255, 120, 255}, 8);
}

void requestMappingParticle(GameResources& resources, Point position)
{
    requestParticle(resources, position, U'?', Color{100, 180, 255, 255}, 10);
}

void requestTrapParticle(GameResources& resources, Point position)
{
    requestParticle(resources, position, U'^', Color{255, 210, 80, 255}, 8);
}

void flushParticleRequests(entt::registry& registry, GameResources& resources)
{
    for (const auto& request : resources.particleRequests) {
        const auto particle = registry.create();
        registry.emplace<Position>(
            particle, Position{request.position.x, request.position.y});
        registry.emplace<Renderable>(
            particle,
            Renderable{
                request.glyph, request.foreground, request.background, 20});
        registry.emplace<Particle>(particle);
        registry.emplace<ParticleLifetime>(
            particle, ParticleLifetime{request.lifetimeFrames});
    }
    resources.particleRequests.clear();
}

void updateParticleLifetimes(entt::registry& registry)
{
    std::vector<entt::entity> expired;
    const auto view = registry.view<ParticleLifetime>();
    for (const auto entity : view) {
        auto& lifetime = view.get<ParticleLifetime>(entity);
        --lifetime.remainingFrames;
        if (lifetime.remainingFrames <= 0) {
            expired.push_back(entity);
        }
    }

    for (const auto entity : expired) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
}

} // namespace rl
