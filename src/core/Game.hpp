#pragma once

#include <memory>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/RunState.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/IRenderer.hpp"

namespace rl {

class Game {
public:
    Game();

    void run();

private:
    void tick();

    RunState state_ = RunState::AwaitingInput;
    ConsoleBuffer console_{80, 50};
    entt::registry registry_;
    GameResources resources_;
    std::unique_ptr<IRenderer> renderer_;
    bool running_ = true;
};

} // namespace rl