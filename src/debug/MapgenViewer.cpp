/**
 * **************************************************************************
 * @file     MapgenViewer.cpp
 * @brief    Prints generated maps, snapshots, seeds, and validation results
 * **************************************************************************
 */

#include "debug/MapgenViewer.hpp"

#include <ostream>

#include "map/Map.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MapValidation.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

[[nodiscard]] char glyphForTile(TileType tile) noexcept
{
    switch (tile) {
    case TileType::Wall:
        return '#';
    case TileType::Floor:
        return '.';
    case TileType::DownStairs:
        return '>';
    case TileType::ClosedDoor:
        return '+';
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
        options.builderId, options.width, options.height, rng, buildOptions);

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
