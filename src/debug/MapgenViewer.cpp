/**
 * **************************************************************************
 * @file     MapgenViewer.cpp
 * @brief    Prints generated maps, snapshots, seeds, and validation results
 * **************************************************************************
 */

#include "debug/MapgenViewer.hpp"

#include <ostream>
#include <stdexcept>

#include "map/Map.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"

namespace rl {
namespace {

constexpr int viewerWidth = 40;
constexpr int viewerHeight = 25;

[[nodiscard]] char glyphForTile(TileType tile) noexcept
{
    switch (tile) {
    case TileType::Wall:
        return '#';
    case TileType::Floor:
        return '.';
    case TileType::DownStairs:
        return '>';
    }
    return '#';
}

void printMap(std::ostream& output, const Map& map)
{
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            output << glyphForTile(map.tileAt(x, y));
        }
        output << '\n';
    }
}

void printValidation(std::ostream& output,
                     const Map& map,
                     Point start,
                     Point stairs)
{
    const auto validation = validateMapBuild(map, start, stairs);
    output << "Validation: " << (validation.valid ? "ok" : "failed") << '\n';
    output << "Walkable: " << validation.walkableTiles
           << " Reachable: " << validation.reachableTiles << '\n';
    for (const auto& error : validation.errors) {
        output << "Error: " << error << '\n';
    }
}

} // namespace

std::vector<std::string> availableMapBuilders()
{
    return {"simple", "bsp", "bsp-interior"};
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
    throw std::invalid_argument("Unknown map builder");
}

int runMapgenViewer(const MapgenViewerOptions& options, std::ostream& output)
{
    if (!isKnownMapBuilder(options.builderId)) {
        output << "Unknown builder: " << options.builderId << '\n';
        output << "Available builders:";
        for (const auto& id : availableMapBuilders()) {
            output << ' ' << id;
        }
        output << '\n';
        return 1;
    }

    Rng rng{options.seed.value_or(Rng::makeSeed())};
    const auto seed = rng.seed();
    const MapBuildOptions buildOptions{options.showSnapshots};
    const auto result = buildMapById(
        options.builderId, viewerWidth, viewerHeight, rng, buildOptions);

    output << "Builder: " << result.builderName << '\n';
    output << "Seed: " << seed << '\n';
    output << "Start: " << result.playerStart.x << ',' << result.playerStart.y
           << " Stairs: " << result.stairsPosition.x << ','
           << result.stairsPosition.y << '\n';
    output << "Snapshots: " << result.snapshots.size() << '\n';
    printValidation(
        output, result.map, result.playerStart, result.stairsPosition);

    if (options.showSnapshots) {
        for (std::size_t index = 0; index < result.snapshots.size(); ++index) {
            output << "Snapshot " << (index + 1) << '/'
                   << result.snapshots.size() << ": "
                   << result.snapshots[index].label << '\n';
            printMap(output, result.snapshots[index].map);
        }
    } else {
        output << "Final map:\n";
        printMap(output, result.map);
    }

    return 0;
}

} // namespace rl
