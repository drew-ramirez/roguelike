/**
 * **************************************************************************
 * @file     HungerSystem.cpp
 * @brief    Applies gentle starvation pressure after real player turns
 * **************************************************************************
 */

#include "systems/HungerSystem.hpp"

#include <algorithm>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "systems/DamageSystem.hpp"

namespace rl {
namespace {

constexpr int wellFedThreshold = 800;
constexpr int hungryThreshold = 200;
constexpr int starvingThreshold = 0;
constexpr int starvationDamageInterval = 20;

void logStateTransition(GameResources& resources,
                        HungerState previous,
                        HungerState current)
{
    if (previous == current) {
        return;
    }

    if (current == HungerState::Normal && previous == HungerState::WellFed) {
        resources.log.add("You are no longer well fed.");
    } else if (current == HungerState::Hungry) {
        resources.log.add("You are getting hungry.");
    } else if (current == HungerState::Starving) {
        resources.log.add("You are starving!");
    }
}

} // namespace

HungerState hungerStateForDuration(int duration) noexcept
{
    if (duration > wellFedThreshold) {
        return HungerState::WellFed;
    }
    if (duration > hungryThreshold) {
        return HungerState::Normal;
    }
    if (duration > starvingThreshold) {
        return HungerState::Hungry;
    }
    return HungerState::Starving;
}

std::string_view hungerStateLabel(HungerState state) noexcept
{
    switch (state) {
    case HungerState::WellFed:
        return "Well fed";
    case HungerState::Normal:
        return "Normal";
    case HungerState::Hungry:
        return "Hungry";
    case HungerState::Starving:
        return "Starving";
    }
    return "Normal";
}

void consumeHunger(entt::registry& registry,
                   GameResources& resources,
                   int amount)
{
    if (amount <= 0) {
        return;
    }

    const auto player = resources.player;
    if (player == entt::null || !registry.valid(player) ||
        !registry.all_of<HungerClock>(player)) {
        return;
    }

    auto& clock = registry.get<HungerClock>(player);
    const HungerState previous = clock.state;
    clock.duration -= amount;
    clock.state = hungerStateForDuration(clock.duration);
    logStateTransition(resources, previous, clock.state);

    if (clock.state == HungerState::Starving) {
        const int damage = std::max(1, amount / starvationDamageInterval);
        resources.log.add("Starvation hurts.");
        queueDamage(registry, player, damage);
    }
}

void processHunger(entt::registry& registry, GameResources& resources)
{
    const auto player = resources.player;
    if (player == entt::null || !registry.valid(player) ||
        !registry.all_of<HungerClock>(player)) {
        return;
    }

    auto& clock = registry.get<HungerClock>(player);
    const HungerState previous = clock.state;
    --clock.duration;
    clock.state = hungerStateForDuration(clock.duration);
    logStateTransition(resources, previous, clock.state);

    if (clock.state == HungerState::Starving &&
        clock.duration % starvationDamageInterval == 0) {
        resources.log.add("Starvation hurts.");
        queueDamage(registry, player, 1);
    }
}

} // namespace rl
