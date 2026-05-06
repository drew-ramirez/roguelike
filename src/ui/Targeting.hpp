/**
 * **************************************************************************
 * @file     Targeting.hpp
 * @brief    Declares keyboard targeting input and overlay rendering
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

#include "input/InputEvent.hpp"

namespace rl {

class Camera;
class ConsoleBuffer;
class Map;
struct GameResources;

enum class TargetingInputResult { None, Close, TookTurn };

[[nodiscard]] TargetingInputResult
handleTargetingInput(entt::registry& registry,
                     GameResources& resources,
                     const Map& map,
                     const std::vector<InputEvent>& events);

void drawTargetingOverlay(ConsoleBuffer& console,
                          const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map);
void drawTargetingOverlay(ConsoleBuffer& console,
                          const entt::registry& registry,
                          const GameResources& resources,
                          const Map& map,
                          const Camera& camera);

} // namespace rl
