/**
 * **************************************************************************
 * @file     Game.hpp
 * @brief    Declares the main game loop owner and engine service wiring
 * **************************************************************************
 */

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "core/RunState.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "input/InputEvent.hpp"
#include "map/Map.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/IRenderer.hpp"

namespace rl {

class Game {
  public:
    explicit Game(std::optional<Rng::Seed> seed = std::nullopt);

    void run();

  private:
    void tick(const std::vector<InputEvent>& events);
    void render();

    RunState state_ = RunState::AwaitingInput;
    ConsoleBuffer console_{80, 50};
    Map map_{80, 43, TileType::Wall};
    entt::registry registry_;
    GameResources resources_;
    ShadowcastingFov fovProvider_;
    std::unique_ptr<IRenderer> renderer_;
    bool running_ = true;
};

} // namespace rl
