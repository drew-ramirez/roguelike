#pragma once

#include "TestSupport.hpp"

namespace rl::test {

void runAiTests(TestRunner& runner);
void runAssetTests(TestRunner& runner);
void runDescentAndSpawnTests(TestRunner& runner);
void runDiceTests(TestRunner& runner);
void runDragonTests(TestRunner& runner);
void runEffectsTests(TestRunner& runner);
void runEquipmentTests(TestRunner& runner);
void runFactionTests(TestRunner& runner);
void runInputTests(TestRunner& runner);
void runInventoryTests(TestRunner& runner);
void runLevelSnapshotTests(TestRunner& runner);
void runLogTests(TestRunner& runner);
void runMapgenTests(TestRunner& runner);
void runPolishPressureTests(TestRunner& runner);
void runPortalTests(TestRunner& runner);
void runProgressionTests(TestRunner& runner);
void runRangedCombatTests(TestRunner& runner);
void runRawsValidationTests(TestRunner& runner);
void runRegionRoutingTests(TestRunner& runner);
void runSaveLoadTests(TestRunner& runner);
void runSchedulerTests(TestRunner& runner);
void runSpellTests(TestRunner& runner);
void runSpatialTests(TestRunner& runner);
void runTargetingTests(TestRunner& runner);
void runUiTests(TestRunner& runner);
void runVendorTests(TestRunner& runner);
void runWorldGenerationTests(TestRunner& runner);
void runWorldSurvivalTests(TestRunner& runner);

} // namespace rl::test
