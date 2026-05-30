/**
 * **************************************************************************
 * @file     ConsoleLayer.hpp
 * @brief    Declares logical console layers before final buffer flattening
 * **************************************************************************
 */

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace rl {

enum class ConsoleLayer {
    Map = 0,
    Entity = 1,
    Entities = Entity,
    Effects = 2,
    Ui = 3,
    Log = 4,
    Modal = 5
};

inline constexpr std::array<ConsoleLayer, 6> consoleLayerStack{
    ConsoleLayer::Map,
    ConsoleLayer::Entity,
    ConsoleLayer::Effects,
    ConsoleLayer::Ui,
    ConsoleLayer::Log,
    ConsoleLayer::Modal};

[[nodiscard]] constexpr std::size_t
consoleLayerIndex(ConsoleLayer layer) noexcept
{
    return static_cast<std::size_t>(layer);
}

[[nodiscard]] constexpr std::string_view
consoleLayerName(ConsoleLayer layer) noexcept
{
    switch (layer) {
    case ConsoleLayer::Map:
        return "map";
    case ConsoleLayer::Entity:
        return "entity";
    case ConsoleLayer::Effects:
        return "effects";
    case ConsoleLayer::Ui:
        return "ui";
    case ConsoleLayer::Log:
        return "log";
    case ConsoleLayer::Modal:
        return "modal";
    }
    return "unknown";
}

} // namespace rl
