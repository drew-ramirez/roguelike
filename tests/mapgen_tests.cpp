#include "TestSuites.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Rng.hpp"
#include "debug/MapgenGallery.hpp"
#include "debug/MapgenPlayback.hpp"
#include "debug/MapgenViewer.hpp"
#include "map/MapBrush.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MapConnectivity.hpp"
#include "map/MapGeometry.hpp"
#include "map/MapQualityMetrics.hpp"
#include "map/MapValidation.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/BuilderChain.hpp"
#include "map/generators/DlaBuilder.hpp"
#include "map/generators/DrunkardsWalkBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "map/generators/VoronoiBuilder.hpp"
#include "map/generators/WfcBuilder.hpp"
#include "map/prefabs/Prefab.hpp"
#include "map/rooms/RoomCarver.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/MapRenderSystem.hpp"

namespace rl::test {
namespace {

struct OpennessMetrics {
    double walkablePercent = 0.0;
    double openNeighborhoodPercent = 0.0;
};

[[nodiscard]] bool bufferContains(const ConsoleBuffer& console,
                                  std::string_view text)
{
    if (text.empty()) {
        return true;
    }
    for (int y = 0; y < console.height(); ++y) {
        std::string row;
        row.reserve(static_cast<std::size_t>(console.width()));
        for (int x = 0; x < console.width(); ++x) {
            const auto glyph = console.cellAt(x, y).glyph;
            row.push_back(glyph <= 127 ? static_cast<char>(glyph) : '?');
        }
        if (row.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] int passableNeighborCount(const Map& map, Point point)
{
    int count = 0;
    for (int y = point.y - 1; y <= point.y + 1; ++y) {
        for (int x = point.x - 1; x <= point.x + 1; ++x) {
            if (x == point.x && y == point.y) {
                continue;
            }
            if (isConnectivityPassable(map, Point{x, y})) {
                ++count;
            }
        }
    }
    return count;
}

[[nodiscard]] OpennessMetrics measureOpenness(const Map& map)
{
    int walkable = 0;
    int wideOpen = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (!isConnectivityPassable(map, Point{x, y})) {
                continue;
            }
            ++walkable;
            if (passableNeighborCount(map, Point{x, y}) >= 7) {
                ++wideOpen;
            }
        }
    }

    const double totalTiles = static_cast<double>(map.width() * map.height());
    const double walkableTiles = static_cast<double>(walkable);
    return OpennessMetrics{(walkableTiles * 100.0) / totalTiles,
                           walkable == 0
                               ? 0.0
                               : (static_cast<double>(wideOpen) * 100.0) /
                                     walkableTiles};
}

[[nodiscard]] int countTiles(const Map& map, TileType tile)
{
    int count = 0;
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.tileAt(x, y) == tile) {
                ++count;
            }
        }
    }
    return count;
}

[[nodiscard]] bool touchesMapEdge(const Map& map, Point point) noexcept
{
    return point.x == 0 || point.y == 0 || point.x == map.width() - 1 ||
           point.y == map.height() - 1;
}

[[nodiscard]] bool isWallTile(const Map& map, Point point)
{
    return map.inBounds(point.x, point.y) &&
           map.tileAt(point.x, point.y) == TileType::Wall;
}

[[nodiscard]] int countSmallInteriorWallComponents(const Map& map,
                                                   std::size_t maxSize)
{
    std::vector<bool> visited(static_cast<std::size_t>(map.tileCount()), false);
    int count = 0;

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Point start{x, y};
            const auto startIndex =
                static_cast<std::size_t>(map.xyIdx(start.x, start.y));
            if (visited[startIndex] || !isWallTile(map, start)) {
                continue;
            }

            bool touchesEdge = false;
            std::vector<Point> component;
            std::vector<Point> frontier{start};
            visited[startIndex] = true;

            while (!frontier.empty()) {
                const Point current = frontier.back();
                frontier.pop_back();
                component.push_back(current);
                touchesEdge = touchesEdge || touchesMapEdge(map, current);

                for (const Point next : cardinalNeighbors(current)) {
                    if (!isWallTile(map, next)) {
                        continue;
                    }

                    const auto nextIndex =
                        static_cast<std::size_t>(map.xyIdx(next.x, next.y));
                    if (visited[nextIndex]) {
                        continue;
                    }

                    visited[nextIndex] = true;
                    frontier.push_back(next);
                }
            }

            if (!touchesEdge && component.size() <= maxSize) {
                ++count;
            }
        }
    }

    return count;
}

void carveVerticalDoorCandidate(Map& map, int x, int y)
{
    map.setTile(x, y - 1, TileType::Floor);
    map.setTile(x, y, TileType::Floor);
    map.setTile(x, y + 1, TileType::Floor);
}

[[nodiscard]] wfc::ChunkPattern wallChunkPattern()
{
    wfc::ChunkPattern pattern{};
    pattern.fill(TileType::Wall);
    return pattern;
}

void setChunkTile(wfc::ChunkPattern& pattern, int x, int y, TileType tile)
{
    pattern[static_cast<std::size_t>(y * wfc::chunkSize + x)] = tile;
}

[[nodiscard]] bool chunkHasFloorAt(const wfc::MapChunk& chunk, int x, int y)
{
    return chunk.pattern[static_cast<std::size_t>(y * wfc::chunkSize + x)] ==
           TileType::Floor;
}

[[nodiscard]] bool chunkCollectionHasFloorAt(
    const std::vector<wfc::MapChunk>& chunks, int x, int y)
{
    return std::any_of(chunks.begin(), chunks.end(), [x, y](const auto& chunk) {
        return chunkHasFloorAt(chunk, x, y);
    });
}

[[nodiscard]] wfc::SourceMap sourceFromPattern(const wfc::ChunkPattern& pattern)
{
    wfc::SourceMap source;
    source.width = wfc::chunkSize;
    source.height = wfc::chunkSize;
    source.tiles.assign(pattern.begin(), pattern.end());
    return source;
}

[[nodiscard]] bool compatibleListContains(const wfc::MapChunk& chunk,
                                          wfc::Direction direction,
                                          int expected)
{
    const auto& compatible = chunk.compatibleWith[static_cast<std::size_t>(
        wfc::directionIndex(direction))];
    return std::find(compatible.begin(), compatible.end(), expected) !=
           compatible.end();
}

