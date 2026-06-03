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
    return std::any_of(
        result.buildSteps.begin(),
        result.buildSteps.end(),
        [step](const std::string& candidate) { return candidate == step; });
}

[[nodiscard]] bool hasSpawnRequest(const MapBuildResult& result,
                                   std::string_view id)
{
    return std::any_of(
        result.spawnRequests.begin(),
        result.spawnRequests.end(),
        [id](const SpawnRequest& request) { return request.id == id; });
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
    runner.expect(routeForDepth(4).region == DungeonRegion::DeepCaverns &&
                      routeForDepth(4).builderId == "deep-caverns",
                  "depth 4 routes to deep caverns");
    runner.expect(routeForDepth(5).region ==
                          DungeonRegion::DwarfFortTransition &&
                      routeForDepth(5).builderId == "dwarf-transition",
                  "depth 5 routes to dwarf transition");
    runner.expect(routeForDepth(6).region == DungeonRegion::DragonLair &&
                      routeForDepth(6).builderId == "dragon-lair",
                  "depth 6 routes to dragon fort");
    runner.expect(routeForDepth(7).region == DungeonRegion::MushroomEntrance,
                  "depth 7 routes to mushroom entrance");
    runner.expect(routeForDepth(8).region == DungeonRegion::MushroomForest,
                  "depth 8 routes to mushroom forest");
    runner.expect(routeForDepth(9).region == DungeonRegion::MushroomExit,
                  "depth 9 routes to mushroom exit");
    runner.expect(routeForDepth(10).region == DungeonRegion::DarkElfCity,
                  "depth 10 routes to dark elf city");

    Rng townRng{99U};
    const auto town = buildLevelForDepth(1, 40, 25, townRng, {});
    runner.expect(town.builderName == "town", "depth 1 builds town");
    runner.expect(hasBuildStep(town, "region:town"),
                  "town build records its region route");
    runner.expect(
        town.map.tileAt(town.stairsPosition.x, town.stairsPosition.y) ==
            TileType::DownStairs,
        "town has a down exit");

    Rng woodsRng{100U};
    const auto woods = buildLevelForDepth(2, 40, 25, woodsRng, {});
    runner.expect(woods.builderName == "woods", "depth 2 builds woods");
    runner.expect(!woods.spawnRegions.empty(), "woods provides spawn regions");
    runner.expect(hasSpawnRequest(woods, "wolf") &&
                      hasSpawnRequest(woods, "wild_boar") &&
                      hasSpawnRequest(woods, "food_ration") &&
                      hasSpawnRequest(woods, "healing_potion"),
                  "woods requests wildlife and supplies");
    runner.expect(hasBuildStep(woods, "content: woods wildlife and supplies"),
                  "woods build records authored content");

    Rng cavernRng{101U};
    const auto cavern = buildLevelForDepth(3, 40, 25, cavernRng, {});
    runner.expect(cavern.builderName == "limestone-caverns",
                  "limestone depths use the themed cavern builder");
    runner.expect(hasBuildStep(cavern, "region:limestone"),
                  "limestone build records its region route");
    runner.expect(hasBuildStep(cavern, "theme: limestone caverns"),
                  "limestone build records its theme");
    runner.expect(cavern.spawnRegions.size() >= 3,
                  "limestone caverns expose multiple spawn regions");
    runner.expect(hasSpawnRequest(cavern, "kobold") &&
                      hasSpawnRequest(cavern, "cave_bat") &&
                      hasSpawnRequest(cavern, "limestone_column"),
                  "limestone caverns request themed encounters");
    runner.expect(
        cavern.map.tileAt(cavern.playerStart.x, cavern.playerStart.y) ==
                TileType::UpStairs &&
            cavern.map.tileAt(cavern.stairsPosition.x,
                              cavern.stairsPosition.y) == TileType::DownStairs,
        "limestone caverns connect backtracking exits");

    Rng deepRng{102U};
    const auto deep = buildLevelForDepth(4, 40, 25, deepRng, {});
    runner.expect(deep.builderName == "deep-caverns",
                  "depth 4 builds deep caverns");
    runner.expect(hasBuildStep(deep, "region:deep_caverns") &&
                      hasBuildStep(deep, "theme: deep caverns"),
                  "deep caverns record route and theme");
    runner.expect(hasSpawnRequest(deep, "deep_kobold") &&
                      hasSpawnRequest(deep, "stone_golem") &&
                      hasSpawnRequest(deep, "deep_limestone_column") &&
                      hasSpawnRequest(deep, "town_portal_scroll") &&
                      hasSpawnRequest(deep, "food_ration"),
                  "deep caverns request themed encounters and supplies");

    Rng dwarfRng{103U};
    const auto dwarf = buildLevelForDepth(5, 40, 25, dwarfRng, {});
    runner.expect(dwarf.builderName == "dwarf-transition",
                  "depth 5 builds the dwarf transition");
    runner.expect(hasSpawnRequest(dwarf, "dwarf_sentry") &&
                      hasSpawnRequest(dwarf, "dwarf_rubble") &&
                      hasSpawnRequest(dwarf, "food_ration"),
                  "dwarf transition requests fortress content");

    Rng dragonRng{104U};
    const auto dragon = buildLevelForDepth(6, 40, 25, dragonRng, {});
    runner.expect(dragon.builderName == "dragon-lair",
                  "depth 6 builds the dragon fort");
    runner.expect(hasSpawnRequest(dragon, "dragon") &&
                      hasSpawnRequest(dragon, "gauntlets_of_might") &&
                      hasSpawnRequest(dragon, "food_ration"),
                  "dragon fort requests boss and treasure spawns");

    Rng entranceRng{105U};
    const auto entrance = buildLevelForDepth(7, 40, 25, entranceRng, {});
    runner.expect(entrance.builderName == "mushroom-entrance",
                  "depth 7 builds the mushroom entrance");
    runner.expect(hasSpawnRequest(entrance, "mushroom_patch") &&
                      hasSpawnRequest(entrance, "sporeling") &&
                      hasSpawnRequest(entrance, "food_ration"),
                  "mushroom entrance requests first fungal content");

    Rng mushroomRng{106U};
    const auto mushroom = buildLevelForDepth(8, 40, 25, mushroomRng, {});
    runner.expect(mushroom.builderName == "mushroom-forest",
                  "depth 8 builds the mushroom forest");
    runner.expect(hasSpawnRequest(mushroom, "sporeling") &&
                      hasSpawnRequest(mushroom, "fungal_giant") &&
                      hasSpawnRequest(mushroom, "confusion_spore") &&
                      hasSpawnRequest(mushroom, "food_ration"),
                  "mushroom forest requests fungal encounters");

    Rng exitRng{107U};
    const auto exit = buildLevelForDepth(9, 40, 25, exitRng, {});
    runner.expect(exit.builderName == "mushroom-exit",
                  "depth 9 builds the mushroom exit");
    runner.expect(hasSpawnRequest(exit, "dark_elf_scout_a") &&
                      hasSpawnRequest(exit, "hand_crossbow") &&
                      hasSpawnRequest(exit, "food_ration"),
                  "mushroom exit requests dark elf foreshadowing");

    Rng cityRng{108U};
    const auto city = buildLevelForDepth(10, 40, 25, cityRng, {});
    runner.expect(city.builderName == "dark-elf-city",
                  "depth 10 builds the dark elf city");
    runner.expect(city.stairsPosition == city.playerStart,
                  "final city does not place a deeper exit");
    runner.expect(
        city.upStairsPosition == city.playerStart &&
            city.map.tileAt(city.upStairsPosition.x, city.upStairsPosition.y) ==
                TileType::UpStairs,
        "final city places an up exit at the arrival point");
    runner.expect(hasSpawnRequest(city, "dark_elf_marksman_a") &&
                      hasSpawnRequest(city, "dark_elf_blade_b"),
                  "dark elf city requests rival clan spawns");
}

} // namespace rl::test
