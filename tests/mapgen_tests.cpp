#include "TestSuites.hpp"

#include <sstream>
#include <string>

#include "core/Rng.hpp"
#include "debug/MapgenViewer.hpp"
#include "map/MapValidation.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"

namespace rl::test {

void runMapgenTests(TestRunner& runner)
{
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
    }

    {
        runner.expect(isKnownMapBuilder("simple") && isKnownMapBuilder("bsp") &&
                          isKnownMapBuilder("bsp-interior"),
                      "mapgen viewer exposes all implemented builders");
        std::ostringstream output;
        const int result = runMapgenViewer(
            MapgenViewerOptions{"bsp", Rng::Seed{777U}, false}, output);
        const std::string text = output.str();
        runner.expect(result == 0, "mapgen viewer runs selected builder");
        runner.expect(text.find("Builder: bsp") != std::string::npos,
                      "mapgen viewer prints builder id");
        runner.expect(text.find("Validation: ok") != std::string::npos,
                      "mapgen viewer prints validation result");
    }
}

} // namespace rl::test
