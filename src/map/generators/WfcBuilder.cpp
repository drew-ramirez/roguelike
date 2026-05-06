/**
 * **************************************************************************
 * @file     WfcBuilder.cpp
 * @brief    Builds maps from REX Paint chunks with WFC
 * **************************************************************************
 */

#include "map/generators/WfcBuilder.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "assets/RexPaintLoader.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/MapConnectivity.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

constexpr int restartAttempts = 50;
constexpr std::array<wfc::Direction, 4> directions{
    wfc::Direction::North,
    wfc::Direction::East,
    wfc::Direction::South,
    wfc::Direction::West,
};

struct PlacedChunk {
    int chunk = -1;
};

struct SolverSnapshot {
    int placementCount = 0;
    std::vector<PlacedChunk> placements;
};

struct SolverResult {
    std::vector<PlacedChunk> placements;
    std::vector<SolverSnapshot> snapshots;
};

[[nodiscard]] int chunkIndex(int width, int x, int y)
{
    return y * width + x;
}

[[nodiscard]] Point deltaFor(wfc::Direction direction) noexcept
{
    switch (direction) {
    case wfc::Direction::North:
        return Point{0, -1};
    case wfc::Direction::East:
        return Point{1, 0};
    case wfc::Direction::South:
        return Point{0, 1};
    case wfc::Direction::West:
        return Point{-1, 0};
    }
    return {};
}

[[nodiscard]] bool isPlaced(const PlacedChunk& placed) noexcept
{
    return placed.chunk >= 0;
}

[[nodiscard]] bool containsChunk(const std::vector<int>& chunks, int chunk)
{
    return std::find(chunks.begin(), chunks.end(), chunk) != chunks.end();
}

[[nodiscard]] std::vector<int>
placementCandidateChunks(const std::vector<wfc::MapChunk>& chunks)
{
    std::vector<int> candidates;
    for (int index = 0; index < static_cast<int>(chunks.size()); ++index) {
        if (chunks[static_cast<std::size_t>(index)].hasExits) {
            candidates.push_back(index);
        }
    }
    if (!candidates.empty()) {
        return candidates;
    }

    candidates.reserve(chunks.size());
    for (int index = 0; index < static_cast<int>(chunks.size()); ++index) {
        candidates.push_back(index);
    }
    return candidates;
}

[[nodiscard]] bool
compatibleWithPlacedNeighbor(const std::vector<wfc::MapChunk>& chunks,
                             int candidate,
                             wfc::Direction directionToNeighbor,
                             int neighbor)
{
    const auto& compatible = chunks[static_cast<std::size_t>(candidate)]
                                 .compatibleWith[static_cast<std::size_t>(
                                     wfc::directionIndex(directionToNeighbor))];
    return containsChunk(compatible, neighbor);
}

[[nodiscard]] std::vector<int>
optionsForCell(const std::vector<wfc::MapChunk>& chunks,
               const std::vector<PlacedChunk>& placements,
               const std::vector<int>& baseCandidates,
               int gridWidth,
               int gridHeight,
               Point cell)
{
    std::vector<int> options = baseCandidates;
    // collapse only against neighbors that are already fixed, unplaced
    // neighbors still have every option, so they do not constrain this cell
    // until their own turn
    for (const auto direction : directions) {
        const Point delta = deltaFor(direction);
        const Point neighbor{cell.x + delta.x, cell.y + delta.y};
        if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= gridWidth ||
            neighbor.y >= gridHeight) {
            continue;
        }

        const auto& placedNeighbor = placements[static_cast<std::size_t>(
            chunkIndex(gridWidth, neighbor.x, neighbor.y))];
        if (!isPlaced(placedNeighbor)) {
            continue;
        }

        std::vector<int> filtered;
        for (const int candidate : options) {
            if (compatibleWithPlacedNeighbor(
                    chunks, candidate, direction, placedNeighbor.chunk)) {
                filtered.push_back(candidate);
            }
        }
        options = std::move(filtered);
        if (options.empty()) {
            break;
        }
    }

    return options;
}

