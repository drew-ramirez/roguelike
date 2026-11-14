/**
 * **************************************************************************
 * @file     EffectSystem.hpp
 * @brief    Declares generic item and spell effect resolution
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/Point.hpp"
#include "raws/RawTypes.hpp"

namespace rl {

struct GameResources;
class IFovProvider;
class Map;

enum class EffectKind {
    Heal,
    Nutrition,
    Damage,
    RevealMap,
    Identify,
    RemoveCurse,
    TownPortal,
    Confusion
};

struct EffectRequest {
    EffectKind kind = EffectKind::Heal;
    entt::entity actor = entt::null;
    entt::entity source = entt::null;
    std::optional<Point> target;
    int amount = 0;
    int radius = 0;
    entt::entity targetEntity = entt::null;
};

class EffectsQueue {
  public:
    [[nodiscard]] static EffectsQueue
    forItem(const entt::registry& registry,
            entt::entity actor,
            entt::entity item,
            std::optional<Point> target = std::nullopt,
            entt::entity targetEntity = entt::null);
    [[nodiscard]] static EffectsQueue
    forRawEffects(const RawEffects& effects,
                  entt::entity actor,
                  entt::entity source,
                  std::optional<Point> target = std::nullopt);

    void enqueue(EffectRequest request);
    [[nodiscard]] bool resolve(entt::registry& registry,
                               GameResources& resources,
                               Map& map,
                               IFovProvider* fovProvider = nullptr);

  private:
    std::vector<EffectRequest> requests_;
};

[[nodiscard]] bool applyItemEffects(entt::registry& registry,
                                    GameResources& resources,
                                    Map& map,
                                    entt::entity actor,
                                    entt::entity item,
                                    std::optional<Point> target = std::nullopt,
                                    IFovProvider* fovProvider = nullptr);

} // namespace rl
