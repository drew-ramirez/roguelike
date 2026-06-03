#include "TestSuites.hpp"

#include <array>
#include <cstddef>
#include <string_view>

#include "world/WorldGeneration.hpp"

namespace rl::test {
namespace {

[[nodiscard]] bool fieldsInRange(const WorldCellFields& fields) noexcept
{
    const std::array values{fields.elevation,
                            fields.moisture,
                            fields.woodlandDensity,
                            fields.marshPressure,
                            fields.ruinDensity,
                            fields.dangerPressure};
    for (const double value : values) {
        if (value < 0.0 || value > 1.0) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool sameCellIdentity(const WorldCellDescriptor& left,
                                    const WorldCellDescriptor& right)
{
    return left.position == right.position &&
           left.fields.elevation == right.fields.elevation &&
           left.fields.moisture == right.fields.moisture &&
           left.fields.woodlandDensity == right.fields.woodlandDensity &&
           left.fields.marshPressure == right.fields.marshPressure &&
           left.fields.ruinDensity == right.fields.ruinDensity &&
           left.fields.dangerPressure == right.fields.dangerPressure &&
           left.biome == right.biome && left.name == right.name &&
           left.glyph == right.glyph && left.discovered == right.discovered &&
           left.visited == right.visited &&
           left.landmark.has_value() == right.landmark.has_value();
}

[[nodiscard]] bool allFieldsInRange(const WorldMapState& world) noexcept
{
    for (const auto& cell : world.cells) {
        if (!fieldsInRange(cell.fields)) {
            return false;
        }
    }
    return true;
}

} // namespace

void runWorldGenerationTests(TestRunner& runner)
{
    {
        const auto first = generateWorldMap(Rng::Seed{1234U});
        const auto second = generateWorldMap(Rng::Seed{1234U});
        runner.expect(first.cells.size() ==
                          static_cast<std::size_t>(worldMapWidth *
                                                   worldMapHeight),
                      "world generation fills the macro grid");
        runner.expect(first.cells.size() == second.cells.size(),
                      "same-seed worlds have matching cell counts");

        bool same = true;
        for (std::size_t index = 0; index < first.cells.size(); ++index) {
            same = same && sameCellIdentity(first.cells[index],
                                            second.cells[index]);
        }
        runner.expect(same, "world generation is deterministic for a seed");
        runner.expect(allFieldsInRange(first),
                      "world noise fields stay in normalized ranges");
    }

    {
        auto world = generateWorldMap(Rng::Seed{77U});
        const auto* greenholt = worldCellAt(world, greenholtWorldPosition());
        const auto* reedfen = worldCellAt(world, reedfenWorldPosition());
        const auto* limestone = worldCellAt(world, limestoneWorldPosition());

        runner.expect(greenholt != nullptr && reedfen != nullptr &&
                          limestone != nullptr,
                      "anchor landmarks resolve to valid world cells");
        runner.expect(world.currentMacro == greenholtWorldPosition() &&
                          world.cursor == greenholtWorldPosition(),
                      "world starts the player cursor at Greenholt");
        runner.expect(greenholt->name == "Greenholt" &&
                          greenholt->biome == BiomeVariant::DryWoods &&
                          greenholt->discovered &&
                          greenholt->landmark.has_value() &&
                          greenholt->landmark->id == "greenholt" &&
                          !greenholt->landmark->fastTravelUnlocked,
                      "Greenholt anchor starts discovered but not unlocked");
        runner.expect(reedfen->name == "Reedfen" &&
                          reedfen->biome == BiomeVariant::ReedMarsh &&
                          reedfen->landmark.has_value() &&
                          !reedfen->discovered,
                      "Reedfen anchor keeps its marsh identity undiscovered");
        runner.expect(limestone->name == "Limestone" &&
                          limestone->biome == BiomeVariant::LimestoneRise &&
                          limestone->landmark.has_value() &&
                          !limestone->discovered,
                      "Limestone anchor keeps its rise identity undiscovered");
    }

    {
        const std::array biomes{BiomeVariant::DryWoods,
                                BiomeVariant::WetWoods,
                                BiomeVariant::DeepGrove,
                                BiomeVariant::ReedMarsh,
                                BiomeVariant::Sinklands,
                                BiomeVariant::LimestoneRise,
                                BiomeVariant::RuinedWoodland,
                                BiomeVariant::DangerousWilds};
        for (const auto biome : biomes) {
            const std::string_view id = biomeVariantId(biome);
            runner.expect(biomeVariantFromId(id) == biome,
                          "biome id round-trips");
            runner.expect(!biomeDisplayName(biome).empty() &&
                              biomeGlyph(biome) != U'.',
                          "biome exposes map metadata");
        }
    }
}

} // namespace rl::test