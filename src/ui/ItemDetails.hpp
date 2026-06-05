/**
 * **************************************************************************
 * @file     ItemDetails.hpp
 * @brief    Declares item metadata formatting and detail-pane rendering
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "render/Color.hpp"

namespace rl {

class ConsoleBuffer;

[[nodiscard]] std::string displayItemName(const entt::registry& registry,
                                          entt::entity item);

[[nodiscard]] std::vector<std::string>
itemDetailLines(const entt::registry& registry, entt::entity item);

void drawWrappedText(ConsoleBuffer& console,
                     int x,
                     int y,
                     int width,
                     int height,
                     const std::vector<std::string>& lines,
                     Color color = Color{200, 200, 200, 255});

void drawItemDetailsPane(ConsoleBuffer& console,
                         const entt::registry& registry,
                         entt::entity item,
                         int x,
                         int y,
                         int width,
                         int height);

} // namespace rl
