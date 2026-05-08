#pragma once

#include "TestSupport.hpp"

namespace rl::test {

void runAssetTests(TestRunner& runner);
void runDescentAndSpawnTests(TestRunner& runner);
void runDiceTests(TestRunner& runner);
void runEquipmentTests(TestRunner& runner);
void runInputTests(TestRunner& runner);
void runInventoryTests(TestRunner& runner);
void runLevelSnapshotTests(TestRunner& runner);
void runMapgenTests(TestRunner& runner);
void runPolishPressureTests(TestRunner& runner);
void runProgressionTests(TestRunner& runner);
void runRawsValidationTests(TestRunner& runner);
void runRegionRoutingTests(TestRunner& runner);
void runSaveLoadTests(TestRunner& runner);
void runTargetingTests(TestRunner& runner);
void runUiTests(TestRunner& runner);

} // namespace rl::test
