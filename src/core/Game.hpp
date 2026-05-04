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
    explicit Game(std::optional<Rng::Seed> seed = std::nullopt,
                  bool loadExisting = false);

    void run();

  private:
    void startNewGame(std::optional<Rng::Seed> seed);
    void tick(const std::vector<InputEvent>& events);
    void resolvePlayerTurn(bool runMonsters = true);
    void render();

    std::optional<Rng::Seed> initialSeed_;
    RunState state_ = RunState::MainMenu;
    int menuSelection_ = 0;
    ConsoleBuffer console_{80, 50};
    Map map_{80, 43, TileType::Wall};
    entt::registry registry_;
    GameResources resources_;
    ShadowcastingFov fovProvider_;
    std::unique_ptr<IRenderer> renderer_;
    bool running_ = true;
};

} // namespace rl
