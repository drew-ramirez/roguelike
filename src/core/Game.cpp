/**
 * **************************************************************************
 * @file     Game.cpp
 * @brief    Wires startup, turns, rendering, and the playable loop
 * **************************************************************************
 */

#include "core/Game.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include "ecs/Components.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "raws/RawDatabase.hpp"
#include "render/SdlRenderer.hpp"
#include "save/SaveGame.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/MonsterSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "spawn/TrapSpawner.hpp"
#include "systems/BystanderAI.hpp"
#include "systems/DamageSystem.hpp"
#include "systems/HungerSystem.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/MapRenderSystem.hpp"
#include "systems/MapTransitionSystem.hpp"
#include "systems/MeleeCombatSystem.hpp"
#include "systems/MonsterAI.hpp"
#include "systems/ParticleSystem.hpp"
#include "systems/PortalSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/SystemScheduler.hpp"
#include "systems/TrapSystem.hpp"
#include "systems/VendorSystem.hpp"
#include "systems/VisibilitySystem.hpp"
#include "systems/WorldTravelSystem.hpp"
#include "ui/CampMenu.hpp"
#include "ui/Hud.hpp"
#include "ui/InventoryMenu.hpp"
#include "ui/ItemSelectionMenu.hpp"
#include "ui/MainMenu.hpp"
#include "ui/SettlementServiceMenu.hpp"
#include "ui/SpellMenu.hpp"
#include "ui/Targeting.hpp"
#include "ui/VendorMenu.hpp"
#include "ui/WorldMapMenu.hpp"
#include "world/WorldGeneration.hpp"

