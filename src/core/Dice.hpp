/**
 * **************************************************************************
 * @file     Dice.hpp
 * @brief    Declares dice notation parsing and rolling helpers
 * **************************************************************************
 */

#pragma once

#include <optional>
#include <string_view>

namespace rl {

class Rng;

struct DiceExpression {
    int count = 1;
    int sides = 1;
    int modifier = 0;
};

[[nodiscard]] std::optional<DiceExpression>
tryParseDice(std::string_view text);

[[nodiscard]] DiceExpression parseDice(std::string_view text);

[[nodiscard]] int rollDice(const DiceExpression& dice, Rng& rng);

} // namespace rl
