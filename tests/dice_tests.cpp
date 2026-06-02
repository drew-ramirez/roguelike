#include "TestSuites.hpp"

#include <stdexcept>

#include "core/Dice.hpp"
#include "core/Rng.hpp"

namespace rl::test {

void runDiceTests(TestRunner& runner)
{
    {
        const auto dice = parseDice("1d6+2");
        runner.expect(dice.count == 1 && dice.sides == 6 && dice.modifier == 2,
                      "dice parser reads positive modifiers");
    }
    {
        const auto dice = parseDice("3D4 - 1");
        runner.expect(
            dice.count == 3 && dice.sides == 4 && dice.modifier == -1,
            "dice parser reads uppercase dice and negative modifiers");
    }
    {
        const auto dice = parseDice("d8");
        runner.expect(dice.count == 1 && dice.sides == 8,
                      "dice parser defaults missing count to one");
    }
    {
        runner.expect(!tryParseDice("").has_value(),
                      "dice parser rejects empty expressions");
        runner.expect(!tryParseDice("2d0").has_value(),
                      "dice parser rejects zero-sided dice");
        runner.expect(!tryParseDice("2dd6").has_value(),
                      "dice parser rejects malformed dice separators");
    }
    {
        bool threw = false;
        try {
            (void)parseDice("bad");
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        runner.expect(threw, "strict dice parser throws on invalid input");
    }
    {
        Rng rng{42U};
        runner.expect(rollDice(parseDice("3d1+2"), rng) == 5,
                      "dice roller applies positive modifiers");
        runner.expect(rollDice(parseDice("2d1-3"), rng) == -1,
                      "dice roller applies negative modifiers");
    }
}

} // namespace rl::test