namespace rl {
namespace {

constexpr int mapgenFrameMs = 300;
constexpr int tickFrameMs = 16;
constexpr int hudHeight = 7;

} // namespace

Game::Game(std::optional<Rng::Seed> seed,
           bool loadExisting,
           std::string builderId,
           int mapWidth,
           int mapHeight,
           bool visualMapgen,
           MapTheme mapTheme)
    : initialSeed_(seed), builderId_(std::move(builderId)),
      visualMapgen_(visualMapgen), mapTheme_(std::move(mapTheme)),
      camera_(console_.width(), console_.height() - hudHeight),
      map_(mapWidth, mapHeight, TileType::Wall),
      renderer_(
          std::make_unique<SdlRenderer>(console_.width(), console_.height()))
{
    if (loadExisting &&
        loadGame(defaultSavePath(), registry_, resources_, map_)) {
        resources_.log.add("Loaded saved game.");
        updateVisibility(registry_, map_, resources_.player, fovProvider_);
        state_ = RunState::AwaitingInput;
        return;
    }

    if (loadExisting) {
        if (visualMapgen_) {
            beginVisualMapgen(seed);
        } else {
            startNewGame(seed);
        }
        return;
    }

    if (visualMapgen_) {
        beginVisualMapgen(seed);
    }
}

void Game::startNewGame(std::optional<Rng::Seed> seed)
{
    registry_ = entt::registry{};
    resources_ = GameResources{};
    resources_.mapBuilderId = builderId_;
    map_ = Map{map_.width(), map_.height(), TileType::Wall};
    resources_.rng.reseed(seed.value_or(Rng::makeSeed()));
    std::cerr << "Map seed: " << resources_.rng.seed() << '\n';
    resources_.world = generateWorldMap(resources_.rng.seed());

    const auto options = MapBuildOptions{false, resources_.depth};
    if (resources_.mapBuilderId == "normal") {
        commitGeneratedMap(buildLevelForDepth(resources_.depth,
                                              map_.width(),
                                              map_.height(),
                                              resources_.rng,
                                              options));
    } else {
        commitGeneratedMap(buildMapById(resources_.mapBuilderId,
                                        map_.width(),
                                        map_.height(),
                                        resources_.rng,
                                        options));
    }
}

void Game::beginVisualMapgen(std::optional<Rng::Seed> seed)
{
    registry_ = entt::registry{};
    resources_ = GameResources{};
    resources_.mapBuilderId = builderId_;
    map_ = Map{map_.width(), map_.height(), TileType::Wall};
    resources_.rng.reseed(seed.value_or(Rng::makeSeed()));
    std::cerr << "Map seed: " << resources_.rng.seed() << '\n';
    resources_.world = generateWorldMap(resources_.rng.seed());

    const auto options = MapBuildOptions{true, resources_.depth};
    auto build = resources_.mapBuilderId == "normal"
                     ? buildLevelForDepth(resources_.depth,
                                          map_.width(),
                                          map_.height(),
                                          resources_.rng,
                                          options)
                     : buildMapById(resources_.mapBuilderId,
                                    map_.width(),
                                    map_.height(),
                                    resources_.rng,
                                    options);
    std::vector<MapSnapshot> frames = build.snapshots;
    frames.push_back(MapSnapshot{"final generated map", build.map});
    resetMapgenPlayback(mapgenPlayback_, std::move(frames));
    pendingMapgen_ = std::move(build);
    state_ = RunState::MapGeneration;
}

void Game::commitGeneratedMap(MapBuildResult build)
{
    map_ = std::move(build.map);
    resources_.player = spawnPlayer(registry_, build.playerStart);
    [[maybe_unused]] const auto requested =
        spawnRequestedEntities(registry_,
                               build.spawnRequests,
                               map_,
                               build.playerStart,
                               build.stairsPosition);
    populateAllVendorStock(registry_, defaultRawDatabase());
    const bool townStart =
        resources_.mapBuilderId == "normal" && resources_.depth == 1;
    resources_.log.add(townStart ? "Entered the town."
                                 : "Entered the dungeon.");
    const auto spawned = townStart ? 0
                                   : spawnMonsters(registry_,
                                                   build.spawnRegions,
                                                   map_,
                                                   resources_.rng,
                                                   build.playerStart,
                                                   resources_.depth);
    [[maybe_unused]] const auto spawnedItems =
        townStart ? 0
                  : spawnItems(registry_,
                               build.spawnRegions,
                               map_,
                               resources_.rng,
                               build.playerStart,
                               resources_.depth);
    [[maybe_unused]] const auto spawnedTraps =
        townStart ? 0
                  : spawnTraps(registry_,
                               build.spawnRegions,
                               map_,
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

void Game::tickMapGeneration(const std::vector<InputEvent>& events)
{
    const auto command = advanceMapgenPlayback(
        mapgenPlayback_, events, tickFrameMs, mapgenFrameMs);
    if (command == MapgenPlaybackCommand::Cancel) {
        pendingMapgen_.reset();
        resetMapgenPlayback(mapgenPlayback_, {});
        state_ = RunState::MainMenu;
    } else if (command == MapgenPlaybackCommand::Commit &&
               pendingMapgen_.has_value()) {
        auto build = std::move(*pendingMapgen_);
        pendingMapgen_.reset();
        resetMapgenPlayback(mapgenPlayback_, {});
        commitGeneratedMap(std::move(build));
    }
    render();
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

    if (state_ == RunState::MapGeneration) {
        tickMapGeneration(events);
        return;
    }

    if (state_ == RunState::MainMenu || state_ == RunState::GameOver) {
        const bool loadAvailable = std::filesystem::exists(defaultSavePath());
        const MainMenuMode mode = state_ == RunState::MainMenu
                                      ? MainMenuMode::Title
                                      : MainMenuMode::GameOver;
        const auto action =
            handleMainMenuInput(events, loadAvailable, mode, menuSelection_);
        if (action == MainMenuAction::NewGame) {
            if (visualMapgen_) {
                beginVisualMapgen(initialSeed_);
            } else {
                startNewGame(initialSeed_);
            }
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
        } else if (input == PlayerInputResult::OpenCamp) {
            state_ = RunState::ShowCamp;
        } else if (input == PlayerInputResult::OpenSettlementService) {
            state_ = RunState::ShowSettlementService;
        } else if (input == PlayerInputResult::OpenVendor) {
            state_ = RunState::ShowVendor;
        } else if (input == PlayerInputResult::OpenSpellMenu) {
            state_ = RunState::ShowSpellMenu;
        } else if (input == PlayerInputResult::StartedTargeting) {
            state_ = RunState::ShowTargeting;
        } else if (input == PlayerInputResult::OpenWorldMap) {
            state_ = RunState::ShowWorldMap;
        } else if (input == PlayerInputResult::TravelLocalEdge) {
            if (resources_.world.pendingLocalTravel.has_value() &&
                tryTravelByEdge(registry_,
                                resources_,
                                map_,
                                fovProvider_,
                                *resources_.world.pendingLocalTravel)) {
                state_ = RunState::AwaitingInput;
            }
            resources_.world.pendingLocalTravel.reset();
        } else if (input == PlayerInputResult::UsePortal) {
            if (tryUseReturnPortal(registry_, resources_, map_, fovProvider_)) {
                state_ = RunState::AwaitingInput;
            }
        } else if (input == PlayerInputResult::Descend) {
            if (tryDescendToNextLevel(
                    registry_, resources_, map_, fovProvider_)) {
                state_ = RunState::AwaitingInput;
            }
        } else if (input == PlayerInputResult::Ascend) {
            if (tryAscendToPreviousLevel(
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
        } else if (input == InventoryInputResult::StartedItemSelection) {
            state_ = RunState::ShowItemSelection;
        } else if (input == InventoryInputResult::TookTurn) {
            resolvePlayerTurn();
        }
    } else if (state_ == RunState::ShowCamp) {
        const auto input = handleCampInput(registry_, resources_, events);
        if (input == CampInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == CampInputResult::TookTurn) {
            resolvePlayerTurn(false);
        }
    } else if (state_ == RunState::ShowSettlementService) {
        const auto input =
            handleSettlementServiceInput(registry_, resources_, events);
        if (input == SettlementServiceInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == SettlementServiceInputResult::TookTurn) {
            resolvePlayerTurn(false);
        }
    } else if (state_ == RunState::ShowItemSelection) {
        const auto input =
            handleItemSelectionInput(registry_, resources_, map_, events);
        if (input == ItemSelectionInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == ItemSelectionInputResult::TookTurn) {
            resolvePlayerTurn();
        }
    } else if (state_ == RunState::ShowVendor) {
        const auto input = handleVendorInput(registry_, resources_, events);
        if (input == VendorInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == VendorInputResult::TookTurn) {
            resolvePlayerTurn(false);
        }
    } else if (state_ == RunState::ShowSpellMenu) {
        const auto input =
            handleSpellInput(registry_, resources_, map_, events);
        if (input == SpellInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == SpellInputResult::StartedTargeting) {
            state_ = RunState::ShowTargeting;
        } else if (input == SpellInputResult::TookTurn) {
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
    } else if (state_ == RunState::ShowWorldMap) {
        const auto input = handleWorldMapInput(resources_, events);
        if (input == WorldMapInputResult::Close) {
            state_ = RunState::AwaitingInput;
        } else if (input == WorldMapInputResult::MoveCursor) {
            if (resources_.world.pendingLocalTravel.has_value() &&
                moveWorldMapCursor(registry_,
                                   resources_,
                                   map_,
                                   fovProvider_,
                                   *resources_.world.pendingLocalTravel)) {
                state_ = RunState::AwaitingInput;
            }
            resources_.world.pendingLocalTravel.reset();
        } else if (input == WorldMapInputResult::EnterSelected) {
            if (enterWorldMapSelection(
                    registry_, resources_, map_, fovProvider_)) {
                state_ = RunState::AwaitingInput;
            }
        }
    }

    render();
}

void Game::resolvePlayerTurn(bool runMonsters)
{
    resolveTurnSystems(
        registry_, resources_, map_, fovProvider_, state_, runMonsters);
}

void Game::render()
{
    console_.clear();
    if (state_ == RunState::MapGeneration) {
        renderMapGeneration();
        return;
    }

    if (state_ == RunState::MainMenu || state_ == RunState::GameOver) {
        drawMainMenu(console_,
                     std::filesystem::exists(defaultSavePath()),
                     state_ == RunState::MainMenu ? MainMenuMode::Title
                                                  : MainMenuMode::GameOver,
                     menuSelection_,
                     "assets/title.txt");
        return;
    }

    if (resources_.player != entt::null && registry_.valid(resources_.player) &&
        registry_.all_of<Position>(resources_.player)) {
        const auto& position = registry_.get<Position>(resources_.player);
        camera_.centerOn(Point{position.x, position.y}, map_);
    }

    renderMap(map_, console_, camera_, mapTheme_);
    renderEntities(registry_, map_, console_, camera_);
    if (state_ == RunState::ShowTargeting) {
        drawTargetingOverlay(console_, registry_, resources_, map_, camera_);
    }
    drawHud(console_, resources_, registry_, state_);
    if (state_ == RunState::ShowInventory) {
        drawInventoryMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowCamp) {
        drawCampMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowSettlementService) {
        drawSettlementServiceMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowItemSelection) {
        drawItemSelectionMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowVendor) {
        drawVendorMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowSpellMenu) {
        drawSpellMenu(console_, registry_, resources_);
    } else if (state_ == RunState::ShowWorldMap) {
        drawWorldMapMenu(console_, resources_.world);
    }
}

void Game::renderMapGeneration()
{
    const auto* frame = currentMapgenFrame(mapgenPlayback_);
    if (frame == nullptr) {
        console_.print(1, 1, "No mapgen frames recorded.", Color::white());
        return;
    }

    const int viewportHeight = std::max(1, console_.height() - 2);
    Camera snapshotCamera{console_.width(), viewportHeight};
    snapshotCamera.centerOn(
        Point{frame->map.width() / 2, frame->map.height() / 2}, frame->map);
    renderMapDebug(frame->map, console_, snapshotCamera);

    const std::string status =
        "Mapgen " + builderId_ + " seed " +
        std::to_string(resources_.rng.seed()) + " frame " +
        std::to_string(mapgenPlayback_.frameIndex + 1) + "/" +
        std::to_string(mapgenPlayback_.frames.size()) + " " + frame->label;
    console_.print(1, console_.height() - 2, status, Color::yellow());
    console_.print(1,
                   console_.height() - 1,
                   "Space pause  Left/Right step  Enter play  Esc menu",
                   Color{170, 170, 170, 255});
}

} // namespace rl
