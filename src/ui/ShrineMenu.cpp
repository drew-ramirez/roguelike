/**
 * **************************************************************************
 * @file     ShrineMenu.cpp
 * @brief    Handles shrine menu input and rendering
 * **************************************************************************
 */

#include "ui/ShrineMenu.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/services/ShrineRiteSystem.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int panelX = 14;
constexpr int panelY = 8;
constexpr int panelWidth = 52;
constexpr int panelHeight = 12;
constexpr std::array<std::string_view, 2> actions{"Make offering", "Leave"};

} // namespace

ShrineInputResult handleShrineInput(entt::registry& registry,
                                    GameResources& resources,
                                    const std::vector<InputEvent>& events)
{
    if (!resources.pendingShrine.has_value() ||
        !shrineAvailable(
            resources, registry, resources.pendingShrine->shrine)) {
        resources.pendingShrine.reset();
        return ShrineInputResult::Close;
    }

    resources.shrineSelection = std::clamp(
        resources.shrineSelection, 0, static_cast<int>(actions.size()) - 1);
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape) {
            resources.pendingShrine.reset();
            return ShrineInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && movement->y != 0) {
            resources.shrineSelection =
                std::clamp(resources.shrineSelection + movement->y,
                           0,
                           static_cast<int>(actions.size()) - 1);
            return ShrineInputResult::None;
        }

        if (event.key == Key::Enter) {
            if (resources.shrineSelection == 0) {
                return offerAtShrine(
                           registry, resources, resources.pendingShrine->shrine)
                           ? ShrineInputResult::TookTurn
                           : ShrineInputResult::None;
            }
            resources.pendingShrine.reset();
            return ShrineInputResult::Close;
        }
    }
    return ShrineInputResult::None;
}

void drawShrineMenu(ConsoleBuffer& console,
                    const entt::registry& registry,
                    const GameResources& resources)
{
    drawMenuFrame(console, panelX, panelY, panelWidth, panelHeight);
    std::string name = "Shrine";
    if (resources.pendingShrine.has_value() &&
        registry.valid(resources.pendingShrine->shrine) &&
        registry.all_of<Name>(resources.pendingShrine->shrine)) {
        name = registry.get<Name>(resources.pendingShrine->shrine).value;
    }

    console.print(panelX + 2, panelY + 1, name, Color::yellow());
    int row = panelY + 3;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const bool selected =
            static_cast<int>(index) == resources.shrineSelection;
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} +
                          std::string{actions[index]},
                      selected ? Color::yellow() : Color{200, 200, 200, 255});
        ++row;
    }
}

} // namespace rl
