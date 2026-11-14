/**
 * **************************************************************************
 * @file     Game.hpp
 * @brief    Declares the main game loop owner and engine service wiring
 * **************************************************************************
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "core/Rng.hpp"
#include "core/RunState.hpp"
#include "debug/MapgenGallery.hpp"
#include "debug/MapgenPlayback.hpp"
#include "fov/ShadowcastingFov.hpp"
#include "input/InputEvent.hpp"
#include "map/Map.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "render/IRenderer.hpp"
#include "render/MapTheme.hpp"

namespace rl {

class Game {
  public:
    explicit Game(std::optional<Rng::Seed> seed = std::nullopt,
                  bool loadExisting = false,
                  std::string builderId = "normal",
                  int mapWidth = 80,
                  int mapHeight = 43,
                  bool visualMapgen = false,
                  MapTheme mapTheme = defaultMapTheme(),
                  bool startInGallery = false,
                  BiomeVariant galleryBiome = BiomeVariant::DryWoods,
                  std::string galleryBuilderId = "simple");

    void run();

  private:
    void startNewGame(std::optional<Rng::Seed> seed);
    void showMapgenGallery();
    void beginVisualMapgen(std::optional<Rng::Seed> seed);
    void commitGeneratedMap(MapBuildResult build);
    void tickMapGeneration(const std::vector<InputEvent>& events);
    void tickMapgenGallery(const std::vector<InputEvent>& events);
    void tick(const std::vector<InputEvent>& events);
    void resolvePlayerTurn(bool runMonsters = true);
    void render();
    void renderMapGeneration();

    std::optional<Rng::Seed> initialSeed_;
    std::string builderId_ = "normal";
    bool visualMapgen_ = false;
    BiomeVariant galleryBiome_ = BiomeVariant::DryWoods;
    std::string galleryBuilderId_ = "simple";
    MapTheme mapTheme_;
    RunState state_ = RunState::MainMenu;
    int menuSelection_ = 0;
    ConsoleBuffer console_{80, 50};
    Camera camera_{80, 43};
    Map map_{80, 43, TileType::Wall};
    std::optional<MapBuildResult> pendingMapgen_;
    MapgenPlayback mapgenPlayback_;
    MapgenGalleryState mapgenGallery_;
    entt::registry registry_;
    GameResources resources_;
    ShadowcastingFov fovProvider_;
    std::unique_ptr<IRenderer> renderer_;
    bool running_ = true;
};

} // namespace rl
