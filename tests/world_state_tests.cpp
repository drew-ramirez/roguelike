#include "TestSuites.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

#include "world/WorldGeneration.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool inUnitRange(double value) noexcept
{
    return value >= 0.0 && value <= 1.0;
}

[[nodiscard]] bool fieldsInRange(const WorldCellFields& fields) noexcept
{
    const std::array values{fields.elevation,
                            fields.moisture,
                            fields.woodlandDensity,
                            fields.marshPressure,
                            fields.ruinDensity,
                            fields.dangerPressure};
    for (const double value : values) {
        if (!inUnitRange(value)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool sameFields(const WorldCellFields& left,
                              const WorldCellFields& right) noexcept
{
    return left.elevation == right.elevation &&
           left.moisture == right.moisture &&
           left.woodlandDensity == right.woodlandDensity &&
           left.marshPressure == right.marshPressure &&
           left.ruinDensity == right.ruinDensity &&
           left.dangerPressure == right.dangerPressure;
}

[[nodiscard]] bool allCellsHaveState(const WorldMapState& world)
{
    if (world.cells.size() !=
        static_cast<std::size_t>(worldMapWidth * worldMapHeight)) {
        return false;
    }
    for (int y = 0; y < worldMapHeight; ++y) {
        for (int x = 0; x < worldMapWidth; ++x) {
            const Point point{x, y};
            const auto* cell = worldCellAt(world, point);
            if (cell == nullptr || cell->position != point ||
                !fieldsInRange(cell->fields) || cell->glyph == U'\0' ||
                cell->regionId.empty() ||
                world.regions.find(cell->regionId) == world.regions.end()) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool throwsForInvalidWorldCell()
{
    try {
        (void)worldCellIndex(Point{-1, 0});
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

[[nodiscard]] bool throwsForInvalidLocalScreen(const WorldMapState& world)
{
    try {
        (void)describeLocalScreen(world, Point{0, 0}, Point{3, 1});
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

} // namespace

void runWorldStateTests(TestRunner& runner)
{
    {
        const auto world = generateWorldMap(Rng::Seed{321U});

        runner.expect(world.active && world.onSurface,
                      "generated world starts as an active surface map");
        runner.expect(world.currentMacro == greenholtWorldPosition() &&
                          world.currentScreen == Point{1, 1} &&
                          world.cursor == greenholtWorldPosition() &&
                          world.currentRegionId == "greenholt_hearth",
                      "generated world starts at Greenholt center");
        runner.expect(world.regions.size() == world.cells.size() &&
                          allCellsHaveState(world),
                      "generated world fills cells and region descriptors");
        runner.expect(worldCellAt(world, Point{-1, 0}) == nullptr &&
                          worldCellAt(WorldMapState{}, Point{0, 0}) ==
                              nullptr &&
                          throwsForInvalidWorldCell(),
                      "world cell lookup handles invalid state safely");
    }

    {
        const auto first = generateWorldMap(Rng::Seed{500U});
        const auto second = generateWorldMap(Rng::Seed{500U});
        const auto changed = generateWorldMap(Rng::Seed{501U});
        const auto* firstCell = worldCellAt(first, Point{0, 0});
        const auto* secondCell = worldCellAt(second, Point{0, 0});
        const auto* changedCell = worldCellAt(changed, Point{0, 0});

        runner.expect(firstCell != nullptr && secondCell != nullptr &&
                          changedCell != nullptr,
                      "test setup resolves generated cells");
        runner.expect(firstCell != nullptr && secondCell != nullptr &&
                          sameFields(firstCell->fields, secondCell->fields) &&
                          firstCell->biome == secondCell->biome &&
                          firstCell->localSeed == secondCell->localSeed,
                      "same seed reproduces macro cell fields");
        runner.expect(firstCell != nullptr && changedCell != nullptr &&
                          !sameFields(firstCell->fields, changedCell->fields),
                      "changing the seed changes generated cell fields");
    }

    {
        auto world = generateWorldMap(Rng::Seed{77U});
        auto* greenholt = worldRegionById(world, "greenholt_hearth");
        auto* reedfen = worldRegionById(world, "reedfen_ford");
        auto* limestone = worldRegionById(world, "limestone_sink");
        const auto* greenholtCell =
            worldCellAt(world, greenholtWorldPosition());
        const auto* reedfenCell = worldCellAt(world, reedfenWorldPosition());
        const auto* limestoneCell =
            worldCellAt(world, limestoneWorldPosition());

        runner.expect(
            greenholtCell != nullptr && greenholtCell->discovered &&
                greenholtCell->visited && greenholtCell->landmark.has_value() &&
                greenholtCell->landmark->regionId == "greenholt_hearth",
            "Greenholt starts discovered with a landmark");
        runner.expect(reedfenCell != nullptr &&
                          reedfenCell->biome == BiomeVariant::ReedMarsh &&
                          !reedfenCell->discovered &&
                          reedfenCell->landmark.has_value(),
                      "Reedfen starts hidden as a marsh landmark");
        runner.expect(limestoneCell != nullptr &&
                          limestoneCell->biome == BiomeVariant::LimestoneRise &&
                          !limestoneCell->discovered &&
                          limestoneCell->landmark.has_value(),
                      "Limestone starts hidden as a rise landmark");
        runner.expect(greenholt != nullptr && greenholt->hasSettlement &&
                          greenholt->hasCamp && greenholt->visited &&
                          findDiscoveredRoute(*greenholt, "reedfen_ford") !=
                              nullptr,
                      "Greenholt exposes settlement, camp, and Reedfen route");
        runner.expect(
            reedfen != nullptr && reedfen->hasCamp && reedfen->hasShrine &&
                findDiscoveredRoute(*reedfen, "greenholt_hearth") != nullptr,
            "Reedfen exposes camp, shrine, and return route");
        runner.expect(limestone != nullptr && limestone->hasCamp &&
                          limestone->hasShrine &&
                          findDiscoveredRoute(*limestone, "reedfen_ford") !=
                              nullptr,
                      "Limestone exposes camp, shrine, and Reedfen route");

        world.currentMacro = reedfenWorldPosition();
        world.currentScreen = Point{1, 1};
        updateCurrentWorldRegion(world);
        auto* currentReedfen = worldCellAt(world, reedfenWorldPosition());
        runner.expect(currentWorldRegion(world) == reedfen &&
                          currentReedfen != nullptr &&
                          currentReedfen->discovered && currentReedfen->visited,
                      "updating current world region discovers landmarks");
        runner.expect(currentReedfen != nullptr &&
                          unlockCurrentLandmarkFastTravel(world) &&
                          currentReedfen->landmark.has_value() &&
                          currentReedfen->landmark->fastTravelUnlocked,
                      "landmark center screen unlocks fast travel");

        world.currentScreen = Point{0, 0};
        runner.expect(!unlockCurrentLandmarkFastTravel(world),
                      "non-landmark local screens do not unlock fast travel");
        if (greenholt != nullptr && !greenholt->routes.empty()) {
            greenholt->routes.front().discovered = false;
            runner.expect(discoveredRoutes(*greenholt).empty() &&
                              findDiscoveredRoute(*greenholt, "reedfen_ford") ==
                                  nullptr,
                          "route discovery filters hidden links");
        }
    }

    {
        const auto world = generateWorldMap(Rng::Seed{42U});
        const auto greenholtCenter =
            describeLocalScreen(world, greenholtWorldPosition(), Point{1, 1});
        const auto greenholtEdge =
            describeLocalScreen(world, greenholtWorldPosition(), Point{1, 0});
        const auto reedfenCenter =
            describeLocalScreen(world, reedfenWorldPosition(), Point{1, 1});
        const auto limestoneCenter =
            describeLocalScreen(world, limestoneWorldPosition(), Point{1, 1});
        const auto danger = describeLocalScreen(
            world, greenholtWorldPosition(), Point{2, 1}, true);

        runner.expect(greenholtCenter.role == LocalScreenRole::CenterLandmark &&
                          greenholtCenter.builderId == "town" &&
                          greenholtCenter.landmarkId == "greenholt",
                      "Greenholt center uses the town landmark builder");
        runner.expect(greenholtEdge.role == LocalScreenRole::TownOutskirts &&
                          greenholtEdge.builderId == "surface-town-outskirts" &&
                          (greenholtEdge.exitMask & exitNorth) != 0,
                      "Greenholt cardinal screens use outskirts builders");
        runner.expect(reedfenCenter.role == LocalScreenRole::CenterLandmark &&
                          reedfenCenter.builderId == "surface-camp-shrine",
                      "Reedfen center uses camp shrine local content");
        runner.expect(limestoneCenter.role == LocalScreenRole::CenterLandmark &&
                          limestoneCenter.builderId == "surface-cave-mouth",
                      "Limestone center uses cave mouth local content");
        runner.expect(danger.role == LocalScreenRole::DangerEncounter &&
                          danger.builderId == "surface-danger-encounter" &&
                          danger.dangerEncounter,
                      "danger local screens use danger encounter content");
        runner.expect(throwsForInvalidLocalScreen(world),
                      "invalid local screen coordinates reject descriptors");
    }
}

} // namespace rl::test
