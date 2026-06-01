/**
 * **************************************************************************
 * @file     EffectSystem.hpp
 * @brief    Declares generic item effect resolution
 * **************************************************************************
 */

#pragma once

#include <cstddef>
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
    std::optional<Point> target;
    int amount = 0;
    int radius = 0;
    entt::entity source = entt::null;
    entt::entity targetEntity = entt::null;
};

class EffectsQueue {
  public:
    EffectsQueue() = default;
    explicit EffectsQueue(std::vector<EffectRequest> requests);

    void push(EffectRequest request);
    void append(std::vector<EffectRequest> requests);
    void enqueue(EffectRequest request);
    void enqueue(std::vector<EffectRequest> requests);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const std::vector<EffectRequest>& requests() const noexcept;
    [[nodiscard]] bool resolve(entt::registry& registry,
                               GameResources& resources,
                               Map& map,
                               IFovProvider* fovProvider = nullptr);

  private:
    std::vector<EffectRequest> requests_;
};

[[nodiscard]] std::vector<EffectRequest>
effectRequestsForItem(const entt::registry& registry,
                      entt::entity actor,
                      entt::entity item,
                      std::optional<Point> target = std::nullopt);

[[nodiscard]] std::vector<EffectRequest>
effectRequestsForRawEffects(const RawEffects& effects,
                            entt::entity actor,
                            entt::entity source,
                            std::optional<Point> target = std::nullopt);

[[nodiscard]] EffectsQueue
effectsQueueForItem(const entt::registry& registry,
                    entt::entity actor,
                    entt::entity item,
                    std::optional<Point> target = std::nullopt);

[[nodiscard]] EffectsQueue
effectsQueueForRawEffects(const RawEffects& effects,
                          entt::entity actor,
                          entt::entity source,
                          std::optional<Point> target = std::nullopt);

[[nodiscard]] bool
resolveEffectRequests(entt::registry& registry,
                      GameResources& resources,
                      Map& map,
                      const std::vector<EffectRequest>& requests,
                      IFovProvider* fovProvider = nullptr);

[[nodiscard]] bool resolveEffectsQueue(entt::registry& registry,
                                       GameResources& resources,
                                       Map& map,
                                       const EffectsQueue& queue,
                                       IFovProvider* fovProvider = nullptr);

[[nodiscard]] bool applyItemEffects(entt::registry& registry,
                                    GameResources& resources,
                                    Map& map,
                                    entt::entity actor,
                                    entt::entity item,
                                    std::optional<Point> target = std::nullopt,
                                    IFovProvider* fovProvider = nullptr);

} // namespace rl
