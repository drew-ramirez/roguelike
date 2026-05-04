/**
 * **************************************************************************
 * @file     Game.cpp
 * @brief    Wires startup, turns, rendering, and the playable loop
 * **************************************************************************
 */

#include "core/Game.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

#include "map/generators/SimpleMapBuilder.hpp"
#include "render/SdlRenderer.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/MapTransitionSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"
#include "systems/MonsterAI.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/TrapSystem.hpp"
#include "systems/VisibilitySystem.hpp"
#include "ui/Hud.hpp"
#include "ui/InventoryMenu.hpp"
#include "ui/MainMenu.hpp"
#include "ui/Targeting.hpp"

namespace rl {

Game::Game(std::optional<Rng::Seed> seed, bool loadExisting)
    : initialSeed_(seed), renderer_(std::make_unique<SdlRenderer>(
                              console_.width(), console_.height()))
{
    if (loadExisting &&
        loadGame(defaultSavePath(), registry_, resources_, map_)) {
        resources_.log.add("Loaded saved game.");
        updateVisibility(registry_, map_, resources_.player, fovProvider_);
        state_ = RunState::AwaitingInput;
        return;
    }

    if (loadExisting) {
        startNewGame(seed);
    }
}

void Game::startNewGame(std::optional<Rng::Seed> seed)
{
    registry_ = entt::registry{};
    resources_ = GameResources{};
    map_ = Map{80, 43, TileType::Wall};
    resources_.rng.reseed(seed.value_or(Rng::makeSeed()));
    std::cerr << "Map seed: " << resources_.rng.seed() << '\n';

    auto build =
        SimpleMapBuilder::build(map_.width(), map_.height(), resources_.rng);
    map_ = std::move(build.map);
    resources_.player = spawnPlayer(registry_, build.playerStart);
    resources_.log.add("Entered the dungeon.");
    const auto spawned = spawnMonsters(registry_,
                                       build.rooms,
                                       resources_.rng,
                                       build.playerStart,
                                       resources_.depth);
    [[maybe_unused]] const auto spawnedItems = spawnItems(registry_,
                                                          build.rooms,
                                                          resources_.rng,
                                                          build.playerStart,
                                                          resources_.depth);
    [[maybe_unused]] const auto spawnedTraps = spawnTraps(registry_,
                                                          build.rooms,
                                                          resources_.rng,
                                                          build.playerStart,
                                                          build.stairsPosition,
                                                          resources_.depth);
    if (spawned > 0) {
        resources_.log.add("You hear movement in the dark.");
    }
    updateVisibility(registry_, map_, resources_.player, fovProvider_);
    state_ = RunState::AwaitingInput;
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
    updateParticleLifetimes(registry_);

    for (const auto& event : events) {
        if (event.type == InputEventType::Quit) {
            running_ = false;
        }
    }

    if (state_ == RunState::MainMenu || state_ == RunState::GameOver) {
        const bool loadAvailable = std::filesystem::exists(defaultSavePath());
        const MainMenuMode mode = state_ == RunState::MainMenu
                                      ? MainMenuMode::Title
                                      : MainMenuMode::GameOver;
        const auto action =
            handleMainMenuInput(events, loadAvailable, mode, menuSelection_);
        if (action == MainMenuAction::NewGame) {
            startNewGame(initialSeed_);
        } else if (action == MainMenuAction::LoadGame && loadAvailable) {
            if (loadGame(defaultSavePath(), registry_, resources_, map_)) {
                resources_.log.add("Loaded saved game.");
                updateVisibility(
                    registry_, map_, resources_.player, fovProvider_);
                state_ = RunState::AwaitingInput;
            }
        } else if (action == MainMenuAction::Quit) {
            running_ = false;
        }
        render();
        return;
    }

    if (state_ == RunState::AwaitingInput) {
        const auto input = handleMapInput(registry_, resources_, map_, events);
        if (input == PlayerInputResult::TookTurn) {
            resolvePlayerTurn();
        } else if (input == PlayerInputResult::OpenInventory) {
            state_ = RunState::ShowInventory;
        } else if (input == PlayerInputResult::Descend) {
            if (tryDescendToNextLevel(
                    registry_, resources_, map_, fovProvider_)) {
                state_ = RunState::AwaitingInput;
            }
        } else if (input == PlayerInputResult::Save) {
            try {
                saveGame(defaultSavePath(), registry_, resources_, map_);
                resources_.log.add("Game saved.");
            } catch (const std::exception& error) {
                resources_.log.add(std::string{"Save failed: "} + error.what());
            }
        }
    } else if (state_ == RunState::ShowInventory) {
        const auto input =
            handleInventoryInput(registry_, resources_, map_, events);
        if (input == InventoryInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == InventoryInputResult::StartedTargeting) {
            state_ = RunState::ShowTargeting;
        } else if (input == InventoryInputResult::TookTurn) {
            resolvePlayerTurn();
        }
    } else if (state_ == RunState::ShowTargeting) {
        const auto input =
            handleTargetingInput(registry_, resources_, map_, events);
        if (input == TargetingInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == TargetingInputResult::TookTurn) {
            resolvePlayerTurn();
        }
    }

    render();
}

void Game::resolvePlayerTurn(bool runMonsters)
{
    state_ = RunState::PlayerTurn;
    processPickupItems(registry_, resources_);
    [[maybe_unused]] const bool usedItem =
        processItemUses(registry_, resources_, map_);
    processMeleeCombat(registry_, resources_.log);
    processPlayerTraps(registry_, resources_);
    processHunger(registry_, resources_);
    applyDamageAndCleanup(registry_, resources_, state_, &map_);
    updateVisibility(registry_, map_, resources_.player, fovProvider_);

    if (state_ != RunState::GameOver && runMonsters) {
        state_ = RunState::MonsterTurn;
        runMonsterAi(registry_, resources_, map_);
        processMeleeCombat(registry_, resources_.log);
        applyDamageAndCleanup(registry_, resources_, state_, &map_);
    }

    flushParticleRequests(registry_, resources_);

    if (state_ != RunState::GameOver) {
        state_ = RunState::AwaitingInput;
    }
}

void Game::render()
{
    console_.clear();
    if (state_ == RunState::MainMenu || state_ == RunState::GameOver) {
        drawMainMenu(console_,
                     std::filesystem::exists(defaultSavePath()),
                     state_ == RunState::MainMenu ? MainMenuMode::Title
                                                  : MainMenuMode::GameOver,
                     menuSelection_,
                     "assets/title.txt");
        return;
    }

    renderMap(map_, console_);
    renderEntities(registry_, map_, console_);
    if (state_ == RunState::ShowTargeting) {
        drawTargetingOverlay(console_, registry_, resources_, map_);
    }
    drawHud(console_, resources_, registry_, state_);
    if (state_ == RunState::ShowInventory) {
        drawInventoryMenu(console_, registry_, resources_);
    }
}

} // namespace rl
