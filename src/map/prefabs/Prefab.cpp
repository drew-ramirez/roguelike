/**
 * **************************************************************************
 * @file     Prefab.cpp
 * @brief    Implements reusable prefab and sectional placement helpers
 * **************************************************************************
 */

#include "map/prefabs/Prefab.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "assets/RexPaintLoader.hpp"
#include "map/MapBuilderCommon.hpp"
#include "map/TileType.hpp"

namespace rl {
namespace {

[[nodiscard]] bool isRectangular(const PrefabTemplate& prefab)
{
    if (prefab.rows.empty() || prefab.rows.front().empty()) {
        return false;
    }

    const auto width = prefab.rows.front().size();
    return std::all_of(
        prefab.rows.begin(), prefab.rows.end(), [width](const auto& row) {
            return row.size() == width;
        });
}

void validatePrefab(const PrefabTemplate& prefab)
{
    if (!isRectangular(prefab)) {
        throw std::invalid_argument("Prefab rows must be rectangular");
    }
}

[[nodiscard]] bool canPlacePrefab(const MapBuildResult& result,
                                  const PrefabTemplate& prefab,
                                  Point origin)
{
    for (int y = 0; y < prefabHeight(prefab); ++y) {
        for (int x = 0; x < prefabWidth(prefab); ++x) {
            const Point point{origin.x + x, origin.y + y};
            if (!result.map.inBounds(point.x, point.y) ||
                point == result.playerStart || point == result.stairsPosition) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool
inPrefabArea(Point point, Point origin, int width, int height)
{
    return point.x >= origin.x && point.y >= origin.y &&
           point.x < origin.x + width && point.y < origin.y + height;
}

void removeSpawnTilesInArea(MapBuildResult& result,
                            Point origin,
                            int width,
                            int height)
{
    // Prefabs own every tile in their footprint, remove generic spawn points
    // there first so marker-driven spawn requests do not stack with random
    // room population
    for (auto& region : result.spawnRegions) {
        region.tiles.erase(std::remove_if(region.tiles.begin(),
                                          region.tiles.end(),
                                          [origin, width, height](Point point) {
                                              return inPrefabArea(
                                                  point, origin, width, height);
                                          }),
                           region.tiles.end());
    }
    result.spawnRegions.erase(std::remove_if(result.spawnRegions.begin(),
                                             result.spawnRegions.end(),
                                             [](const SpawnRegion& region) {
                                                 return region.tiles.empty();
                                             }),
                              result.spawnRegions.end());
    result.spawnRequests.erase(
        std::remove_if(result.spawnRequests.begin(),
                       result.spawnRequests.end(),
                       [origin, width, height](const SpawnRequest& request) {
                           return inPrefabArea(
                               request.position, origin, width, height);
                       }),
        result.spawnRequests.end());
}

[[nodiscard]] char markerFromRexGlyph(char32_t glyph)
{
    if (glyph == U'\0') {
        return ' ';
    }
    if (glyph > static_cast<char32_t>(0x7f)) {
        return '.';
    }
    return static_cast<char>(glyph);
}

void addSpawnRequestForMarker(MapBuildResult& result, char marker, Point point)
{
    if (marker == 'M') {
        result.spawnRequests.push_back(SpawnRequest{"goblin", point});
    } else if (marker == 'O') {
        result.spawnRequests.push_back(SpawnRequest{"orc", point});
    } else if (marker == '!') {
        result.spawnRequests.push_back(SpawnRequest{"healing_potion", point});
    } else if (marker == '%') {
        result.spawnRequests.push_back(SpawnRequest{"food_ration", point});
    } else if (marker == '^') {
        result.spawnRequests.push_back(SpawnRequest{"spike_trap", point});
    }
}

void applyPrefab(MapBuildResult& result,
                 const PrefabTemplate& prefab,
                 Point origin,
                 bool recordSnapshots)
{
    const int width = prefabWidth(prefab);
    const int height = prefabHeight(prefab);
    removeSpawnTilesInArea(result, origin, width, height);

    std::vector<Point> spawnTiles;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const char marker = prefab.rows[static_cast<std::size_t>(y)]
                                           [static_cast<std::size_t>(x)];
            if (marker == ' ') {
                continue;
            }

            const Point point{origin.x + x, origin.y + y};
            // terrain markers are consumed here, entity markers become floor
            // plus SpawnRequest entries
            if (marker == '#') {
                result.map.setTile(point.x, point.y, TileType::Wall);
            } else if (marker == '+') {
                result.map.setTile(point.x, point.y, TileType::ClosedDoor);
            } else {
                result.map.setTile(point.x, point.y, TileType::Floor);
                spawnTiles.push_back(point);
                addSpawnRequestForMarker(result, marker, point);
            }
        }
    }

    if (!spawnTiles.empty()) {
        result.spawnRegions.push_back(
            spawnRegionFromTiles(std::move(spawnTiles), SpawnRegionKind::Room));
    }
    recordSnapshot(result.snapshots,
                   result.map,
                   "placed prefab " + prefab.name,
                   recordSnapshots);
}

[[nodiscard]] Point fixedOrigin(const Map& map,
                                const PrefabTemplate& prefab,
                                PrefabPlacementMode mode)
{
    const int maxX = std::max(1, map.width() - prefabWidth(prefab) - 1);
    const int maxY = std::max(1, map.height() - prefabHeight(prefab) - 1);
    switch (mode) {
    case PrefabPlacementMode::Center:
        return Point{std::max(1, (map.width() - prefabWidth(prefab)) / 2),
                     std::max(1, (map.height() - prefabHeight(prefab)) / 2)};
    case PrefabPlacementMode::Northwest:
        return Point{1, 1};
    case PrefabPlacementMode::Northeast:
        return Point{maxX, 1};
    case PrefabPlacementMode::Southwest:
        return Point{1, maxY};
    case PrefabPlacementMode::Southeast:
        return Point{maxX, maxY};
    case PrefabPlacementMode::Random:
        break;
    }
    return Point{1, 1};
}

[[nodiscard]] Point sectionOrigin(const Map& map, const PrefabSection& section)
{
    const int width = prefabWidth(section.prefab);
    const int height = prefabHeight(section.prefab);
    const int maxX = std::max(1, map.width() - width - 1);
    const int maxY = std::max(1, map.height() - height - 1);

    int x = (map.width() - width) / 2;
    if (section.horizontal == PrefabHorizontalPlacement::Left) {
        x = 1;
    } else if (section.horizontal == PrefabHorizontalPlacement::Right) {
        x = maxX;
    }

    int y = (map.height() - height) / 2;
    if (section.vertical == PrefabVerticalPlacement::Top) {
        y = 1;
    } else if (section.vertical == PrefabVerticalPlacement::Bottom) {
        y = maxY;
    }

    return Point{std::max(1, x), std::max(1, y)};
}

[[nodiscard]] PrefabPlacementResult fail(std::string reason)
{
    return PrefabPlacementResult{false, {}, std::move(reason)};
}

} // namespace

const PrefabTemplate& shrinePrefab()
{
    static const PrefabTemplate piece{"shrine",
                                      {
                                          "#####",
                                          "#...#",
                                          "#.!M#",
                                          "#...#",
                                          "##+##",
                                      }};
    return piece;
}

const PrefabTemplate& armoryVaultPrefab()
{
    static const PrefabTemplate piece{"armory vault",
                                      {
                                          "#######",
                                          "#M...M#",
                                          "#.###.#",
                                          "#.!^!.#",
                                          "#.....#",
                                          "###+###",
                                      }};
    return piece;
}

namespace {

const PrefabTemplate& goblinDenPrefab()
{
    static const PrefabTemplate piece{"goblin den",
                                      {
                                          "######",
                                          "#M..M#",
                                          "#.##.#",
                                          "#.!..#",
                                          "##+###",
                                      }};
    return piece;
}

const PrefabTemplate& trapVaultPrefab()
{
    static const PrefabTemplate piece{"trap vault",
                                      {
                                          "#######",
                                          "#^...^#",
                                          "#.###.#",
                                          "#.!O!.#",
                                          "###+###",
                                      }};
    return piece;
}

} // namespace

PrefabTemplate prefabFromAscii(std::string name, std::vector<std::string> rows)
{
    PrefabTemplate prefab{std::move(name), std::move(rows)};
    validatePrefab(prefab);
    return prefab;
}

PrefabTemplate loadPrefabFromRexPaint(const std::filesystem::path& path,
                                      std::string name)
{
    const auto image = loadRexPaintImage(path);
    std::vector<std::string> rows(
        static_cast<std::size_t>(image.height),
        std::string(static_cast<std::size_t>(image.width), ' '));
    for (const auto& layer : image.layers) {
        for (int y = 0; y < layer.height; ++y) {
            for (int x = 0; x < layer.width; ++x) {
                const auto& cell = layer.cellAt(x, y);
                if (cell.transparentBackground) {
                    continue;
                }
                rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] =
                    markerFromRexGlyph(cell.glyph);
            }
        }
    }
    return prefabFromAscii(std::move(name), std::move(rows));
}

std::vector<PrefabVault> roomVaultLibrary()
{
    return {
        PrefabVault{shrinePrefab(), 1, 3},
        PrefabVault{goblinDenPrefab(), 1, 5},
        PrefabVault{armoryVaultPrefab(), 2, 999},
        PrefabVault{trapVaultPrefab(), 3, 999},
    };
}

std::vector<PrefabVault> vaultsForDepth(int depth)
{
    std::vector<PrefabVault> vaults;
    for (const auto& vault : roomVaultLibrary()) {
        if (depth >= vault.minimumDepth && depth <= vault.maximumDepth) {
            vaults.push_back(vault);
        }
    }
    return vaults;
}

int prefabWidth(const PrefabTemplate& prefab)
{
    validatePrefab(prefab);
    return static_cast<int>(prefab.rows.front().size());
}

int prefabHeight(const PrefabTemplate& prefab)
{
    validatePrefab(prefab);
    return static_cast<int>(prefab.rows.size());
}

PrefabPlacementResult placeSection(MapBuildResult& result,
                                   const PrefabSection& section,
                                   MapBuildOptions options)
{
    if (!isRectangular(section.prefab)) {
        return fail("non-rectangular prefab");
    }

    const int maxX = result.map.width() - prefabWidth(section.prefab) - 1;
    const int maxY = result.map.height() - prefabHeight(section.prefab) - 1;
    if (maxX <= 1 || maxY <= 1) {
        return fail("section does not fit map");
    }

    const Point origin = sectionOrigin(result.map, section);
    if (!canPlacePrefab(result, section.prefab, origin)) {
        return fail("section placement blocked");
    }

    applyPrefab(result, section.prefab, origin, options.recordSnapshots);
    return PrefabPlacementResult{true, origin, {}};
}

PrefabPlacementResult placePrefab(MapBuildResult& result,
                                  const PrefabTemplate& prefab,
                                  Rng& rng,
                                  MapBuildOptions options,
                                  PrefabPlacementMode mode,
                                  int maxAttempts)
{
    if (!isRectangular(prefab)) {
        return fail("non-rectangular prefab");
    }

    const int maxX = result.map.width() - prefabWidth(prefab) - 1;
    const int maxY = result.map.height() - prefabHeight(prefab) - 1;
    if (maxX <= 1 || maxY <= 1) {
        return fail("prefab does not fit map");
    }

    if (mode != PrefabPlacementMode::Random) {
        const Point origin = fixedOrigin(result.map, prefab, mode);
        if (!canPlacePrefab(result, prefab, origin)) {
            return fail("fixed placement blocked");
        }
        applyPrefab(result, prefab, origin, options.recordSnapshots);
        return PrefabPlacementResult{true, origin, {}};
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        const Point origin{rng.rangeInclusive(1, maxX),
                           rng.rangeInclusive(1, maxY)};
        if (!canPlacePrefab(result, prefab, origin)) {
            continue;
        }
        applyPrefab(result, prefab, origin, options.recordSnapshots);
        return PrefabPlacementResult{true, origin, {}};
    }

    return fail("no valid random placement");
}

} // namespace rl
