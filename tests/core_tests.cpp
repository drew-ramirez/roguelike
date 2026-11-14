#include "TestSuites.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void runSuite(rl::test::TestRunner& runner,
              std::string_view name,
              void (*suite)(rl::test::TestRunner&))
{
    try {
        suite(runner);
    } catch (const std::exception& error) {
        ++runner.failures;
        std::cerr << "FAIL: " << name << " threw " << error.what() << '\n';
    } catch (...) {
        ++runner.failures;
        std::cerr << "FAIL: " << name << " threw an unknown exception\n";
    }
}

} // namespace

int main()
{
    rl::test::TestRunner runner;
    runSuite(runner, "asset_tests", rl::test::runAssetTests);
    runSuite(runner, "ai_tests", rl::test::runAiTests);
    runSuite(runner, "input_tests", rl::test::runInputTests);
    runSuite(runner, "inventory_tests", rl::test::runInventoryTests);
    runSuite(runner, "item_details_tests", rl::test::runItemDetailsTests);
    runSuite(runner, "effects_tests", rl::test::runEffectsTests);
    runSuite(runner, "spell_tests", rl::test::runSpellTests);
    runSuite(runner, "ranged_combat_tests", rl::test::runRangedCombatTests);
    runSuite(runner, "dragon_tests", rl::test::runDragonTests);
    runSuite(runner, "faction_tests", rl::test::runFactionTests);
    runSuite(runner, "portal_tests", rl::test::runPortalTests);
    runSuite(runner, "log_tests", rl::test::runLogTests);
    runSuite(runner, "turn_tests", rl::test::runTurnTests);
    runSuite(runner, "targeting_tests", rl::test::runTargetingTests);
    runSuite(runner, "ui_tests", rl::test::runUiTests);
    runSuite(runner, "dice_tests", rl::test::runDiceTests);
    runSuite(runner, "equipment_tests", rl::test::runEquipmentTests);
    runSuite(runner, "level_snapshot_tests", rl::test::runLevelSnapshotTests);
    runSuite(runner, "save_load_tests", rl::test::runSaveLoadTests);
    runSuite(runner, "spatial_tests", rl::test::runSpatialTests);
    runSuite(runner, "vendor_tests", rl::test::runVendorTests);
    runSuite(runner, "descent_spawn_tests", rl::test::runDescentAndSpawnTests);
    runSuite(runner, "raws_validation_tests", rl::test::runRawsValidationTests);
    runSuite(runner, "region_routing_tests", rl::test::runRegionRoutingTests);
    runSuite(runner, "polish_pressure_tests", rl::test::runPolishPressureTests);
    runSuite(runner, "progression_tests", rl::test::runProgressionTests);
    runSuite(
        runner, "world_generation_tests", rl::test::runWorldGenerationTests);
    runSuite(runner, "world_state_tests", rl::test::runWorldStateTests);
    runSuite(runner, "world_survival_tests", rl::test::runWorldSurvivalTests);
    runSuite(runner, "mapgen_tests", rl::test::runMapgenTests);
    return runner.failures == 0 ? 0 : 1;
}
