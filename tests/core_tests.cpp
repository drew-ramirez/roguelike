#include "TestSuites.hpp"

int main()
{
    rl::test::TestRunner runner;
    rl::test::runAssetTests(runner);
    rl::test::runAiTests(runner);
    rl::test::runInputTests(runner);
    rl::test::runInventoryTests(runner);
    rl::test::runEffectsTests(runner);
    rl::test::runTargetingTests(runner);
    rl::test::runUiTests(runner);
    rl::test::runDiceTests(runner);
    rl::test::runEquipmentTests(runner);
    rl::test::runLevelSnapshotTests(runner);
    rl::test::runLogTests(runner);
    rl::test::runSaveLoadTests(runner);
    rl::test::runSpatialTests(runner);
    rl::test::runVendorTests(runner);
    rl::test::runDescentAndSpawnTests(runner);
    rl::test::runRawsValidationTests(runner);
    rl::test::runRegionRoutingTests(runner);
    rl::test::runPolishPressureTests(runner);
    rl::test::runProgressionTests(runner);
    rl::test::runMapgenTests(runner);
    return runner.failures == 0 ? 0 : 1;
}