[[nodiscard]] int
placedNeighborCount(const std::vector<PlacedChunk>& placements,
                    int gridWidth,
                    int gridHeight,
                    Point cell)
{
    int count = 0;
    for (const auto direction : directions) {
        const Point delta = deltaFor(direction);
        const Point neighbor{cell.x + delta.x, cell.y + delta.y};
        if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= gridWidth ||
            neighbor.y >= gridHeight) {
            continue;
        }
        if (isPlaced(placements[static_cast<std::size_t>(
                chunkIndex(gridWidth, neighbor.x, neighbor.y))])) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::vector<Point> initialRemainingCells(int gridWidth,
                                                       int gridHeight)
{
    std::vector<Point> cells;
    cells.reserve(static_cast<std::size_t>(gridWidth * gridHeight));
    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            cells.push_back(Point{x, y});
        }
    }
    return cells;
}

[[nodiscard]] std::optional<std::size_t>
chooseNextCell(const std::vector<PlacedChunk>& placements,
               const std::vector<Point>& remaining,
               int gridWidth,
               int gridHeight,
               Rng& rng)
{
    std::vector<std::size_t> candidates;
    int bestNeighborCount = -1;

    // this scoped solver uses "most constrained by placed neighbors" as a
    // cheap entropy heuristic, it grows from existing constraints instead of
    // dropping isolated chunks that may become incompatible later
    for (std::size_t index = 0; index < remaining.size(); ++index) {
        const Point point = remaining[index];
        const int neighbors =
            placedNeighborCount(placements, gridWidth, gridHeight, point);
        if (neighbors > bestNeighborCount) {
            candidates.clear();
            bestNeighborCount = neighbors;
        }
        if (neighbors == bestNeighborCount) {
            candidates.push_back(index);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    const int pick =
        rng.rangeInclusive(0, static_cast<int>(candidates.size()) - 1);
    return candidates[static_cast<std::size_t>(pick)];
}

[[nodiscard]] std::optional<SolverResult>
solveChunks(const std::vector<wfc::MapChunk>& chunks,
            int gridWidth,
            int gridHeight,
            Rng& rng,
            bool recordSnapshots)
{
    const auto baseCandidates = placementCandidateChunks(chunks);
    std::vector<PlacedChunk> placements(
        static_cast<std::size_t>(gridWidth * gridHeight));
    std::vector<Point> remaining = initialRemainingCells(gridWidth, gridHeight);
    std::vector<SolverSnapshot> snapshots;

    int placementCount = 0;
    while (!remaining.empty()) {
        const auto nextIndex =
            chooseNextCell(placements, remaining, gridWidth, gridHeight, rng);
        if (!nextIndex.has_value()) {
            break;
        }
        const Point next = remaining[*nextIndex];

        auto options = optionsForCell(
            chunks, placements, baseCandidates, gridWidth, gridHeight, next);
        if (options.empty()) {
            return std::nullopt;
        }

        const int option = options[static_cast<std::size_t>(
            rng.rangeInclusive(0, static_cast<int>(options.size()) - 1))];
        placements[static_cast<std::size_t>(
            chunkIndex(gridWidth, next.x, next.y))] = PlacedChunk{option};
        remaining.erase(remaining.begin() +
                        static_cast<std::ptrdiff_t>(*nextIndex));
        ++placementCount;

        if (recordSnapshots) {
            snapshots.push_back(SolverSnapshot{placementCount, placements});
        }
    }

    for (const auto& placement : placements) {
        if (!isPlaced(placement)) {
            return std::nullopt;
        }
    }

    return SolverResult{std::move(placements), std::move(snapshots)};
}

[[nodiscard]] const wfc::MapChunk&
chunkAt(const std::vector<wfc::MapChunk>& chunks, const PlacedChunk& placement)
{
    return chunks[static_cast<std::size_t>(placement.chunk)];
}

void stampChunk(Map& map, const wfc::MapChunk& chunk, int chunkX, int chunkY)
{
    const int originX = chunkX * wfc::chunkSize;
    const int originY = chunkY * wfc::chunkSize;
    for (int y = 0; y < wfc::chunkSize; ++y) {
        for (int x = 0; x < wfc::chunkSize; ++x) {
            const int mapX = originX + x;
            const int mapY = originY + y;
            if (!map.inBounds(mapX, mapY)) {
                continue;
            }
            map.setTile(
                mapX,
                mapY,
                chunk
                    .pattern[static_cast<std::size_t>(y * wfc::chunkSize + x)]);
        }
    }
}

[[nodiscard]] Map mapFromPlacements(const std::vector<wfc::MapChunk>& chunks,
                                    const std::vector<PlacedChunk>& placements,
                                    int gridWidth,
                                    int gridHeight,
                                    int mapWidth,
                                    int mapHeight)
{
    Map map{mapWidth, mapHeight, TileType::Wall};
    for (int y = 0; y < gridHeight; ++y) {
        for (int x = 0; x < gridWidth; ++x) {
            const auto& placement = placements[static_cast<std::size_t>(
                chunkIndex(gridWidth, x, y))];
            if (isPlaced(placement)) {
                stampChunk(map, chunkAt(chunks, placement), x, y);
            }
        }
    }
    return map;
}

[[nodiscard]] Map
snapshotMapFromPlacements(const std::vector<wfc::MapChunk>& chunks,
                          const std::vector<PlacedChunk>& placements,
                          int gridWidth,
                          int gridHeight,
                          int mapWidth,
                          int mapHeight)
{
    return mapFromPlacements(
        chunks, placements, gridWidth, gridHeight, mapWidth, mapHeight);
}

[[nodiscard]] Map sourceSnapshotMap(const wfc::SourceMap& source)
{
    Map map{source.width, source.height, TileType::Wall};
    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            map.setTile(x, y, wfc::tileAt(source, x, y));
        }
    }
    return map;
}

