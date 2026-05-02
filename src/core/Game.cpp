#include "core/Game.hpp"

#include <chrono>
#include <thread>

#include "core/Point.hpp"
#include "render/SdlRenderer.hpp"
#include "spawn/Spawner.hpp"
#include "systems/RenderSystem.hpp"

namespace rl {

Game::Game()
    : renderer_(std::make_unique<SdlRenderer>(console_.width(), console_.height()))
{
    resources_.player = spawnPlayer(registry_, Point{40, 22});
}

void Game::run()
{
    while (running_ && !renderer_->shouldQuit()) {
        renderer_->pollEvents();
        tick();
        renderer_->present(console_);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Game::tick()
{
    console_.clear();
    renderEntities(registry_, console_);

    if (state_ == RunState::GameOver) {
        running_ = false;
    }
}

} // namespace rl