#include "TestSuites.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "core/Rng.hpp"
#include "map/DungeonRegionRouter.hpp"
#include "map/MapBuilderRegistry.hpp"
#include "map/TileType.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool hasBuildStep(const MapBuildResult& result,
                                std::string_view step)
{
    return std::any_of(result.buildSteps.begin(),
                       result.buildSteps.end(),
                       [step](const std::string& candidate) {
                           return candidate == step;
                       });
}

} // namespace

void runRegionRoutingTests(TestRunner& runner)
{
    runner.expect(routeForDepth(1).region == DungeonRegion::Town,
                  "depth 1 routes to town");
    runner.expect(routeForDepth(2).region == DungeonRegion::Woods,
                  "depth 2 routes to woods");
    runner.expect(routeForDepth(3).region == DungeonRegion::LimestoneCaverns,
                  "depth 3 routes to limestone caverns");
    runner.expect(routeForDepth(8).region == DungeonRegion::DragonLair,
                  "depth 8 routes to dragon lair");
    runner.expect(routeForDepth(11).region == DungeonRegion::DarkElfCity,
                  "deep routes reach dark elf city");

    Rng townRng{99U};
    const auto town = buildLevelForDepth(1, 40, 25, townRng, {});
    runner.expect(town.builderName == "town", "depth 1 builds town");
    runner.expect(hasBuildStep(town, "region:town"),
                  "town build records its region route");
    runner.expect(
        town.map.tileAt(town.stairsPosition.x, town.stairsPosition.y) ==
            TileType::DownStairs,
        "town has a down exit");

    Rng cavernRng{101U};
    const auto cavern = buildLevelForDepth(3, 40, 25, cavernRng, {});
    runner.expect(cavern.builderName == "cellular",
                  "limestone depths reuse the cellular cavern builder");
    runner.expect(hasBuildStep(cavern, "region:limestone"),
                  "limestone build records its region route");

    Rng deepRng{102U};
    const auto deep = buildLevelForDepth(5, 40, 25, deepRng, {});
    runner.expect(deep.builderName == "cavern-vaults",
                  "deep depths use cavern vaults");

    Rng finalRng{103U};
    const auto final = buildLevelForDepth(11, 40, 25, finalRng, {});
    runner.expect(final.builderName == "ruined-rooms",
                  "final routed depth uses an existing city stand-in");
    runner.expect(final.stairsPosition == final.playerStart,
                  "final routed depth does not place a deeper exit");
}

} // namespace rl::test