[[nodiscard]] Map chunkGallerySnapshot(const std::vector<wfc::MapChunk>& chunks)
{
    constexpr int galleryColumns = 4;
    constexpr int gap = 1;
    // gallery snapshots are not playable maps, they lay chunk patterns out
    // with wall gaps so exit masks and deduped variants can be inspected
    const int rows =
        std::max(1,
                 (static_cast<int>(chunks.size()) + galleryColumns - 1) /
                     galleryColumns);
    const int width =
        (galleryColumns * wfc::chunkSize) + ((galleryColumns - 1) * gap);
    const int height = (rows * wfc::chunkSize) + ((rows - 1) * gap);

    Map gallery{width, height, TileType::Wall};
    for (int index = 0; index < static_cast<int>(chunks.size()); ++index) {
        const int column = index % galleryColumns;
        const int row = index / galleryColumns;
        const int originX = column * (wfc::chunkSize + gap);
        const int originY = row * (wfc::chunkSize + gap);
        const auto& chunk = chunks[static_cast<std::size_t>(index)];
        for (int y = 0; y < wfc::chunkSize; ++y) {
            for (int x = 0; x < wfc::chunkSize; ++x) {
                gallery.setTile(originX + x,
                                originY + y,
                                chunk.pattern[static_cast<std::size_t>(
                                    y * wfc::chunkSize + x)]);
            }
        }
    }
    return gallery;
}

