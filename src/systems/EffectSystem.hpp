/**
 * **************************************************************************
 * @file     EffectSystem.hpp
 * @brief    Declares generic item effect resolution
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"

namespace rl {

struct GameResources;
class Map;

enum class EffectKind { Heal, Nutrition, Damage, RevealMap };

struct EffectRequest {
    EffectKind kind = EffectKind::Heal;
    entt::entity actor = entt::null;
    std::optional<Point> target;
    int amount = 0;
    int radius = 0;
};

class EffectsQueue {
  public:
    EffectsQueue() = default;
    explicit EffectsQueue(std::vector<EffectRequest> requests);

    void push(EffectRequest request);

    [[nodiscard]] bool resolve(entt::registry& registry,
                               GameResources& resources,
                               Map& map);

  private:
    std::vector<EffectRequest> requests_;
};

[[nodiscard]] std::vector<EffectRequest>
effectRequestsForItem(const entt::registry& registry,
                      entt::entity actor,
                      entt::entity item,
                      std::optional<Point> target = std::nullopt);

[[nodiscard]] bool applyItemEffects(entt::registry& registry,
                                    GameResources& resources,
                                    Map& map,
                                    entt::entity actor,
                                    entt::entity item,
                                    std::optional<Point> target = std::nullopt);

} // namespace rl
