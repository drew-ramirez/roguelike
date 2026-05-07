#include "TestSuites.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/Rng.hpp"
#include "debug/MapgenPlayback.hpp"
#include "debug/MapgenViewer.hpp"
#include "map/MapBrush.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/MapConnectivity.hpp"
#include "map/MapValidation.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"
#include "map/generators/DrunkardsWalkBuilder.hpp"
#include "map/generators/DlaBuilder.hpp"
#include "map/generators/VoronoiBuilder.hpp"
#include "map/generators/WfcBuilder.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/BuilderChain.hpp"
#include "map/generators/SimpleMapBuilder.hpp"
#include "map/prefabs/Prefab.hpp"
#include "render/Camera.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/MapRenderSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool hasBuildStepContaining(const MapBuildResult& result,
                                          std::string_view text)
{
    return std::any_of(result.buildSteps.begin(),
                       result.buildSteps.end(),
                       [text](const auto& step) {
                           return step.find(text) != std::string::npos;
                       });
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
[[nodiscard]] bool hasSnapshotContaining(const MapBuildResult& result,
                                         std::string_view text)
{
    return std::any_of(result.snapshots.begin(),
                       result.snapshots.end(),
                       [text](const auto& s) {
                           return s.label.find(text) != std::string::npos;
                       });
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
} // namespace

void runMapgenTests(TestRunner& runner)
{
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
        Rng cleanupRng{4242U};
        const auto cleaned = VoronoiBuilder::buildWithSettings(
            80,
            43,
            cleanupRng,
            VoronoiSettings{64, VoronoiDistance::Pythagoras},
            MapBuildOptions{true});
        runner.expect(countSmallInteriorWallComponents(cleaned.map, 8U) == 0,
                      "Voronoi output removes tiny floating wall fragments");
    }
    {
        Rng inwardsRng{91U};
        const auto inwards = DlaBuilder::buildWithSettings(
            40,
            25,
            inwardsRng,
            DlaBuilder::walkInwardsSettings(),
            MapBuildOptions{true});
        const auto inwardsValidation = validateMapBuild(
            inwards.map, inwards.playerStart, inwards.stairsPosition);
        runner.expect(inwardsValidation.valid,
                      "DLA walk-inwards settings validate");
        runner.expect(hasBuildStepContaining(inwards, "walk-inwards"),
                      "DLA records walk-inwards settings");

        Rng attractorRng{92U};
        const auto attractor = DlaBuilder::buildWithSettings(
            40,
            25,
            attractorRng,
            DlaBuilder::centralAttractorSettings(),
            MapBuildOptions{true});
        const auto attractorValidation = validateMapBuild(
            attractor.map, attractor.playerStart, attractor.stairsPosition);
        runner.expect(attractorValidation.valid,
                      "DLA central-attractor settings validate");
        runner.expect(hasBuildStepContaining(attractor, "central-attractor"),
                      "DLA records central-attractor settings");

        runner.expect(DlaBuilder::walkOutwardsSettings().algorithm ==
                          DlaAlgorithm::WalkOutwards,
                      "DLA exposes walk-outwards settings");
        runner.expect(DlaBuilder::insectoidSettings().symmetry ==
                          Symmetry::Horizontal,
                      "DLA exposes insectoid symmetry settings");
    }

    {
        Rng pythagorasRng{101U};
        Rng manhattanRng{101U};
        const auto pythagoras = VoronoiBuilder::buildWithSettings(
            40,
            25,
            pythagorasRng,
            VoronoiSettings{64, VoronoiDistance::Pythagoras},
            MapBuildOptions{true});
        const auto manhattan = VoronoiBuilder::buildWithSettings(
            40,
            25,
            manhattanRng,
            VoronoiSettings{64, VoronoiDistance::Manhattan},
            MapBuildOptions{true});
        const auto pythagorasValidation = validateMapBuild(
            pythagoras.map, pythagoras.playerStart, pythagoras.stairsPosition);
        const auto manhattanValidation = validateMapBuild(
            manhattan.map, manhattan.playerStart, manhattan.stairsPosition);
        runner.expect(pythagorasValidation.valid && manhattanValidation.valid,
                      "Voronoi distance settings validate");
        runner.expect(hasBuildStepContaining(pythagoras, "pythagoras") &&
                          hasBuildStepContaining(manhattan, "manhattan"),
                      "Voronoi records selected distance settings");
        runner.expect(!sameMapTiles(pythagoras.map, manhattan.map),
                      "Voronoi distance settings produce distinct maps");
    }
    {
        Map map{8, 8, TileType::Wall};
        const auto painted = paintBrushSize(map,
                                            Point{4, 4},
                                            TileType::Floor,
                                            2,
                                            Symmetry::None);
        runner.expect(painted.size() == 4U && map.isWalkable(3, 3) &&
                          map.isWalkable(4, 4),
                      "brush sizing paints square stamps");
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
        Rng rng{2468U};
        const auto result =
            CellularAutomataBuilder::build(40, 25, rng, MapBuildOptions{true});
        const auto validation = validateMapBuild(
            result.map, result.playerStart, result.stairsPosition);
        runner.expect(result.builderName == "cellular",
                      "cellular builder reports its id");
        runner.expect(validation.valid, "cellular builder validates");
        runner.expect(!result.snapshots.empty(),
                      "cellular builder records snapshots when requested");
    }

    {
        Rng openAreaRng{81U};
        const auto openArea = DrunkardsWalkBuilder::buildWithSettings(
            40,
            25,
            openAreaRng,
            DrunkardsWalkBuilder::openAreaSettings(),
            MapBuildOptions{true});
        const auto openAreaValidation = validateMapBuild(
            openArea.map, openArea.playerStart, openArea.stairsPosition);
        runner.expect(openAreaValidation.valid,
                      "drunkard open-area settings validate");
        runner.expect(hasBuildStepContaining(openArea, "starting-point") &&
                          hasBuildStepContaining(openArea, "lifetime 400"),
                      "drunkard open-area uses starting-point settings");

        Rng fatPassagesRng{82U};
        const auto fatPassages = DrunkardsWalkBuilder::buildWithSettings(
            40,
            25,
            fatPassagesRng,
            DrunkardsWalkBuilder::fatPassagesSettings(),
            MapBuildOptions{true});
        const auto fatPassagesValidation = validateMapBuild(
            fatPassages.map,
            fatPassages.playerStart,
            fatPassages.stairsPosition);
        runner.expect(fatPassagesValidation.valid,
                      "drunkard fat-passages settings validate");
        runner.expect(hasBuildStepContaining(fatPassages, "brush 2"),
                      "drunkard fat-passages uses a wider brush");
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
        runner.expect(isKnownMapBuilder("simple") && isKnownMapBuilder("bsp") &&
                          isKnownMapBuilder("bsp-interior") &&
                          isKnownMapBuilder("cellular") &&
                          isKnownMapBuilder("drunkard") &&
                          isKnownMapBuilder("drunkard-open-area") &&
                          isKnownMapBuilder("drunkard-open-halls") &&
                          isKnownMapBuilder("drunkard-winding-passages") &&
                          isKnownMapBuilder("drunkard-fat-passages") &&
                          isKnownMapBuilder("drunkard-symmetry") &&
                          isKnownMapBuilder("maze") &&
                          isKnownMapBuilder("dla") &&
                          isKnownMapBuilder("dla-walk-inwards") &&
                          isKnownMapBuilder("dla-walk-outwards") &&
                          isKnownMapBuilder("dla-central-attractor") &&
                          isKnownMapBuilder("dla-insectoid") &&
                          isKnownMapBuilder("voronoi") &&
                          isKnownMapBuilder("voronoi-pythagoras") &&
                          isKnownMapBuilder("voronoi-manhattan") &&
                          isKnownMapBuilder("voronoi-chebyshev") &&
                          isKnownMapBuilder("wfc") &&
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
