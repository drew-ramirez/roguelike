#include "TestSuites.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
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
           left.builderId == right.builderId && left.glyph == right.glyph &&
           left.localSeed == right.localSeed &&
           left.dangerTier == right.dangerTier &&
           left.baseSafety == right.baseSafety &&
           left.discovered == right.discovered &&
           left.visited == right.visited && left.regionId == right.regionId &&
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
        runner.expect(first.cells.size() == static_cast<std::size_t>(
                                                worldMapWidth * worldMapHeight),
                      "world generation fills the macro grid");
        runner.expect(first.cells.size() == second.cells.size(),
                      "same-seed worlds have matching cell counts");

        bool same = first.cells.size() == second.cells.size();
        for (std::size_t index = 0; same && index < first.cells.size();
             ++index) {
            same = same &&
                   sameCellIdentity(first.cells[index], second.cells[index]);
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
                          world.cursor == greenholtWorldPosition() &&
                          world.currentRegionId == "greenholt_hearth",
                      "world starts the player cursor and region at Greenholt");
        runner.expect(greenholt->name == "Greenholt" &&
                          greenholt->biome == BiomeVariant::DryWoods &&
                          greenholt->discovered && greenholt->visited &&
                          greenholt->regionId == "greenholt_hearth" &&
                          greenholt->landmark.has_value() &&
                          greenholt->landmark->id == "greenholt" &&
                          greenholt->landmark->regionId == "greenholt_hearth" &&
                          !greenholt->landmark->fastTravelUnlocked,
                      "Greenholt anchor starts discovered but not unlocked");
        runner.expect(reedfen->name == "Reedfen" &&
                          reedfen->biome == BiomeVariant::ReedMarsh &&
                          reedfen->regionId == "reedfen_ford" &&
                          reedfen->landmark.has_value() && !reedfen->discovered,
                      "Reedfen anchor keeps its marsh identity undiscovered");
        runner.expect(limestone->name == "Limestone" &&
                          limestone->biome == BiomeVariant::LimestoneRise &&
                          limestone->regionId == "limestone_sink" &&
                          limestone->landmark.has_value() &&
                          !limestone->discovered,
                      "Limestone anchor keeps its rise identity undiscovered");
    }

    {
        auto world = generateWorldMap(Rng::Seed{99U});
        runner.expect(world.regions.size() == world.cells.size(),
                      "world generation creates one region per macro cell");

        auto* greenholt = worldRegionById(world, "greenholt_hearth");
        runner.expect(greenholt != nullptr && greenholt->hasSettlement &&
                          greenholt->hasCamp && greenholt->fastTravelLandmark &&
                          greenholt->visited,
                      "Greenholt region exposes settlement and camp flags");

        const auto* reedfenRoute =
            greenholt == nullptr
                ? nullptr
                : findDiscoveredRoute(*greenholt, "reedfen_ford");
        runner.expect(reedfenRoute != nullptr &&
                          reedfenRoute->name == "Reedfen Ford" &&
                          reedfenRoute->routeTag == "marsh" &&
                          reedfenRoute->hungerCost == 35,
                      "Greenholt exposes its discovered Reedfen route");
        runner.expect(greenholt != nullptr &&
                          discoveredRoutes(*greenholt).size() == 1,
                      "discovered route filtering returns known links only");

        world.currentMacro = reedfenWorldPosition();
        world.currentScreen = Point{1, 1};
        updateCurrentWorldRegion(world);
        const auto* current = currentWorldRegion(world);
        const auto* reedfen = worldCellAt(world, reedfenWorldPosition());
        runner.expect(current != nullptr && current->id == "reedfen_ford" &&
                          current->visited && reedfen != nullptr &&
                          reedfen->visited && reedfen->discovered,
                      "moving onto a landmark cell reveals its current region");
        runner.expect(reedfen != nullptr && reedfen->landmark.has_value() &&
                          !reedfen->landmark->fastTravelUnlocked,
                      "landmark discovery does not unlock fast travel alone");
        runner.expect(reedfen != nullptr &&
                          unlockCurrentLandmarkFastTravel(world) &&
                          reedfen->landmark.has_value() &&
                          reedfen->landmark->fastTravelUnlocked,
                      "entering a landmark screen can unlock fast travel");
    }

    {
        runner.expect(deriveBiomeVariant(
                          WorldCellFields{0.3, 0.4, 0.2, 0.2, 0.2, 0.82}) ==
                          BiomeVariant::DangerousWilds,
                      "high danger pressure derives dangerous wilds");
        runner.expect(deriveBiomeVariant(
                          WorldCellFields{0.3, 0.4, 0.5, 0.2, 0.74, 0.2}) ==
                          BiomeVariant::RuinedWoodland,
                      "high ruin density derives ruined woodland");
        runner.expect(deriveBiomeVariant(
                          WorldCellFields{0.82, 0.4, 0.2, 0.2, 0.2, 0.2}) ==
                          BiomeVariant::LimestoneRise,
                      "high elevation derives limestone rise");
        runner.expect(deriveBiomeVariant(
                          WorldCellFields{0.2, 0.8, 0.2, 0.72, 0.2, 0.2}) ==
                          BiomeVariant::Sinklands,
                      "low wet marsh pressure derives sinklands");
        runner.expect(deriveBiomeVariant(
                          WorldCellFields{0.4, 0.8, 0.2, 0.72, 0.2, 0.2}) ==
                          BiomeVariant::ReedMarsh,
                      "wet marsh pressure derives reed marsh");
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

        const std::array roles{LocalScreenRole::CenterLandmark,
                               LocalScreenRole::CardinalTransition,
                               LocalScreenRole::CornerWilderness,
                               LocalScreenRole::RuinPocket,
                               LocalScreenRole::CaveMouth,
                               LocalScreenRole::CampShrine,
                               LocalScreenRole::ResourceHeavy,
                               LocalScreenRole::TownOutskirts,
                               LocalScreenRole::DangerEncounter};
        for (const auto role : roles) {
            runner.expect(localScreenRoleFromId(localScreenRoleId(role)) ==
                              role,
                          "local screen role id round-trips");
        }
    }

    {
        const auto world = generateWorldMap(Rng::Seed{42U});
        const auto center =
            describeLocalScreen(world, greenholtWorldPosition(), Point{1, 1});
        runner.expect(center.role == LocalScreenRole::CenterLandmark &&
                          center.builderId == "town" &&
                          center.landmarkId == "greenholt",
                      "Greenholt center screen resolves to the town builder");

        const auto outskirts =
            describeLocalScreen(world, greenholtWorldPosition(), Point{1, 0});
        runner.expect(outskirts.role == LocalScreenRole::TownOutskirts &&
                          outskirts.builderId == "surface-town-outskirts" &&
                          (outskirts.exitMask & exitNorth) != 0,
                      "Greenholt edge screens resolve to town outskirts");

        const auto danger = describeLocalScreen(
            world, greenholtWorldPosition(), Point{2, 1}, true);
        runner.expect(danger.role == LocalScreenRole::DangerEncounter &&
                          danger.builderId == "surface-danger-encounter" &&
                          danger.dangerEncounter,
                      "forced danger screens use the danger encounter role");

        bool threw = false;
        try {
            (void)describeLocalScreen(world, Point{-1, 0}, Point{1, 1});
        } catch (const std::out_of_range&) {
            threw = true;
        }
        runner.expect(threw, "invalid macro coordinates reject screen lookup");
    }
}

} // namespace rl::test