void recordWfcSetupSnapshots(std::vector<MapSnapshot>& snapshots,
                             const std::vector<wfc::SourceMap>& sources,
                             const std::vector<wfc::MapChunk>& chunks,
                             bool enabled)
{
    if (!enabled) {
        return;
    }

    for (std::size_t index = 0; index < sources.size(); ++index) {
        snapshots.push_back(MapSnapshot{
            "wfc source " + std::to_string(index + 1),
            sourceSnapshotMap(sources[index]),
        });
    }
    snapshots.push_back(
        MapSnapshot{"wfc chunk gallery exits", chunkGallerySnapshot(chunks)});
}

void recordPlacementSnapshots(std::vector<MapSnapshot>& snapshots,
                              const std::vector<wfc::MapChunk>& chunks,
                              const std::vector<SolverSnapshot>& solver,
                              int gridWidth,
                              int gridHeight,
                              int mapWidth,
                              int mapHeight,
                              bool enabled)
{
    if (!enabled) {
        return;
    }

    for (const auto& snapshot : solver) {
        snapshots.push_back(MapSnapshot{
            "wfc placement " + std::to_string(snapshot.placementCount),
            snapshotMapFromPlacements(chunks,
                                      snapshot.placements,
                                      gridWidth,
                                      gridHeight,
                                      mapWidth,
                                      mapHeight),
        });
    }
}

[[nodiscard]] std::vector<std::filesystem::path> defaultSourcePaths()
{
    return {
        std::filesystem::path{"assets/wfc-demo1.xp"},
        std::filesystem::path{"assets/wfc-demo2.xp"},
    };
}

[[nodiscard]] std::vector<wfc::SourceMap>
loadDefaultSources(const std::vector<std::filesystem::path>& paths)
{
    std::vector<wfc::SourceMap> sources;
    sources.reserve(paths.size());
    for (const auto& path : paths) {
        sources.push_back(wfc::loadSourceMap(path));
    }
    return sources;
}

struct ChunkLibrary {
    std::vector<wfc::SourceMap> sources;
    std::vector<wfc::MapChunk> chunks;
};

[[nodiscard]] ChunkLibrary buildChunkLibrary()
{
    ChunkLibrary library;
    library.sources = loadDefaultSources(defaultSourcePaths());
    library.chunks = wfc::extractChunks(library.sources);
    wfc::buildCompatibility(library.chunks);
    if (library.chunks.empty()) {
        throw std::runtime_error("WFC source maps did not provide chunks");
    }
    return library;
}

[[nodiscard]] const ChunkLibrary& cachedChunkLibrary()
{
    static const ChunkLibrary library = buildChunkLibrary();
    return library;
}

[[nodiscard]] int chunksForDimension(int dimension)
{
    return std::max(1, (dimension + wfc::chunkSize - 1) / wfc::chunkSize);
}

} // namespace

