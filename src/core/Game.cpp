#include "core/Game.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

#include "map/generators/SimpleMapBuilder.hpp"
#include "render/SdlRenderer.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/RenderSystem.hpp"

namespace rl {

Game::Game(std::optional<Rng::Seed> seed)
    : renderer_(std::make_unique<SdlRenderer>(console_.width(), console_.height()))
{
    resources_.rng.reseed(seed.value_or(Rng::makeSeed()));
    std::cerr << "Map seed: " << resources_.rng.seed() << '\n';

    auto build = SimpleMapBuilder::build(map_.width(), map_.height(), resources_.rng);
    map_ = std::move(build.map);
    resources_.player = spawnPlayer(registry_, build.playerStart);
}

void Game::run()
{
    while (running_ && !renderer_->shouldQuit()) {
        const auto events = renderer_->pollEvents();
        tick(events);
        renderer_->present(console_);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Game::tick(const std::vector<InputEvent>& events)
{
    for (const auto& event : events) {
        if (event.type == InputEventType::Quit) {
            running_ = false;
        }
    }

    handlePlayerInput(registry_, resources_, map_, events);

    console_.clear();
    renderMap(map_, console_);
    renderEntities(registry_, console_);

    if (state_ == RunState::GameOver) {
        running_ = false;
    }
}

} // namespace rl
