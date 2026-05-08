/**
 * **************************************************************************
 * @file     Dice.cpp
 * @brief    Parses and rolls compact dice expressions
 * **************************************************************************
 */

#include "core/Dice.hpp"

#include <charconv>
#include <cctype>
#include <stdexcept>

#include "core/Rng.hpp"

namespace rl {
namespace {

[[nodiscard]] bool isSpace(char value)
{
    return std::isspace(static_cast<unsigned char>(value)) != 0;
}

[[nodiscard]] std::string_view trim(std::string_view text)
{
    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] std::optional<int> parseInt(std::string_view text)
{
    text = trim(text);
    if (text.empty()) {
        return std::nullopt;
    }

    int value = 0;
    const auto* const begin = text.data();
    const auto* const end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] bool isDiceSeparator(char value)
{
    return value == 'd' || value == 'D';
}

[[nodiscard]] std::optional<std::size_t>
modifierSignPosition(std::string_view text)
{
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '+' || text[index] == '-') {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<DiceExpression> tryParseDice(std::string_view text)
{
    text = trim(text);
    if (text.empty()) {
        return std::nullopt;
    }

    const auto separator = text.find_first_of("dD");
    if (separator == std::string_view::npos) {
        return std::nullopt;
    }

    const auto countText = trim(text.substr(0, separator));
    const auto count = countText.empty() ? std::optional<int>{1}
                                         : parseInt(countText);
    if (!count.has_value() || *count <= 0) {
        return std::nullopt;
    }

    auto remainder = trim(text.substr(separator + 1));
    if (remainder.empty() || isDiceSeparator(remainder.front())) {
        return std::nullopt;
    }

    const auto sign = modifierSignPosition(remainder);
    const auto sidesText =
        sign.has_value() ? remainder.substr(0, *sign) : remainder;
    const auto sides = parseInt(sidesText);
    if (!sides.has_value() || *sides <= 0) {
        return std::nullopt;
    }

    int modifier = 0;
    if (sign.has_value()) {
        const char signChar = remainder[*sign];
        const auto modifierText = remainder.substr(*sign + 1);
        const auto parsedModifier = parseInt(modifierText);
        if (!parsedModifier.has_value() || *parsedModifier < 0) {
            return std::nullopt;
        }
        modifier = signChar == '-' ? -*parsedModifier : *parsedModifier;
    }

    return DiceExpression{*count, *sides, modifier};
}

DiceExpression parseDice(std::string_view text)
{
    const auto dice = tryParseDice(text);
    if (!dice.has_value()) {
        throw std::invalid_argument("Invalid dice expression");
    }
    return *dice;
}

int rollDice(const DiceExpression& dice, Rng& rng)
{
    if (dice.count <= 0 || dice.sides <= 0) {
        throw std::invalid_argument("Dice rolls and sides must be positive");
    }

    int total = dice.modifier;
    for (int roll = 0; roll < dice.count; ++roll) {
        total += rng.rangeInclusive(1, dice.sides);
    }
    return total;
}

} // namespace rl
