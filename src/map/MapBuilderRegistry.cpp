/**
 * **************************************************************************
 * @file     MapBuilderRegistry.cpp
 * @brief    Implements registered map builder lookup and dispatch
 * **************************************************************************
 */

#include "map/MapBuilderRegistry.hpp"

#include <array>
#include <stdexcept>

#include "map/DungeonRegionRouter.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/TileType.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/BuilderChain.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"
#include "map/generators/DlaBuilder.hpp"
#include "map/generators/DrunkardsWalkBuilder.hpp"
#include "map/generators/MazeBuilder.hpp"
#include "map/generators/Section4Builders.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "map/generators/VoronoiBuilder.hpp"
#include "map/generators/WfcBuilder.hpp"

namespace rl {
namespace {

constexpr std::array<MapBuilderDescriptor, 41> builderCatalog{{
    {"simple", MapBuilderCategory::Algorithm},
    {"bsp", MapBuilderCategory::Algorithm},
    {"bsp-interior", MapBuilderCategory::Algorithm},
    {"cellular", MapBuilderCategory::Algorithm},
    {"drunkard", MapBuilderCategory::Algorithm},
    {"drunkard-open-area", MapBuilderCategory::Algorithm},
    {"drunkard-open-halls", MapBuilderCategory::Algorithm},
    {"drunkard-winding-passages", MapBuilderCategory::Algorithm},
    {"drunkard-fat-passages", MapBuilderCategory::Algorithm},
    {"drunkard-symmetry", MapBuilderCategory::Algorithm},
    {"maze", MapBuilderCategory::Algorithm},
    {"dla", MapBuilderCategory::Algorithm},
    {"dla-walk-inwards", MapBuilderCategory::Algorithm},
    {"dla-walk-outwards", MapBuilderCategory::Algorithm},
    {"dla-central-attractor", MapBuilderCategory::Algorithm},
    {"dla-insectoid", MapBuilderCategory::Algorithm},
    {"voronoi", MapBuilderCategory::Algorithm},
    {"voronoi-pythagoras", MapBuilderCategory::Algorithm},
    {"voronoi-manhattan", MapBuilderCategory::Algorithm},
    {"voronoi-chebyshev", MapBuilderCategory::Algorithm},
    {"wfc", MapBuilderCategory::Algorithm},
    {"cavern-vaults", MapBuilderCategory::Composition},
    {"ruined-rooms", MapBuilderCategory::Composition},
    {"town", MapBuilderCategory::Surface},
    {"woods", MapBuilderCategory::Surface},
    {"surface-town-outskirts", MapBuilderCategory::Surface},
    {"surface-cardinal-transition", MapBuilderCategory::Surface},
    {"surface-corner-wilderness", MapBuilderCategory::Surface},
    {"surface-ruin-pocket", MapBuilderCategory::Surface},
    {"surface-cave-mouth", MapBuilderCategory::Surface},
    {"surface-camp-shrine", MapBuilderCategory::Surface},
    {"surface-resource-heavy", MapBuilderCategory::Surface},
    {"surface-danger-encounter", MapBuilderCategory::Surface},
    {"limestone-caverns", MapBuilderCategory::DungeonRegion},
    {"deep-caverns", MapBuilderCategory::DungeonRegion},
    {"dwarf-transition", MapBuilderCategory::DungeonRegion},
    {"dragon-lair", MapBuilderCategory::DungeonRegion},
    {"mushroom-entrance", MapBuilderCategory::DungeonRegion},
    {"mushroom-forest", MapBuilderCategory::DungeonRegion},
    {"mushroom-exit", MapBuilderCategory::DungeonRegion},
    {"dark-elf-city", MapBuilderCategory::DungeonRegion},
}};

MapBuildResult withBuilderName(MapBuildResult result, std::string_view id)
{
    result.builderName = std::string{id};
    return result;
}

} // namespace

std::span<const MapBuilderDescriptor> mapBuilderCatalog() noexcept
{
    return builderCatalog;
}

const MapBuilderDescriptor*
mapBuilderDescriptor(std::string_view builderId) noexcept
{
    for (const auto& descriptor : builderCatalog) {
        if (descriptor.id == builderId) {
            return &descriptor;
        }
    }
    return nullptr;
}

std::string_view mapBuilderCategoryName(MapBuilderCategory category) noexcept
{
    switch (category) {
    case MapBuilderCategory::Algorithm:
        return "Algorithms";
    case MapBuilderCategory::Composition:
        return "Compositions";
    case MapBuilderCategory::Surface:
        return "Surface";
    case MapBuilderCategory::DungeonRegion:
        return "Dungeon Regions";
    }
    return "Algorithms";
}

std::vector<std::string> availableMapBuilders()
{
    std::vector<std::string> builders;
    builders.reserve(builderCatalog.size());
    for (const auto& descriptor : builderCatalog) {
        builders.emplace_back(descriptor.id);
    }
    return builders;
}

bool isKnownMapBuilder(std::string_view builderId)
{
    return mapBuilderDescriptor(builderId) != nullptr;
}

MapBuildResult buildMapById(std::string_view builderId,
                            int width,
                            int height,
                            Rng& rng,
                            MapBuildOptions options)
{
    if (builderId == "simple") {
        return SimpleMapBuilder::build(width, height, rng, options);
    }
    if (builderId == "bsp") {
        return BspDungeonBuilder::build(width, height, rng, options);
    }
    if (builderId == "bsp-interior") {
        return BspInteriorBuilder::build(width, height, rng, options);
    }
    if (builderId == "cellular") {
        return CellularAutomataBuilder::build(width, height, rng, options);
    }
    if (builderId == "drunkard") {
        return DrunkardsWalkBuilder::build(width, height, rng, options);
    }
    if (builderId == "drunkard-open-area") {
        return withBuilderName(DrunkardsWalkBuilder::buildWithSettings(
                                   width,
                                   height,
                                   rng,
                                   DrunkardsWalkBuilder::openAreaSettings(),
                                   options),
                               builderId);
    }
    if (builderId == "drunkard-open-halls") {
        return withBuilderName(DrunkardsWalkBuilder::buildWithSettings(
                                   width,
                                   height,
                                   rng,
                                   DrunkardsWalkBuilder::openHallsSettings(),
                                   options),
                               builderId);
    }
    if (builderId == "drunkard-winding-passages") {
        return withBuilderName(
            DrunkardsWalkBuilder::buildWithSettings(
                width,
                height,
                rng,
                DrunkardsWalkBuilder::windingPassagesSettings(),
                options),
            builderId);
    }
    if (builderId == "drunkard-fat-passages") {
        return withBuilderName(DrunkardsWalkBuilder::buildWithSettings(
                                   width,
                                   height,
                                   rng,
                                   DrunkardsWalkBuilder::fatPassagesSettings(),
                                   options),
                               builderId);
    }
    if (builderId == "drunkard-symmetry") {
        return withBuilderName(
            DrunkardsWalkBuilder::buildWithSettings(
                width,
                height,
                rng,
                DrunkardsWalkBuilder::fearfulSymmetrySettings(),
                options),
            builderId);
    }
    if (builderId == "maze") {
        return MazeBuilder::build(width, height, rng, options);
    }
    if (builderId == "dla") {
        return DlaBuilder::build(width, height, rng, options);
    }
    if (builderId == "dla-walk-inwards") {
        return withBuilderName(
            DlaBuilder::buildWithSettings(
                width, height, rng, DlaBuilder::walkInwardsSettings(), options),
            builderId);
    }
    if (builderId == "dla-walk-outwards") {
        return withBuilderName(
            DlaBuilder::buildWithSettings(width,
                                          height,
                                          rng,
                                          DlaBuilder::walkOutwardsSettings(),
                                          options),
            builderId);
    }
    if (builderId == "dla-central-attractor") {
        return withBuilderName(DlaBuilder::buildWithSettings(
                                   width,
                                   height,
                                   rng,
                                   DlaBuilder::centralAttractorSettings(),
                                   options),
                               builderId);
    }
    if (builderId == "dla-insectoid") {
        return withBuilderName(
            DlaBuilder::buildWithSettings(
                width, height, rng, DlaBuilder::insectoidSettings(), options),
            builderId);
    }
    if (builderId == "voronoi") {
        return VoronoiBuilder::build(width, height, rng, options);
    }
    if (builderId == "voronoi-pythagoras") {
        return withBuilderName(
            VoronoiBuilder::buildWithSettings(
                width, height, rng, VoronoiSettings{}, options),
            builderId);
    }
    if (builderId == "voronoi-manhattan") {
        return withBuilderName(
            VoronoiBuilder::buildWithSettings(
                width,
                height,
                rng,
                VoronoiSettings{64, VoronoiDistance::Manhattan},
                options),
            builderId);
    }
    if (builderId == "voronoi-chebyshev") {
        return withBuilderName(
            VoronoiBuilder::buildWithSettings(
                width,
                height,
                rng,
                VoronoiSettings{64, VoronoiDistance::Chebyshev},
                options),
            builderId);
    }
    if (builderId == "wfc") {
        return WfcBuilder::build(width, height, rng, options);
    }
    if (builderId == "town") {
        return Section4Builders::buildTown(width, height, rng, options);
    }
    if (builderId == "woods") {
        return Section4Builders::buildWoods(width, height, rng, options);
    }
    if (builderId == "surface-town-outskirts") {
        return Section4Builders::buildTownOutskirts(
            width, height, rng, options);
    }
    if (builderId == "surface-cardinal-transition") {
        return Section4Builders::buildSurfaceCardinalTransition(
            width, height, rng, options);
    }
    if (builderId == "surface-corner-wilderness") {
        return Section4Builders::buildSurfaceCornerWilderness(
            width, height, rng, options);
    }
    if (builderId == "surface-ruin-pocket") {
        return Section4Builders::buildSurfaceRuinPocket(
            width, height, rng, options);
    }
    if (builderId == "surface-cave-mouth") {
        return Section4Builders::buildSurfaceCaveMouth(
            width, height, rng, options);
    }
    if (builderId == "surface-camp-shrine") {
        return Section4Builders::buildSurfaceCampShrine(
            width, height, rng, options);
    }
    if (builderId == "surface-resource-heavy") {
        return Section4Builders::buildSurfaceResourceHeavy(
            width, height, rng, options);
    }
    if (builderId == "surface-danger-encounter") {
        return Section4Builders::buildSurfaceDangerEncounter(
            width, height, rng, options);
    }
    if (builderId == "limestone-caverns") {
        return Section4Builders::buildLimestoneCaverns(
            width, height, rng, options);
    }
    if (builderId == "deep-caverns") {
        return Section4Builders::buildDeepCaverns(width, height, rng, options);
    }
    if (builderId == "dwarf-transition") {
        return Section4Builders::buildDwarfTransition(
            width, height, rng, options);
    }
    if (builderId == "dragon-lair") {
        return Section4Builders::buildDragonLair(width, height, rng, options);
    }
    if (builderId == "mushroom-entrance") {
        return Section4Builders::buildMushroomEntrance(
            width, height, rng, options);
    }
    if (builderId == "mushroom-forest") {
        return Section4Builders::buildMushroomForest(
            width, height, rng, options);
    }
    if (builderId == "mushroom-exit") {
        return Section4Builders::buildMushroomExit(width, height, rng, options);
    }
    if (builderId == "dark-elf-city") {
        return Section4Builders::buildDarkElfCity(width, height, rng, options);
    }
    if (builderId == "cavern-vaults") {
        return BuilderChain::buildCavernVaults(width, height, rng, options);
    }
    if (builderId == "ruined-rooms") {
        return BuilderChain::buildRuinedRooms(width, height, rng, options);
    }
    throw std::invalid_argument("Unknown map builder");
}

MapBuildResult buildLevelForDepth(
    int depth, int width, int height, Rng& rng, MapBuildOptions options)
{
    const auto route = routeForDepth(depth);
    options.depth = depth;
    options.placeUpStairs = route.hasUpStairs;
    auto result = buildMapById(route.builderId, width, height, rng, options);
    result.builderName = route.builderId;
    if (!route.hasDownStairs) {
        result.map.setTile(
            result.stairsPosition.x, result.stairsPosition.y, TileType::Floor);
        result.stairsPosition = result.playerStart;
    }
    if (route.hasUpStairs) {
        setUpStairsAtStart(
            result.map, result.playerStart, result.upStairsPosition);
    }
    return result;
}

} // namespace rl
