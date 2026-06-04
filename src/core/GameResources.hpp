#pragma once

#include <entt/entt.hpp>

#include "core/GameLog.hpp"
#include "core/Point.hpp"
#include "core/Rng.hpp"
#include "map/MasterDungeonMap.hpp"
#include "render/Color.hpp"
#include "world/WorldTypes.hpp"

#include <optional>
#include <string>
#include <vector>

namespace rl {

enum class TargetingMode { Item, Spell, Shoot };

struct PendingTargeting {
    TargetingMode mode = TargetingMode::Item;
    entt::entity item = entt::null;
    std::string spellId;
    int manaCost = 0;
    int range = 0;
    std::optional<int> radius;
    Point cursor;
};

enum class VendorMode { Buy, Sell };

struct PendingVendor {
    entt::entity vendor = entt::null;
    VendorMode mode = VendorMode::Buy;
};

enum class ItemSelectionMode { Identify, RemoveCurse };

struct PendingItemSelection {
    ItemSelectionMode mode = ItemSelectionMode::Identify;
    entt::entity source = entt::null;
};

struct GameProgress {
    bool artifactClaimed = false;
};

struct ParticleRequest {
    Point position;
    char32_t glyph = U'*';
    Color foreground = Color::white();
    Color background = Color::black();
    int lifetimeFrames = 8;
};

struct GameResources {
    GameResources() : rng(), log() {}

    Rng rng;
    GameLog log;
    entt::entity player = entt::null;
    int depth = 1;
    std::string mapBuilderId = "normal";
    MasterDungeonMap dungeon;
    WorldMapState world;
    int inventorySelection = 0;
    int vendorSelection = 0;
    int spellSelection = 0;
    int itemSelection = 0;
    std::optional<PendingTargeting> pendingTargeting;
    std::optional<PendingVendor> pendingVendor;
    std::optional<PendingItemSelection> pendingItemSelection;
    GameProgress progress;
    bool townPortalActive = false;
    int townPortalReturnDepth = 1;
    std::optional<Point> townPortalReturnPosition;
    std::optional<Point> townPortalPosition;
    std::vector<ParticleRequest> particleRequests;
};

} // namespace rl