namespace wfc {

int directionIndex(Direction direction) noexcept
{
    return static_cast<int>(direction);
}

Direction opposite(Direction direction) noexcept
{
    switch (direction) {
    case Direction::North:
        return Direction::South;
    case Direction::East:
        return Direction::West;
    case Direction::South:
        return Direction::North;
    case Direction::West:
        return Direction::East;
    }
    return Direction::North;
}

TileType tileAt(const SourceMap& source, int x, int y)
{
    if (x < 0 || y < 0 || x >= source.width || y >= source.height) {
        throw std::out_of_range("WFC source map coordinate is out of bounds");
    }
    return source.tiles[static_cast<std::size_t>(y * source.width + x)];
}

SourceMap loadSourceMap(const std::filesystem::path& path)
{
    const auto image = loadRexPaintImage(path);
    SourceMap source;
    source.width = image.width;
    source.height = image.height;
    source.tiles.assign(static_cast<std::size_t>(source.width * source.height),
                        TileType::Wall);

    for (const auto& layer : image.layers) {
        for (int y = 0; y < layer.height; ++y) {
            for (int x = 0; x < layer.width; ++x) {
                const auto& cell = layer.cellAt(x, y);
                if (cell.transparentBackground) {
                    continue;
                }
                source.tiles[static_cast<std::size_t>(y * source.width + x)] =
                    cell.glyph == U'#' ? TileType::Wall : TileType::Floor;
            }
        }
    }

    return source;
}

MapChunk makeChunk(const ChunkPattern& pattern)
{
    MapChunk chunk;
    chunk.pattern = pattern;
    for (auto& mask : chunk.exits) {
        mask.fill(false);
    }

    auto setExit = [&chunk](Direction direction, int offset, TileType tile) {
        const bool passable = isConnectivityPassable(tile);
        chunk.exits[static_cast<std::size_t>(directionIndex(direction))]
                   [static_cast<std::size_t>(offset)] = passable;
        chunk.hasExits = chunk.hasExits || passable;
    };

    // Edge masks are the WFC constraints: a corridor can cross a chunk edge
    // only when both touching chunks expose passable cells at the same offsets
    for (int offset = 0; offset < chunkSize; ++offset) {
        setExit(Direction::North,
                offset,
                pattern[static_cast<std::size_t>(offset)]);
        setExit(Direction::East,
                offset,
                pattern[static_cast<std::size_t>(offset * chunkSize +
                                                 (chunkSize - 1))]);
        setExit(Direction::South,
                offset,
                pattern[static_cast<std::size_t>((chunkSize - 1) * chunkSize +
                                                 offset)]);
        setExit(Direction::West,
                offset,
                pattern[static_cast<std::size_t>(offset * chunkSize)]);
    }

    return chunk;
}

ChunkPattern flipHorizontal(const ChunkPattern& pattern)
{
    ChunkPattern flipped{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            flipped[static_cast<std::size_t>(y * chunkSize + x)] =
                pattern[static_cast<std::size_t>(y * chunkSize +
                                                 (chunkSize - x - 1))];
        }
    }
    return flipped;
}

ChunkPattern flipVertical(const ChunkPattern& pattern)
{
    ChunkPattern flipped{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            flipped[static_cast<std::size_t>(y * chunkSize + x)] =
                pattern[static_cast<std::size_t>(
                    (chunkSize - y - 1) * chunkSize + x)];
        }
    }
    return flipped;
}

namespace {

[[nodiscard]] ChunkPattern
patternFromSourceChunk(const SourceMap& source, int originX, int originY)
{
    ChunkPattern pattern{};
    for (int y = 0; y < chunkSize; ++y) {
        for (int x = 0; x < chunkSize; ++x) {
            pattern[static_cast<std::size_t>(y * chunkSize + x)] =
                tileAt(source, originX + x, originY + y);
        }
    }
    return pattern;
}

[[nodiscard]] bool samePattern(const MapChunk& chunk,
                               const ChunkPattern& pattern)
{
    return chunk.pattern == pattern;
}

void addUniqueChunk(std::vector<MapChunk>& chunks, const ChunkPattern& pattern)
{
    const auto exists =
        std::any_of(chunks.begin(), chunks.end(), [&pattern](const auto& c) {
            return samePattern(c, pattern);
        });
    if (!exists) {
        chunks.push_back(makeChunk(pattern));
    }
}

[[nodiscard]] bool compatibleEdges(const MapChunk& source,
                                   const MapChunk& neighbor,
                                   Direction direction)
{
    const auto sourceIndex =
        static_cast<std::size_t>(directionIndex(direction));
    const auto neighborIndex =
        static_cast<std::size_t>(directionIndex(opposite(direction)));
    // Exact mask equality prevents half-open seams: every passable edge cell
    // on one chunk must meet a passable edge cell on the neighbor
    return source.exits[sourceIndex] == neighbor.exits[neighborIndex];
}

} // namespace

