/**
 * **************************************************************************
 * @file     WorldTypes.hpp
 * @brief    Declares macro-world descriptors and surface screen state
 * **************************************************************************
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/Point.hpp"
#include "map/Map.hpp"
#include "map/MasterDungeonMap.hpp"
#include "map/TileType.hpp"
#include "render/Color.hpp"

namespace rl {

inline constexpr int worldMapWidth = 8;
inline constexpr int worldMapHeight = 6;
inline constexpr int localScreenGridSize = 3;

inline constexpr int exitNorth = 1 << 0;
inline constexpr int exitEast = 1 << 1;
inline constexpr int exitSouth = 1 << 2;
inline constexpr int exitWest = 1 << 3;

enum class BiomeVariant {
    DryWoods,
    WetWoods,
    DeepGrove,
    ReedMarsh,
    Sinklands,
    LimestoneRise,
    RuinedWoodland,
    DangerousWilds
};

enum class LocalScreenRole {
    CenterLandmark,
    CardinalTransition,
    CornerWilderness,
    RuinPocket,
    CaveMouth,
    CampShrine,
    ResourceHeavy,
    TownOutskirts,
    DangerEncounter
};

struct WorldCellFields {
    double elevation = 0.0;
    double moisture = 0.0;
    double woodlandDensity = 0.0;
    double marshPressure = 0.0;
    double ruinDensity = 0.0;
    double dangerPressure = 0.0;
};

struct WorldLandmark {
    std::string id;
    std::string name;
    bool fastTravelUnlocked = false;
};

struct WorldCellDescriptor {
    Point position;
    WorldCellFields fields;
    BiomeVariant biome = BiomeVariant::DryWoods;
    std::string name;
    char32_t glyph = U'.';
    Color foreground = Color::white();
    bool discovered = false;
    bool visited = false;
    std::optional<WorldLandmark> landmark;
};

struct LocalScreenDescriptor {
    Point macro;
    Point screen;
    LocalScreenRole role = LocalScreenRole::CornerWilderness;
    int exitMask = 0;
    std::string builderId;
    std::string landmarkId;
    bool dangerEncounter = false;
};

struct SurfaceScreenKey {
    Point macro;
    Point screen;
    bool dangerEncounter = false;

    [[nodiscard]] bool operator<(const SurfaceScreenKey& other) const noexcept
    {
        if (macro.y != other.macro.y) {
            return macro.y < other.macro.y;
        }
        if (macro.x != other.macro.x) {
            return macro.x < other.macro.x;
        }
        if (screen.y != other.screen.y) {
            return screen.y < other.screen.y;
        }
        if (screen.x != other.screen.x) {
            return screen.x < other.screen.x;
        }
        return dangerEncounter < other.dangerEncounter;
    }
};

struct SurfaceScreenState {
    SurfaceScreenKey key;
    std::string builderId;
    Map map{1, 1, TileType::Wall};
    std::vector<StoredEntity> entities;
};

struct WorldMapState {
    bool active = false;
    bool onSurface = false;
    Point currentMacro;
    Point currentScreen{1, 1};
    bool currentDangerEncounter = false;
    Point cursor;
    int undergroundEntryDepth = 3;
    std::vector<WorldCellDescriptor> cells;
    std::map<SurfaceScreenKey, SurfaceScreenState> surfaceScreens;
    std::optional<Point> pendingLocalTravel;
};

} // namespace rl
