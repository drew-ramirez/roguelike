#include "TestSuites.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>

#include "core/Rng.hpp"
#include "debug/MapgenViewer.hpp"
#include "map/MapBrush.hpp"
#include "map/MapValidation.hpp"
#include "map/generators/CellularAutomataBuilder.hpp"
#include "map/generators/DrunkardsWalkBuilder.hpp"
#include "map/generators/BspDungeonBuilder.hpp"
#include "map/generators/BspInteriorBuilder.hpp"
#include "map/generators/SimpleMapBuilder.hpp"

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

} // namespace

void runMapgenTests(TestRunner& runner)
{
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