[[nodiscard]] bool sameMapTiles(const Map& left, const Map& right)
{
    if (left.width() != right.width() || left.height() != right.height()) {
        return false;
    }
    for (int y = 0; y < left.height(); ++y) {
        for (int x = 0; x < left.width(); ++x) {
            if (left.tileAt(x, y) != right.tileAt(x, y)) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool hasSnapshotContaining(const MapBuildResult& result,
                                         std::string_view text)
{
    return std::any_of(result.snapshots.begin(),
                       result.snapshots.end(),
                       [text](const auto& s) {
                           return s.label.find(text) != std::string::npos;
                       });
}

[[nodiscard]] bool hasBuildStepContaining(const MapBuildResult& result,
                                          std::string_view text)
{
    return std::any_of(result.buildSteps.begin(),
                       result.buildSteps.end(),
                       [text](const auto& step) {
                           return step.find(text) != std::string::npos;
                       });
}

[[nodiscard]] bool hasSpawnRequest(const MapBuildResult& result,
                                   std::string_view id)
{
    return std::any_of(
        result.spawnRequests.begin(),
        result.spawnRequests.end(),
        [id](const SpawnRequest& request) { return request.id == id; });
}

[[nodiscard]] bool surfaceExitIsOpen(const Map& map, int exit)
{
    const Point center{map.width() / 2, map.height() / 2};
    if (exit == exitNorth) {
        return isConnectivityPassable(map, Point{center.x, 1});
    }
    if (exit == exitEast) {
        return isConnectivityPassable(map, Point{map.width() - 2, center.y});
    }
    if (exit == exitSouth) {
        return isConnectivityPassable(map, Point{center.x, map.height() - 2});
    }
    if (exit == exitWest) {
        return isConnectivityPassable(map, Point{1, center.y});
    }
    return false;
}

[[nodiscard]] bool surfaceExitIsThreeTilesWide(const Map& map, int exit)
{
    const Point center{map.width() / 2, map.height() / 2};
    for (int offset = -1; offset <= 1; ++offset) {
        Point edge;
        Point inside;
        if (exit == exitNorth || exit == exitSouth) {
            const int y = exit == exitNorth ? 0 : map.height() - 1;
            const int insideY = exit == exitNorth ? 1 : map.height() - 2;
            edge = Point{center.x + offset, y};
            inside = Point{center.x + offset, insideY};
        } else {
            const int x = exit == exitWest ? 0 : map.width() - 1;
            const int insideX = exit == exitWest ? 1 : map.width() - 2;
            edge = Point{x, center.y + offset};
            inside = Point{insideX, center.y + offset};
        }
        if (!isConnectivityPassable(map, edge) ||
            !isConnectivityPassable(map, inside)) {
            return false;
        }
    }
    return true;
}

} // namespace

void runMapgenTests(TestRunner& runner)
{
    {
        Map map{8, 6, TileType::Wall};
        map.setTile(1, 1, TileType::Floor);
        map.setTile(2, 1, TileType::DownStairs);
        map.setTile(6, 4, TileType::Floor);
        const auto regions = findConnectedRegions(map);
        runner.expect(regions.size() == 2,
                      "region finder separates disconnected floors");
        runner.expect(isConnectivityPassable(TileType::DownStairs),
                      "connectivity treats down stairs as passable");
        const auto distances = buildDistanceMap(map, Point{1, 1});
        runner.expect(distances.distanceAt(Point{2, 1}) == 1,
                      "distance map records reachable distance");
        runner.expect(distances.distanceAt(Point{6, 4}) == -1,
                      "distance map marks unreachable tiles");
        const auto invalidDistances = buildDistanceMap(map, Point{0, 0});
        runner.expect(invalidDistances.distanceAt(Point{1, 1}) == -1 &&
                          countReachableTiles(map, Point{0, 0}) == 0,
                      "distance map handles invalid starts as unreachable");
        const auto validation = validateMapBuild(map, Point{1, 1}, Point{2, 1});
        runner.expect(validation.walkableTiles ==
                              countConnectivityPassableTiles(map) &&
                          validation.reachableTiles ==
                              countReachableTiles(map, Point{1, 1}),
                      "map validation reports MapConnectivity tile counts");
        runner.expect(cullUnreachable(map, Point{1, 1}) == 1,
                      "reachability culling removes isolated floors");

        Rng rng{9U};
        map.setTile(6, 4, TileType::Floor);
        runner.expect(repairConnectivity(map, Point{1, 1}, rng) == 1,
                      "connectivity repair links isolated regions");
        const auto repaired = validateMapBuild(map, Point{1, 1}, Point{2, 1});
        runner.expect(repaired.reachableTiles == repaired.walkableTiles,
                      "repaired map has one reachable passable region");
    }

    {
        const std::vector<Point> includeStart{
            Point{1, 1},
            Point{2, 1},
            Point{3, 2},
            Point{4, 2},
        };
        const std::vector<Point> excludeStart{
            Point{2, 1},
            Point{3, 2},
            Point{4, 2},
        };

        runner.expect(bresenhamLine(Point{1, 1},
                                    Point{4, 2},
                                    LineStartPolicy::IncludeStart) ==
                          includeStart,
                      "map geometry includes line starts when requested");
        runner.expect(bresenhamLine(Point{1, 1},
                                    Point{4, 2},
                                    LineStartPolicy::ExcludeStart) ==
                          excludeStart,
                      "map geometry excludes line starts when requested");
    }

    {
        Map map{8, 8, TileType::Wall};
        const Rect room{2, 2, 5, 5};
        const auto carved = carveRectRoom(map, room);
        const auto allTiles = roomTiles(room);
        map.setTile(3, 3, TileType::Wall);
        const auto walkableTiles = walkableRoomTiles(map, room);

        runner.expect(carved.size() == 9U && allTiles.size() == 9U &&
                          map.isWalkable(4, 4),
                      "room helpers carve and enumerate rectangle rooms");
        runner.expect(walkableTiles.size() == 8U &&
                          std::find(walkableTiles.begin(),
                                    walkableTiles.end(),
                                    Point{3, 3}) == walkableTiles.end(),
                      "room helpers filter walkable room tiles");

        Rng rng{21U};
        const Point interior = randomInteriorPointInRoom(room, rng);
        runner.expect(interior.x >= room.x1 + 1 && interior.x <= room.x2 - 2 &&
                          interior.y >= room.y1 + 1 &&
                          interior.y <= room.y2 - 2,
                      "room helpers choose bounded interior points");
    }

    {
        Map map{7, 7, TileType::Wall};
        const auto points = paintBrush(map,
                                       Point{2, 3},
                                       TileType::Floor,
                                       BrushShape::Single,
                                       0,
                                       Symmetry::Horizontal);
        runner.expect(points.size() == 2,
                      "horizontal symmetry paints mirrored brush points");
        runner.expect(map.isWalkable(2, 3) && map.isWalkable(4, 3),
                      "symmetry writes both mirrored tiles");
    }

    {
        MapgenPlayback playback;
        std::vector<MapSnapshot> frames{
            MapSnapshot{"start", Map{5, 5, TileType::Wall}},
            MapSnapshot{"middle", Map{5, 5, TileType::Floor}},
            MapSnapshot{"end", Map{5, 5, TileType::Wall}},
        };
        resetMapgenPlayback(playback, std::move(frames));

        runner.expect(currentMapgenFrame(playback)->label == "start",
                      "visual mapgen playback starts on first frame");
        runner.expect(advanceMapgenPlayback(playback, {}, 150, 100) ==
                          MapgenPlaybackCommand::None,
                      "visual mapgen playback auto-advances without input");
        runner.expect(playback.frameIndex == 1U,
                      "visual mapgen playback records advanced frame index");

        const std::vector<InputEvent> pause{
            {InputEventType::KeyDown, Key::Space}};
        runner.expect(advanceMapgenPlayback(playback, pause, 150, 100) ==
                          MapgenPlaybackCommand::None,
                      "visual mapgen playback accepts pause input");
        runner.expect(playback.paused && playback.frameIndex == 1U,
                      "paused visual mapgen playback does not auto-advance");

        const std::vector<InputEvent> right{
            {InputEventType::KeyDown, Key::Right}};
        runner.expect(advanceMapgenPlayback(playback, right, 0, 100) ==
                          MapgenPlaybackCommand::None,
                      "visual mapgen playback steps forward");
        runner.expect(currentMapgenFrame(playback)->label == "end",
                      "visual mapgen playback exposes stepped frame");

        const std::vector<InputEvent> left{
            {InputEventType::KeyDown, Key::Left}};
        runner.expect(advanceMapgenPlayback(playback, left, 0, 100) ==
                          MapgenPlaybackCommand::None,
                      "visual mapgen playback steps backward");
        runner.expect(currentMapgenFrame(playback)->label == "middle",
                      "visual mapgen playback exposes previous frame");

        const std::vector<InputEvent> enter{
            {InputEventType::KeyDown, Key::Enter}};
        runner.expect(advanceMapgenPlayback(playback, enter, 0, 100) ==
                          MapgenPlaybackCommand::Commit,
                      "visual mapgen playback commits on Enter");
        const std::vector<InputEvent> escape{
            {InputEventType::KeyDown, Key::Escape}};
        runner.expect(advanceMapgenPlayback(playback, escape, 0, 100) ==
                          MapgenPlaybackCommand::Cancel,
                      "visual mapgen playback cancels on Escape");
    }

    {
        Map map{5, 5, TileType::Wall};
        map.setTile(2, 2, TileType::Floor);
        ConsoleBuffer console{5, 5};
        console.clear();
        Camera camera{5, 5};
        camera.centerOn(Point{2, 2}, map);
        renderMapDebug(map, console, camera);
        runner.expect(console.cellAt(2, 2).glyph == U'.',
                      "visual mapgen debug renderer ignores reveal state");
    }

    {
        const auto catalog = mapBuilderCatalog();
        std::vector<std::string_view> catalogIds;
        catalogIds.reserve(catalog.size());
        for (const auto& descriptor : catalog) {
            catalogIds.push_back(descriptor.id);
        }
        auto uniqueIds = catalogIds;
        std::ranges::sort(uniqueIds);
        uniqueIds.erase(std::ranges::unique(uniqueIds).begin(),
                        uniqueIds.end());
        const auto available = availableMapBuilders();
        const bool catalogMatchesAvailable = std::equal(
            catalog.begin(),
            catalog.end(),
            available.begin(),
            available.end(),
            [](const MapBuilderDescriptor& descriptor, const std::string& id) {
                return descriptor.id == id;
            });
        runner.expect(catalog.size() == 41U && uniqueIds.size() == 41U &&
                          available.size() == catalog.size() &&
                          catalogMatchesAvailable,
                      "map builder catalog contains 41 unique builders");
        runner.expect(
            mapBuilderDescriptor("simple")->category ==
                    MapBuilderCategory::Algorithm &&
                mapBuilderDescriptor("cavern-vaults")->category ==
                    MapBuilderCategory::Composition &&
                mapBuilderDescriptor("surface-cave-mouth")->category ==
                    MapBuilderCategory::Surface &&
                mapBuilderDescriptor("dragon-lair")->category ==
                    MapBuilderCategory::DungeonRegion,
            "map builder catalog groups algorithms, chains, and regions");
    }

    {
        MapgenGalleryState gallery;
        MapgenGalleryOptions options;
        options.initialBuilderId = "simple";
        options.seed = Rng::Seed{123U};
        options.width = 40;
        options.height = 25;
        openMapgenGallery(gallery, options);

        runner.expect(gallery.seed == 123U && gallery.paused &&
                          !gallery.frames.empty() &&
                          gallery.frameIndex + 1U == gallery.frames.size() &&
                          gallery.error.empty(),
                      "gallery opens paused on the final generated map");
        runner.expect(selectedMapgenGalleryBuilder(gallery).id == "simple" &&
                          gallery.validation.valid,
                      "gallery exposes its selected validated builder");

        const std::vector<InputEvent> nextBuilder{
            {InputEventType::KeyDown, Key::Down}};
        static_cast<void>(updateMapgenGallery(gallery, nextBuilder, 0));
        runner.expect(selectedMapgenGalleryBuilder(gallery).id == "bsp" &&
                          gallery.seed == 123U,
                      "gallery switches builders while preserving the seed");

        const std::vector<InputEvent> play{
            {InputEventType::KeyDown, Key::Space}};
        static_cast<void>(updateMapgenGallery(gallery, play, 0));
        runner.expect(!gallery.paused && gallery.frameIndex == 0U,
                      "gallery restarts playback when playing from final");
        static_cast<void>(updateMapgenGallery(gallery, {}, 300));
        runner.expect(gallery.frameIndex > 0U || gallery.paused,
                      "gallery advances playback using elapsed time");

        const std::vector<InputEvent> finalFrame{
            {InputEventType::KeyDown, Key::Enter}};
        static_cast<void>(updateMapgenGallery(gallery, finalFrame, 0));
        runner.expect(gallery.paused &&
                          gallery.frameIndex + 1U == gallery.frames.size(),
                      "gallery Enter returns to the final frame");

        const std::vector<InputEvent> picker{{InputEventType::KeyDown, Key::B}};
        static_cast<void>(updateMapgenGallery(gallery, picker, 0));
        const std::vector<InputEvent> nextCategory{
            {InputEventType::KeyDown, Key::Right}};
        static_cast<void>(updateMapgenGallery(gallery, nextCategory, 0));
        runner.expect(gallery.pickerOpen &&
                          pickerMapgenGalleryBuilder(gallery).category ==
                              MapBuilderCategory::Composition,
                      "gallery picker navigates grouped builder categories");
        const std::vector<InputEvent> closePicker{
            {InputEventType::KeyDown, Key::Escape}};
        static_cast<void>(updateMapgenGallery(gallery, closePicker, 0));
        runner.expect(!gallery.pickerOpen &&
                          selectedMapgenGalleryBuilder(gallery).id == "bsp",
                      "closing the picker preserves the active preview");

        ConsoleBuffer console{80, 50};
        console.clear();
        drawMapgenGallery(console, gallery);
        runner.expect(bufferContains(console, "Gallery") &&
                          bufferContains(console, "bsp") &&
                          bufferContains(console, "seed 123") &&
                          bufferContains(console, "valid"),
                      "gallery renderer shows builder, seed, and validation");

        const auto oldSeed = gallery.seed;
        const std::vector<InputEvent> newSeed{
            {InputEventType::KeyDown, Key::N}};
        static_cast<void>(updateMapgenGallery(gallery, newSeed, 0));
        runner.expect(gallery.seed != oldSeed && gallery.error.empty(),
                      "gallery generates a new shared seed on N");

        const std::vector<InputEvent> escape{
            {InputEventType::KeyDown, Key::Escape}};
        runner.expect(updateMapgenGallery(gallery, escape, 0) ==
                          MapgenGalleryCommand::ReturnToTitle,
                      "gallery preview returns to the title menu");
    }

    {
        MapgenGalleryState gallery;
        MapgenGalleryOptions options;
        options.initialBuilderId = "simple";
        options.seed = Rng::Seed{321U};
        options.width = 5;
        options.height = 5;
        openMapgenGallery(gallery, options);
        runner.expect(!gallery.error.empty() && gallery.frames.empty(),
                      "gallery reports builder failures without throwing");
    }

    {
        Map map{9, 9, TileType::Wall};
        const auto both = paintBrush(map,
                                     Point{2, 3},
                                     TileType::Floor,
                                     BrushShape::Single,
                                     0,
                                     Symmetry::Both);
        runner.expect(both.size() == 3U,
                      "both symmetry follows axis mirroring");
        runner.expect(!map.isWalkable(6, 5),
                      "both symmetry does not add diagonal quad points");

        const auto quad = paintBrush(map,
                                     Point{2, 3},
                                     TileType::Floor,
                                     BrushShape::Single,
                                     0,
                                     Symmetry::Quad);
        runner.expect(quad.size() == 4U && map.isWalkable(6, 5),
                      "quad symmetry keeps the optional diagonal mirror mode");
    }

    {
        Rng rng{12345U};
        const auto result =
            SimpleMapBuilder::build(40, 25, rng, MapBuildOptions{true});
        runner.expect(result.builderName == "simple",
                      "simple builder reports its id");
        runner.expect(!result.snapshots.empty(),
                      "simple builder records snapshots when requested");
        runner.expect(!result.spawnRegions.empty(),
                      "simple builder returns spawn regions");
        const auto validation = validateMapBuild(
            result.map, result.playerStart, result.stairsPosition);
        runner.expect(validation.valid,
                      "simple builder output validates through pipeline");
    }

    {
        Rng rng{606U};
        const auto result =
            buildMapById("town", 40, 25, rng, MapBuildOptions{true});
        const auto validation = validateMapBuild(
            result.map, result.playerStart, result.stairsPosition);
        const auto hasSpawn = [&](std::string_view id) {
            return std::any_of(
                result.spawnRequests.begin(),
                result.spawnRequests.end(),
                [id](const auto& request) { return request.id == id; });
        };

        runner.expect(result.builderName == "town",
                      "town builder reports its id");
        runner.expect(validation.valid,
                      "town builder output validates through pipeline");
        runner.expect(
            hasSpawn("blacksmith") && hasSpawn("alchemist") &&
                hasSpawn("townsfolk") && hasSpawn("chair") &&
                hasSpawn("food_ration"),
            "town builder requests vendor, bystander, prop, and food spawns");
    }

    {
        Map map{7, 5, TileType::Floor};
        map.setTile(1, 2, TileType::Tree);
        map.setTile(3, 2, TileType::DenseReeds);
        map.setTile(5, 2, TileType::Rock);
        runner.expect(map.blocksMovement(1, 2) && map.blocksSight(1, 2) &&
                          map.blocksMovement(3, 2) && map.blocksSight(3, 2) &&
                          map.blocksMovement(5, 2) && map.blocksSight(5, 2),
                      "natural terrain blocks movement and sight");
        runner.expect(!isConnectivityPassable(TileType::Tree) &&
                          !isConnectivityPassable(TileType::DenseReeds) &&
                          !isConnectivityPassable(TileType::Rock),
                      "natural terrain is excluded from connectivity");
        runner.expect(isNaturalBlocker(TileType::Tree) &&
                          isNaturalBlocker(TileType::DenseReeds) &&
                          isNaturalBlocker(TileType::Rock) &&
                          !isNaturalBlocker(TileType::Wall),
                      "tile traits distinguish natural and constructed walls");
    }

    {
        struct SurfaceRoleContent {
            std::string id;
            BiomeVariant biome = BiomeVariant::DryWoods;
            LocalScreenRole role = LocalScreenRole::CornerWilderness;
            std::string firstSpawn;
            std::string secondSpawn;
            bool expectsDownStairs = false;
        };

        const std::vector<SurfaceRoleContent> roles{
            {"surface-town-outskirts",
             BiomeVariant::DryWoods,
             LocalScreenRole::TownOutskirts,
             "townsfolk",
             "food_ration",
             false},
            {"surface-cardinal-transition",
             BiomeVariant::ReedMarsh,
             LocalScreenRole::CardinalTransition,
             "wild_boar",
             "food_ration",
             false},
            {"surface-corner-wilderness",
             BiomeVariant::DeepGrove,
             LocalScreenRole::CornerWilderness,
             "wolf",
             "wild_boar",
             false},
            {"surface-ruin-pocket",
             BiomeVariant::RuinedWoodland,
             LocalScreenRole::RuinPocket,
             "goblin",
             "dwarf_rubble",
             false},
            {"surface-cave-mouth",
             BiomeVariant::LimestoneRise,
             LocalScreenRole::CaveMouth,
             "cave_bat",
             "limestone_column",
             true},
            {"surface-camp-shrine",
             BiomeVariant::WetWoods,
             LocalScreenRole::CampShrine,
             "chair",
             "healing_potion",
             false},
            {"surface-resource-heavy",
             BiomeVariant::LimestoneRise,
             LocalScreenRole::ResourceHeavy,
             "limestone_column",
             "healing_potion",
             false},
            {"surface-danger-encounter",
             BiomeVariant::DangerousWilds,
             LocalScreenRole::DangerEncounter,
             "orc",
             "fireball_scroll",
             false},
        };

        for (std::size_t index = 0; index < roles.size(); ++index) {
            Rng rng{static_cast<Rng::Seed>(7070U + index)};
            MapBuildOptions options;
            options.biomeVariant = roles[index].biome;
            options.screenRole = roles[index].role;
            options.exitMask = exitNorth | exitEast;
            options.dangerEncounter =
                roles[index].role == LocalScreenRole::DangerEncounter;

            const auto result =
                buildMapById(roles[index].id, 80, 43, rng, options);
            const auto validation = validateMapBuild(
                result.map, result.playerStart, result.stairsPosition);

            runner.expect(validation.valid,
                          roles[index].id + " surface role validates");
            runner.expect(result.builderName == roles[index].id,
                          roles[index].id + " reports its surface id");
            runner.expect(hasBuildStepContaining(result, "role:"),
                          roles[index].id + " records its local role");
            runner.expect(
                surfaceExitIsOpen(result.map, exitNorth) &&
                    surfaceExitIsOpen(result.map, exitEast) &&
                    surfaceExitIsThreeTilesWide(result.map, exitNorth) &&
                    surfaceExitIsThreeTilesWide(result.map, exitEast),
                roles[index].id + " preserves three-tile requested exits");
            runner.expect(hasSpawnRequest(result, roles[index].firstSpawn) &&
                              hasSpawnRequest(result, roles[index].secondSpawn),
                          roles[index].id +
                              " requests role-appropriate content");
            runner.expect((countTiles(result.map, TileType::DownStairs) > 0) ==
                              roles[index].expectsDownStairs,
                          roles[index].id +
                              " controls deliberate surface stairs");
        }

        MapBuildOptions marshResources;
        marshResources.biomeVariant = BiomeVariant::ReedMarsh;
        marshResources.screenRole = LocalScreenRole::ResourceHeavy;
        marshResources.exitMask = exitNorth | exitEast | exitSouth | exitWest;
        Rng marshResourceRng{8080U};
        const auto marshResourceResult = buildMapById(
            "surface-resource-heavy", 80, 43, marshResourceRng, marshResources);
        runner.expect(hasSpawnRequest(marshResourceResult, "marsh_root") &&
                          hasSpawnRequest(marshResourceResult, "reed_shoot"),
                      "surface resource screens request marsh resources");

        MapBuildOptions marshCamp;
        marshCamp.biomeVariant = BiomeVariant::ReedMarsh;
        marshCamp.screenRole = LocalScreenRole::CampShrine;
        marshCamp.exitMask = exitNorth | exitEast | exitSouth | exitWest;
        Rng marshCampRng{8081U};
        const auto marshCampResult = buildMapById("surface-camp-shrine",
                                                  80,
                                                  43,
                                                  marshCampRng,
                                                  marshCamp);
        runner.expect(hasSpawnRequest(marshCampResult, "reed_camp") &&
                          hasSpawnRequest(marshCampResult, "reed_mentor"),
                      "surface camp screens request marsh service content");
    }

    {
        struct RegionContent {
            std::string id;
            std::string buildStep;
            std::string firstSpawn;
            std::string secondSpawn;
        };
        const std::vector<RegionContent> regions{
            {"deep-caverns",
             "theme: deep caverns",
             "deep_kobold",
             "stone_golem"},
            {"dwarf-transition",
             "theme: cavern to dwarf fort transition",
             "dwarf_sentry",
             "dwarf_rubble"},
            {"dragon-lair",
             "content: dragon and dwarf fortress treasure",
             "dragon",
             "gauntlets_of_might"},
            {"mushroom-entrance",
             "theme: mushroom entrance",
             "mushroom_patch",
             "sporeling"},
            {"mushroom-forest",
             "content: fungal forest encounters",
             "sporeling",
             "fungal_giant"},
            {"mushroom-exit",
             "theme: mushroom exit toward dark elf city",
             "dark_elf_scout_a",
             "hand_crossbow"},
            {"dark-elf-city",
             "content: rival dark elf clans",
             "dark_elf_marksman_a",
             "dark_elf_blade_b"},
        };

        for (std::size_t index = 0; index < regions.size(); ++index) {
            Rng rng{static_cast<Rng::Seed>(700U + index)};
            const auto result = buildMapById(
                regions[index].id, 40, 25, rng, MapBuildOptions{true});
            const auto validation = validateMapBuild(
                result.map, result.playerStart, result.stairsPosition);
            runner.expect(validation.valid,
                          regions[index].id + " region validates");
            runner.expect(
                hasBuildStepContaining(result, regions[index].buildStep),
                regions[index].id + " records authored theme");
            runner.expect(
                hasSpawnRequest(result, regions[index].firstSpawn) &&
                    hasSpawnRequest(result, regions[index].secondSpawn),
                regions[index].id + " requests authored region content");
        }
    }

    {
        Rng rng{222U};
        const auto result =
            BspDungeonBuilder::build(40, 25, rng, MapBuildOptions{true});
        const auto validation = validateMapBuild(
            result.map, result.playerStart, result.stairsPosition);
        runner.expect(result.builderName == "bsp",
                      "bsp builder reports its id");
        runner.expect(validation.valid, "bsp builder produces reachable maps");
        runner.expect(!result.snapshots.empty(),
                      "bsp builder records snapshots when requested");
        runner.expect(!result.corridorPaths.empty(),
                      "bsp builder records corridor paths for modifiers");
        runner.expect(hasBuildStepContaining(result, "random subrects"),
                      "bsp builder records subrect algorithm");
    }

    {
        Rng rng{333U};
        const auto result =
            BspInteriorBuilder::build(40, 25, rng, MapBuildOptions{true});
        const auto validation = validateMapBuild(
            result.map, result.playerStart, result.stairsPosition);
        runner.expect(result.builderName == "bsp-interior",
                      "bsp interior builder reports its id");
        runner.expect(validation.valid,
                      "bsp interior builder keeps maps reachable");
        runner.expect(!result.corridorPaths.empty(),
                      "bsp interior builder records corridor paths");
        runner.expect(hasBuildStepContaining(result, "leaf rooms"),
                      "bsp interior builder records leaf-room algorithm");
    }

    {
        auto pattern = wallChunkPattern();
        setChunkTile(pattern, 1, 2, TileType::Floor);

        const auto chunks = wfc::extractChunks({sourceFromPattern(pattern)});
        runner.expect(chunks.size() == 4U,
                      "wfc chunk extraction adds flips and dedupes patterns");
        runner.expect(!chunkCollectionHasFloorAt(chunks, 5, 1),
                      "wfc chunk extraction does not rotate chunks");
    }

    {
        auto pattern = wallChunkPattern();
        setChunkTile(pattern, 3, 0, TileType::Floor);
        setChunkTile(pattern, 7, 4, TileType::Floor);
        setChunkTile(pattern, 4, 7, TileType::Floor);

        const auto chunk = wfc::makeChunk(pattern);
        runner.expect(chunk.hasExits, "wfc chunks detect any edge exit");
        runner.expect(chunk.exits[static_cast<std::size_t>(
                          wfc::directionIndex(wfc::Direction::North))][3],
                      "wfc chunks detect north exits");
        runner.expect(chunk.exits[static_cast<std::size_t>(
                          wfc::directionIndex(wfc::Direction::East))][4],
                      "wfc chunks detect east exits");
        runner.expect(!chunk.exits[static_cast<std::size_t>(
                          wfc::directionIndex(wfc::Direction::West))][3],
                      "wfc chunks leave closed edges closed");
    }

    {
        auto east = wallChunkPattern();
        auto west = wallChunkPattern();
        auto closed = wallChunkPattern();
        setChunkTile(east, 7, 3, TileType::Floor);
        setChunkTile(west, 0, 3, TileType::Floor);

        std::vector<wfc::MapChunk> chunks{
            wfc::makeChunk(east),
            wfc::makeChunk(west),
            wfc::makeChunk(closed),
        };
        wfc::buildCompatibility(chunks);

        runner.expect(
            compatibleListContains(chunks[0], wfc::Direction::East, 1),
            "wfc compatibility matches open edge exits");
        runner.expect(
            !compatibleListContains(chunks[0], wfc::Direction::East, 2),
            "wfc compatibility rejects open-to-closed edges");
        runner.expect(
            compatibleListContains(chunks[0], wfc::Direction::North, 2),
            "wfc compatibility allows closed-to-closed edges");
    }

    {
        Rng firstRng{123U};
        Rng secondRng{123U};
        const auto first =
            WfcBuilder::build(40, 25, firstRng, MapBuildOptions{true});
        const auto second =
            WfcBuilder::build(40, 25, secondRng, MapBuildOptions{true});
        const auto validation = validateMapBuild(
            first.map, first.playerStart, first.stairsPosition);

        runner.expect(first.builderName == "wfc", "wfc builder reports its id");
        runner.expect(sameMapTiles(first.map, second.map) &&
                          first.playerStart == second.playerStart &&
                          first.stairsPosition == second.stairsPosition,
                      "wfc builder is deterministic for fixed seeds");
        runner.expect(validation.valid,
                      "wfc builder produces reachable start and stairs");
        runner.expect(
            hasSnapshotContaining(first, "wfc source") &&
                hasSnapshotContaining(first, "chunk gallery") &&
                hasSnapshotContaining(first, "placement"),
            "wfc builder records source chunk and placement snapshots");
        runner.expect(hasBuildStepContaining(first, "neighbor-count"),
                      "wfc builder records solver mode");
    }

    {
        Rng openAreaRng{81U};
        const auto openArea = buildMapById(
            "drunkard-open-area", 40, 25, openAreaRng, MapBuildOptions{true});
        runner.expect(openArea.builderName == "drunkard-open-area",
                      "drunkard open-area variant reports its id");
        runner.expect(hasBuildStepContaining(openArea, "starting-point") &&
                          hasBuildStepContaining(openArea, "lifetime 400"),
                      "drunkard open-area uses starting-point settings");

        Rng fatPassagesRng{82U};
        const auto fatPassages = buildMapById("drunkard-fat-passages",
                                              40,
                                              25,
                                              fatPassagesRng,
                                              MapBuildOptions{true});
        runner.expect(fatPassages.builderName == "drunkard-fat-passages",
                      "drunkard fat-passages variant reports its id");
        runner.expect(hasBuildStepContaining(fatPassages, "brush 2"),
                      "drunkard fat-passages uses a wider brush");
    }

    {
        Rng inwardsRng{91U};
        const auto inwards = buildMapById(
            "dla-walk-inwards", 40, 25, inwardsRng, MapBuildOptions{true});
        runner.expect(inwards.builderName == "dla-walk-inwards",
                      "DLA walk-inwards variant reports its id");
        runner.expect(hasBuildStepContaining(inwards, "walk-inwards"),
                      "DLA records walk-inwards settings");

        Rng attractorRng{92U};
        const auto attractor = buildMapById("dla-central-attractor",
                                            40,
                                            25,
                                            attractorRng,
                                            MapBuildOptions{true});
        runner.expect(attractor.builderName == "dla-central-attractor",
                      "DLA central-attractor variant reports its id");
        runner.expect(hasBuildStepContaining(attractor, "central-attractor"),
                      "DLA records central-attractor settings");
    }

    {
        Rng pythagorasRng{101U};
        Rng manhattanRng{101U};
        const auto pythagoras = buildMapById(
            "voronoi-pythagoras", 40, 25, pythagorasRng, MapBuildOptions{true});
        const auto manhattan = buildMapById(
            "voronoi-manhattan", 40, 25, manhattanRng, MapBuildOptions{true});
        runner.expect(pythagoras.builderName == "voronoi-pythagoras" &&
                          manhattan.builderName == "voronoi-manhattan",
                      "Voronoi distance variants report their ids");
        runner.expect(hasBuildStepContaining(pythagoras, "pythagoras") &&
                          hasBuildStepContaining(manhattan, "manhattan"),
                      "Voronoi records selected distance settings");
        runner.expect(!sameMapTiles(pythagoras.map, manhattan.map),
                      "Voronoi distance variants produce distinct maps");

        Rng cleanupRng{4242U};
        const auto cleaned =
            buildMapById("voronoi", 80, 43, cleanupRng, MapBuildOptions{true});
        runner.expect(countSmallInteriorWallComponents(cleaned.map, 8U) == 0,
                      "Voronoi output removes tiny floating wall fragments");
    }

    {
        MapBuildResult result{Map{12, 12, TileType::Floor}, {}, {}};
        result.playerStart = Point{1, 1};
        result.stairsPosition = Point{10, 10};
        Rng rng{5U};

        const auto placed = placePrefab(result,
                                        shrinePrefab(),
                                        rng,
                                        MapBuildOptions{true},
                                        PrefabPlacementMode::Center);
        runner.expect(placed.placed, "prefab center placement succeeds");
        runner.expect(countTiles(result.map, TileType::ClosedDoor) == 1,
                      "prefab placement applies door markers");
        runner.expect(result.spawnRequests.size() == 2U,
                      "prefab placement converts spawn markers to requests");
        runner.expect(hasBuildStepContaining(result, "placed prefab shrine"),
                      "prefab placement records a build step");

        MapBuildResult tiny{Map{5, 5, TileType::Floor}, {}, {}};
        tiny.playerStart = Point{1, 1};
        tiny.stairsPosition = Point{3, 3};
        const auto failed =
            placePrefab(tiny, armoryVaultPrefab(), rng, MapBuildOptions{});
        runner.expect(!failed.placed &&
                          hasBuildStepContaining(tiny, "skipped prefab"),
                      "prefab placement reports failed placement diagnostics");

        MapBuildResult sectionResult{Map{12, 12, TileType::Floor}, {}, {}};
        sectionResult.playerStart = Point{1, 1};
        sectionResult.stairsPosition = Point{10, 10};
        sectionResult.spawnRegions.push_back(spawnRegionFromTiles(
            {Point{5, 5}, Point{6, 6}}, SpawnRegionKind::Room));
        const auto section =
            placeSection(sectionResult,
                         PrefabSection{prefabFromAscii("section",
                                                       {
                                                           "###",
                                                           "#M#",
                                                           "#+#",
                                                       }),
                                       PrefabHorizontalPlacement::Center,
                                       PrefabVerticalPlacement::Center},
                         MapBuildOptions{true});
        runner.expect(section.placed,
                      "prefab section placement succeeds at fixed anchors");
        runner.expect(sectionResult.spawnRequests.size() == 1U,
                      "prefab section converts markers to spawn requests");
        runner.expect(sectionResult.spawnRegions.size() == 1U,
                      "prefab section replaces overlapping spawn regions");

        const auto earlyVaults = vaultsForDepth(1);
        const auto laterVaults = vaultsForDepth(3);
        runner.expect(!earlyVaults.empty() &&
                          laterVaults.size() > earlyVaults.size(),
                      "prefab vault library filters by dungeon depth");
    }

    {
        runner.expect(
            isKnownMapBuilder("simple") && isKnownMapBuilder("bsp") &&
                isKnownMapBuilder("bsp-interior") &&
                isKnownMapBuilder("cellular") &&
                isKnownMapBuilder("drunkard") &&
                isKnownMapBuilder("drunkard-open-area") &&
                isKnownMapBuilder("drunkard-open-halls") &&
                isKnownMapBuilder("drunkard-winding-passages") &&
                isKnownMapBuilder("drunkard-fat-passages") &&
                isKnownMapBuilder("drunkard-symmetry") &&
                isKnownMapBuilder("maze") && isKnownMapBuilder("dla") &&
                isKnownMapBuilder("dla-walk-inwards") &&
                isKnownMapBuilder("dla-walk-outwards") &&
                isKnownMapBuilder("dla-central-attractor") &&
                isKnownMapBuilder("dla-insectoid") &&
                isKnownMapBuilder("voronoi") &&
                isKnownMapBuilder("voronoi-pythagoras") &&
                isKnownMapBuilder("voronoi-manhattan") &&
                isKnownMapBuilder("voronoi-chebyshev") &&
                isKnownMapBuilder("wfc") && isKnownMapBuilder("town") &&
                isKnownMapBuilder("woods") &&
                isKnownMapBuilder("surface-town-outskirts") &&
                isKnownMapBuilder("surface-cardinal-transition") &&
                isKnownMapBuilder("surface-corner-wilderness") &&
                isKnownMapBuilder("surface-ruin-pocket") &&
                isKnownMapBuilder("surface-cave-mouth") &&
                isKnownMapBuilder("surface-camp-shrine") &&
                isKnownMapBuilder("surface-resource-heavy") &&
                isKnownMapBuilder("surface-danger-encounter") &&
                isKnownMapBuilder("limestone-caverns") &&
                isKnownMapBuilder("deep-caverns") &&
                isKnownMapBuilder("dwarf-transition") &&
                isKnownMapBuilder("dragon-lair") &&
                isKnownMapBuilder("mushroom-entrance") &&
                isKnownMapBuilder("mushroom-forest") &&
                isKnownMapBuilder("mushroom-exit") &&
                isKnownMapBuilder("dark-elf-city") &&
                isKnownMapBuilder("cavern-vaults") &&
                isKnownMapBuilder("ruined-rooms"),
            "mapgen viewer exposes all implemented builders");
        std::ostringstream output;
        const int result = runMapgenViewer(
            MapgenViewerOptions{"bsp", Rng::Seed{777U}, false, 40, 25}, output);
        const std::string text = output.str();
        runner.expect(result == 0, "mapgen viewer runs selected builder");
        runner.expect(text.find("Builder: bsp") != std::string::npos,
                      "mapgen viewer prints builder id");
        runner.expect(text.find("Validation: ok") != std::string::npos,
                      "mapgen viewer prints validation result");
        runner.expect(text.find("Quality: coverage") != std::string::npos,
                      "mapgen viewer prints structural quality metrics");

        MapgenViewerOptions biomeOptions{
            "surface-corner-wilderness", Rng::Seed{888U}, false, 40, 25};
        biomeOptions.biomeVariant = BiomeVariant::DryWoods;
        std::ostringstream dryWoodsOutput;
        std::ostringstream repeatedDryWoodsOutput;
        static_cast<void>(runMapgenViewer(biomeOptions, dryWoodsOutput));
        static_cast<void>(
            runMapgenViewer(biomeOptions, repeatedDryWoodsOutput));

        biomeOptions.biomeVariant = BiomeVariant::ReedMarsh;
        std::ostringstream reedMarshOutput;
        static_cast<void>(runMapgenViewer(biomeOptions, reedMarshOutput));

        runner.expect(dryWoodsOutput.str() == repeatedDryWoodsOutput.str(),
                      "mapgen viewer remains deterministic for a fixed biome");
        runner.expect(dryWoodsOutput.str() != reedMarshOutput.str(),
                      "mapgen viewer applies the selected surface biome");
    }

    {
        const std::vector<std::string> ids{"simple",
                                           "bsp",
                                           "bsp-interior",
                                           "cellular",
                                           "maze",
                                           "drunkard",
                                           "dla",
                                           "voronoi",
                                           "wfc",
                                           "town",
                                           "woods",
                                           "surface-town-outskirts",
                                           "surface-cardinal-transition",
                                           "surface-corner-wilderness",
                                           "surface-ruin-pocket",
                                           "surface-cave-mouth",
                                           "surface-camp-shrine",
                                           "surface-resource-heavy",
                                           "surface-danger-encounter",
                                           "limestone-caverns",
                                           "deep-caverns",
                                           "dwarf-transition",
                                           "dragon-lair",
                                           "mushroom-entrance",
                                           "mushroom-forest",
                                           "mushroom-exit",
                                           "dark-elf-city",
                                           "cavern-vaults",
                                           "ruined-rooms"};
        const std::array<Rng::Seed, 2> seeds{1U, 4242U};
        for (const auto& id : ids) {
            for (const Rng::Seed seed : seeds) {
                Rng firstRng{seed};
                Rng secondRng{seed};
                const auto first =
                    buildMapById(id, 80, 43, firstRng, MapBuildOptions{false});
                const auto second =
                    buildMapById(id, 80, 43, secondRng, MapBuildOptions{false});
                runner.expect(sameMapTiles(first.map, second.map) &&
                                  first.playerStart == second.playerStart &&
                                  first.stairsPosition == second.stairsPosition,
                              id + " matches exact same-seed output for " +
                                  std::to_string(seed));
            }
        }
    }

    {
        for (const auto& id : availableMapBuilders()) {
            Rng rng{4242U};
            const auto result =
                buildMapById(id, 40, 25, rng, MapBuildOptions{true});
            const auto validation = validateMapBuild(
                result.map, result.playerStart, result.stairsPosition);
            runner.expect(validation.valid, id + " builder validates");
            runner.expect(result.builderName == id, id + " reports its id");
            runner.expect(!result.snapshots.empty(),
                          id + " records snapshots when requested");
            runner.expect(!result.spawnRegions.empty(),
                          id + " returns generic spawn regions");
        }
    }

    {
        const std::vector<std::string> ids{"simple", "ruined-rooms"};
        for (const auto& id : ids) {
            for (Rng::Seed seed = 1U; seed <= 5U; ++seed) {
                Rng rng{seed};
                const auto result =
                    buildMapById(id, 80, 43, rng, MapBuildOptions{false});
                const auto validation = validateMapBuild(
                    result.map, result.playerStart, result.stairsPosition);
                runner.expect(validation.valid,
                              id + " validates for regression seed " +
                                  std::to_string(seed));
            }
        }
    }

    {
        const std::array<Rng::Seed, 5> seeds{1U, 2U, 7U, 42U, 4242U};
        for (const Rng::Seed seed : seeds) {
            Rng rng{seed};
            const auto result =
                buildMapById("cellular", 80, 43, rng, MapBuildOptions{});
            const auto metrics = measureMapQuality(result.map);
            runner.expect(metrics.walkableCoverage >= 0.28 &&
                              metrics.walkableCoverage <= 0.50,
                          "cellular coverage stays cave-like for seed " +
                              std::to_string(seed));
            runner.expect(metrics.disconnectedRegions == 0 &&
                              metrics.tinyBlockerComponents == 0,
                          "cellular removes disconnected floors and rock "
                          "specks for seed " +
                              std::to_string(seed));
        }
    }
    {
        struct Target {
            std::string id;
            double coverage = 0.0;
            double tolerance = 0.0;
        };
        const std::vector<Target> targets{
            {"drunkard", 0.40, 0.03},
            {"drunkard-open-area", 0.50, 0.03},
            {"drunkard-open-halls", 0.50, 0.03},
            {"drunkard-winding-passages", 0.40, 0.03},
            {"drunkard-fat-passages", 0.40, 0.03},
            {"drunkard-symmetry", 0.40, 0.03},
            {"dla", 0.25, 0.02},
            {"dla-walk-inwards", 0.25, 0.02},
            {"dla-walk-outwards", 0.25, 0.02},
            {"dla-central-attractor", 0.25, 0.02},
            {"dla-insectoid", 0.25, 0.02},
        };
        const std::array<Rng::Seed, 3> seeds{1U, 42U, 4242U};
        for (const auto& target : targets) {
            for (const Rng::Seed seed : seeds) {
                Rng rng{seed};
                const auto result =
                    buildMapById(target.id, 80, 43, rng, MapBuildOptions{});
                const auto metrics = measureMapQuality(result.map);
                runner.expect(
                    std::abs(metrics.walkableCoverage - target.coverage) <=
                        target.tolerance,
                    target.id + " reaches retained floor target for seed " +
                        std::to_string(seed));
                runner.expect(metrics.disconnectedRegions == 0,
                              target.id +
                                  " keeps random growth connected for seed " +
                                  std::to_string(seed));
            }
        }
    }
    {
        const std::array<Rng::Seed, 3> seeds{1U, 42U, 4242U};
        for (const Rng::Seed seed : seeds) {
            Rng mazeRng{seed};
            const auto maze =
                buildMapById("maze", 80, 43, mazeRng, MapBuildOptions{});
            const auto mazeMetrics = measureMapQuality(maze.map);
            runner.expect(mazeMetrics.walkableCoverage >= 0.44 &&
                              mazeMetrics.walkableCoverage <= 0.50 &&
                              mazeMetrics.narrowPercent >= 0.85 &&
                              mazeMetrics.disconnectedRegions == 0,
                          "maze preserves perfect narrow identity for seed " +
                              std::to_string(seed));

            for (const std::string id : {"simple", "bsp", "bsp-interior"}) {
                Rng roomRng{seed};
                const auto result =
                    buildMapById(id, 80, 43, roomRng, MapBuildOptions{});
                const auto metrics = measureMapQuality(result.map);
                runner.expect(metrics.walkableCoverage >= 0.20 &&
                                  metrics.walkableCoverage <= 0.65 &&
                                  metrics.disconnectedRegions == 0,
                              id +
                                  " stays within room-family bounds for seed " +
                                  std::to_string(seed));
            }

            Rng voronoiRng{seed};
            const auto voronoi =
                buildMapById("voronoi", 80, 43, voronoiRng, MapBuildOptions{});
            const auto voronoiMetrics = measureMapQuality(voronoi.map);
            runner.expect(voronoiMetrics.walkableCoverage >= 0.55 &&
                              voronoiMetrics.walkableCoverage <= 0.75 &&
                              voronoiMetrics.disconnectedRegions == 0 &&
                              voronoiMetrics.tinyBlockerComponents == 0,
                          "Voronoi keeps readable connected cells for seed " +
                              std::to_string(seed));
        }
    }

    {
        struct SurfaceTarget {
            std::string id;
            BiomeVariant biome = BiomeVariant::DryWoods;
        };
        const std::vector<SurfaceTarget> targets{
            {"surface-cardinal-transition", BiomeVariant::DryWoods},
            {"surface-corner-wilderness", BiomeVariant::DeepGrove},
            {"surface-ruin-pocket", BiomeVariant::RuinedWoodland},
            {"surface-cave-mouth", BiomeVariant::LimestoneRise},
            {"surface-camp-shrine", BiomeVariant::WetWoods},
            {"surface-resource-heavy", BiomeVariant::LimestoneRise},
        };
        const std::array<Rng::Seed, 3> seeds{1U, 42U, 4242U};
        for (const auto& target : targets) {
            for (const Rng::Seed seed : seeds) {
                MapBuildOptions options;
                options.biomeVariant = target.biome;
                options.exitMask = exitNorth | exitEast | exitSouth | exitWest;
                Rng rng{seed};
                const auto result =
                    buildMapById(target.id, 80, 43, rng, options);
                const auto metrics = measureMapQuality(result.map);
                runner.expect(metrics.walkableCoverage >= 0.50 &&
                                  metrics.walkableCoverage <= 0.68 &&
                                  metrics.narrowPercent <= 0.18 &&
                                  metrics.disconnectedRegions == 0 &&
                                  metrics.tinyBlockerComponents == 0,
                              target.id +
                                  " meets surface quality targets for seed " +
                                  std::to_string(seed));
            }
        }

        for (const Rng::Seed seed : seeds) {
            MapBuildOptions options;
            options.biomeVariant = BiomeVariant::DryWoods;
            options.exitMask = exitNorth | exitEast | exitSouth | exitWest;
            Rng townRng{seed};
            const auto town = buildMapById(
                "surface-town-outskirts", 80, 43, townRng, options);
            const auto townMetrics = measureMapQuality(town.map);
            runner.expect(townMetrics.walkableCoverage >= 0.65 &&
                              townMetrics.walkableCoverage <= 0.82,
                          "town outskirts preserves broad streets for seed " +
                              std::to_string(seed));

            options.biomeVariant = BiomeVariant::DangerousWilds;
            options.dangerEncounter = true;
            Rng dangerRng{seed};
            const auto danger = buildMapById(
                "surface-danger-encounter", 80, 43, dangerRng, options);
            const auto dangerMetrics = measureMapQuality(danger.map);
            runner.expect(dangerMetrics.narrowPercent <= 0.28 &&
                              dangerMetrics.disconnectedRegions == 0 &&
                              dangerMetrics.tinyBlockerComponents == 0,
                          "danger surface keeps a traversable arena for seed " +
                              std::to_string(seed));
        }
    }

    {
        struct OpennessLimit {
            std::string id;
            double maxWalkablePercent = 0.0;
            double maxOpenNeighborhoodPercent = 0.0;
        };
        const std::vector<OpennessLimit> limits{
            {"cellular", 72.0, 82.0},
            {"limestone-caverns", 72.0, 82.0},
            {"cavern-vaults", 75.0, 84.0},
            {"woods", 58.0, 70.0},
            {"surface-cardinal-transition", 92.0, 92.0},
            {"surface-corner-wilderness", 88.0, 90.0},
            {"surface-resource-heavy", 88.0, 90.0},
            {"surface-danger-encounter", 54.0, 70.0},
            {"mushroom-entrance", 62.0, 76.0},
            {"mushroom-forest", 62.0, 76.0},
            {"mushroom-exit", 62.0, 76.0},
            {"drunkard", 48.0, 66.0},
            {"dla", 34.0, 36.0},
            {"voronoi", 78.0, 68.0},
        };

        for (const auto& limit : limits) {
            Rng rng{4242U};
            const auto result =
                buildMapById(limit.id, 80, 43, rng, MapBuildOptions{false});
            const auto metrics = measureOpenness(result.map);
            runner.expect(metrics.walkablePercent <= limit.maxWalkablePercent,
                          limit.id + " keeps floor coverage bounded");
            runner.expect(metrics.openNeighborhoodPercent <=
                              limit.maxOpenNeighborhoodPercent,
                          limit.id + " keeps broad-open areas bounded");
        }
    }

    {
        MapBuildResult result{Map{11, 7, TileType::Wall}, {}, {}};
        result.playerStart = Point{1, 1};
        result.stairsPosition = Point{9, 5};
        carveVerticalDoorCandidate(result.map, 3, 3);
        carveVerticalDoorCandidate(result.map, 7, 3);
        result.corridorPaths.push_back(
            CorridorPath{"test", {Point{3, 3}, Point{3, 2}, Point{3, 4}}});

        Rng rng{7U};
        placeDoors(result, rng, MapBuildOptions{false});
        runner.expect(result.map.tileAt(3, 3) == TileType::ClosedDoor,
                      "door placement uses corridor candidates first");
        runner.expect(result.map.tileAt(7, 3) == TileType::Floor,
                      "corridor door placement avoids whole-map scanning");
    }

    {
        MapBuildResult result{Map{17, 7, TileType::Wall}, {}, {}};
        result.playerStart = Point{1, 1};
        result.stairsPosition = Point{15, 5};
        std::vector<Point> candidates;
        for (int x = 3; x <= 13; x += 2) {
            const Point candidate{x, 3};
            candidates.push_back(candidate);
            carveVerticalDoorCandidate(result.map, candidate.x, candidate.y);
        }
        result.spawnRequests.push_back(SpawnRequest{"goblin", candidates[0]});

        Rng rng{11U};
        placeDoors(result, rng, MapBuildOptions{false});
        runner.expect(result.map.tileAt(candidates[0].x, candidates[0].y) ==
                          TileType::Floor,
                      "door placement skips occupied spawn-request tiles");
        runner.expect(countTiles(result.map, TileType::ClosedDoor) <
                          static_cast<int>(candidates.size()) - 1,
                      "fallback door scan is sparse instead of exhaustive");
    }
}

} // namespace rl::test
