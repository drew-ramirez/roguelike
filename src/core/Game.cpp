#include "core/Game.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

#include "map/generators/SimpleMapBuilder.hpp"
#include "render/SdlRenderer.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/VisibilitySystem.hpp"

namespace rl {

Game::Game(std::optional<Rng::Seed> seed)
    : renderer_(std::make_unique<SdlRenderer>(console_.width(), console_.height()))
{
    resources_.rng.reseed(seed.value_or(Rng::makeSeed()));
    std::cerr << "Map seed: " << resources_.rng.seed() << '\n';

    auto build = SimpleMapBuilder::build(map_.width(), map_.height(), resources_.rng);
    map_ = std::move(build.map);
    resources_.player = spawnPlayer(registry_, build.playerStart);
    resources_.log.add("Entered the dungeon.");
    const auto spawned = spawnMonsters(registry_, build.rooms, resources_.rng, build.playerStart);
    if (spawned > 0) {
        resources_.log.add("You hear movement in the dark.");
    }
    updateVisibility(registry_, map_, resources_.player, fovProvider_);
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

    if (state_ != RunState::GameOver) {
        const bool playerActed = handlePlayerInput(registry_, resources_, map_, events);
        if (playerActed) {
            state_ = RunState::PlayerTurn;
            processMeleeCombat(registry_, resources_.log);
            applyDamageAndCleanup(registry_, resources_, state_);
            updateVisibility(registry_, map_, resources_.player, fovProvider_);

            if (state_ != RunState::GameOver) {
                state_ = RunState::AwaitingInput;
            }
        }
    }

    render();
}

void Game::render()
{
    console_.clear();
    renderMap(map_, console_);
    renderEntities(registry_, map_, console_);
}

} // namespace rl