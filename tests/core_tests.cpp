#include "TestSuites.hpp"

int main()
{
    rl::test::TestRunner runner;
    rl::test::runInputTests(runner);
    rl::test::runInventoryTests(runner);
    rl::test::runTargetingTests(runner);
    rl::test::runEquipmentTests(runner);
    rl::test::runSaveLoadTests(runner);
    rl::test::runDescentAndSpawnTests(runner);
    return runner.failures == 0 ? 0 : 1;
}
