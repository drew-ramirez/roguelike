/**
 * **************************************************************************
 * @file     MapBuilderRegistry.cpp
 * @brief    Implements registered map builder lookup and dispatch
 * **************************************************************************
 */

#include "map/MapBuilderRegistry.hpp"

#include <stdexcept>

#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/BuilderChain.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"
#include "map/generators/DlaBuilder.hpp"
#include "map/generators/DrunkardsWalkBuilder.hpp"
#include "map/generators/MazeBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "map/generators/VoronoiBuilder.hpp"
#include "map/generators/WfcBuilder.hpp"

namespace rl {
namespace {

MapBuildResult withBuilderName(MapBuildResult result, std::string_view id)
{
    result.builderName = std::string{id};
    return result;
}

} // namespace

std::vector<std::string> availableMapBuilders()
{
    return {"simple",
            "bsp",
            "bsp-interior",
            "cellular",
            "drunkard",
            "drunkard-open-area",
            "drunkard-open-halls",
            "drunkard-winding-passages",
            "drunkard-fat-passages",
            "drunkard-symmetry",
            "maze",
            "dla",
            "dla-walk-inwards",
            "dla-walk-outwards",
            "dla-central-attractor",
            "dla-insectoid",
            "voronoi",
            "voronoi-pythagoras",
            "voronoi-manhattan",
            "voronoi-chebyshev",
            "wfc",
            "cavern-vaults",
            "ruined-rooms"};
}

bool isKnownMapBuilder(std::string_view builderId)
{
    for (const auto& id : availableMapBuilders()) {
        if (id == builderId) {
            return true;
        }
    }
    return false;
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
    if (builderId == "cavern-vaults") {
        return BuilderChain::buildCavernVaults(width, height, rng, options);
    }
    if (builderId == "ruined-rooms") {
        return BuilderChain::buildRuinedRooms(width, height, rng, options);
    }
    throw std::invalid_argument("Unknown map builder");
}

} // namespace rl
