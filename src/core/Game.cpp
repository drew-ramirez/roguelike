#include "core/Game.hpp"

#include <chrono>
#include <thread>

#include "render/SdlRenderer.hpp"

namespace rl {

Game::Game()
    : renderer_(std::make_unique<SdlRenderer>(console_.width(), console_.height()))
{
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
    console_.print(2, 2, "Hello Roguelike", Color::yellow());
    console_.set(2, 4, U'@', Color::cyan());

    if (state_ == RunState::GameOver) {
        running_ = false;
    }
}

} // namespace rl