std::vector<MapChunk> extractChunks(const std::vector<SourceMap>& sources)
{
    std::vector<MapChunk> chunks;
    for (const auto& source : sources) {
        for (int y = 0; y <= source.height - chunkSize; y += chunkSize) {
            for (int x = 0; x <= source.width - chunkSize; x += chunkSize) {
                const auto pattern = patternFromSourceChunk(source, x, y);
                const auto horizontal = flipHorizontal(pattern);
                const auto vertical = flipVertical(pattern);
                addUniqueChunk(chunks, pattern);
                addUniqueChunk(chunks, horizontal);
                addUniqueChunk(chunks, vertical);
                addUniqueChunk(chunks, flipVertical(horizontal));
            }
        }
    }
    return chunks;
}

void buildCompatibility(std::vector<MapChunk>& chunks)
{
    for (auto& chunk : chunks) {
        for (auto& compatible : chunk.compatibleWith) {
            compatible.clear();
        }
    }

    for (int source = 0; source < static_cast<int>(chunks.size()); ++source) {
        for (int neighbor = 0; neighbor < static_cast<int>(chunks.size());
             ++neighbor) {
            for (const auto direction : directions) {
                if (compatibleEdges(chunks[static_cast<std::size_t>(source)],
                                    chunks[static_cast<std::size_t>(neighbor)],
                                    direction)) {
                    chunks[static_cast<std::size_t>(source)]
                        .compatibleWith[static_cast<std::size_t>(
                            directionIndex(direction))]
                        .push_back(neighbor);
                }
            }
        }
    }
}

} // namespace wfc

MapBuildResult
WfcBuilder::build(int width, int height, Rng& rng, MapBuildOptions options)
{
    validateBuilderDimensions("WfcBuilder", width, height);

    const auto& library = cachedChunkLibrary();
    const int gridWidth = chunksForDimension(width);
    const int gridHeight = chunksForDimension(height);

    std::optional<SolverResult> solved;
    for (int attempt = 0; attempt < restartAttempts; ++attempt) {
        // if an earlier random placement made some later
        // cell impossible then restarting is simpler & safer than partial
        // backtracking for this fixed-size chunk solver
        solved = solveChunks(library.chunks,
                             gridWidth,
                             gridHeight,
                             rng,
                             options.recordSnapshots);
        if (solved.has_value()) {
            break;
        }
    }
    if (!solved.has_value()) {
        throw std::runtime_error("WFC failed to find a valid chunk collapse");
    }

    MapBuildResult result{mapFromPlacements(library.chunks,
                                            solved->placements,
                                            gridWidth,
                                            gridHeight,
                                            width,
                                            height),
                          {},
                          {}};
    result.builderName = "wfc";
    addBuildStep(result, "source: rexpaint chunk library");
    addBuildStep(result, "base: chunk wfc");
    addBuildStep(result, "solver: neighbor-count collapse");
    recordWfcSetupSnapshots(result.snapshots,
                            library.sources,
                            library.chunks,
                            options.recordSnapshots);
    recordPlacementSnapshots(result.snapshots,
                             library.chunks,
                             solved->snapshots,
                             gridWidth,
                             gridHeight,
                             width,
                             height,
                             options.recordSnapshots);

    auto start =
        closestReachableFloorTo(result.map, Point{width / 2, height / 2});
    if (!start.has_value()) {
        result.map.setTile(width / 2, height / 2, TileType::Floor);
        start = Point{width / 2, height / 2};
    }
    result.playerStart = *start;
    repairConnectivity(result.map, result.playerStart, rng);
    sealMapEdges(result.map);
    cullUnreachable(result.map, result.playerStart);
    setDistantStairs(result.map, result.playerStart, result.stairsPosition);
    result.spawnRegions =
        spawnRegionsFromConnectedFloors(result.map, SpawnRegionKind::Room);
    recordSnapshot(
        result.snapshots, result.map, "wfc output", options.recordSnapshots);
    addBuildStep(result, "modifier: connectivity repair");
    addBuildStep(result, "modifier: distant exit");

    validateBuildOrThrow(result, "WFC generation produced an invalid map");

    return result;
}

} // namespace rl
