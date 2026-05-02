#include "core/Game.hpp"

#include <chrono>
#include <thread>

#include "render/SdlRenderer.hpp"
#include "spawn/Spawner.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MapRenderSystem.hpp"
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
