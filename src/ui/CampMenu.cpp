/**
 * **************************************************************************
 * @file     CampMenu.cpp
 * @brief    Handles camp action menu input and rendering
 * **************************************************************************
 */

#include "ui/CampMenu.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/CampSystem.hpp"
#include "systems/CookingSystem.hpp"
#include "systems/ForageSystem.hpp"
#include "systems/InputSystem.hpp"
#include "ui/Widgets.hpp"

namespace rl {
namespace {

constexpr int panelX = 14;
constexpr int panelY = 8;
constexpr int panelWidth = 52;
constexpr int panelHeight = 16;
constexpr std::array<std::string_view, 4> actions{
    "Rest", "Forage nearby", "Cook meal", "Leave"};

} // namespace

CampInputResult handleCampInput(entt::registry& registry,
                                GameResources& resources,
                                const std::vector<InputEvent>& events)
{
    if (!resources.pendingCamp.has_value() ||
        !registry.valid(resources.pendingCamp->camp)) {
        resources.pendingCamp.reset();
        return CampInputResult::Close;
    }

    resources.campSelection = std::clamp(
        resources.campSelection, 0, static_cast<int>(actions.size()) - 1);
    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape) {
            resources.pendingCamp.reset();
            return CampInputResult::Close;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && movement->y != 0) {
            resources.campSelection =
                std::clamp(resources.campSelection + movement->y,
                           0,
                           static_cast<int>(actions.size()) - 1);
            return CampInputResult::None;
        }

        if (event.key == Key::Enter) {
            const auto camp = resources.pendingCamp->camp;
            if (resources.campSelection == 0) {
                return restAtCamp(registry, resources, camp)
                           ? CampInputResult::TookTurn
                           : CampInputResult::None;
            }
            if (resources.campSelection == 1) {
                return forageCurrentRegion(registry, resources)
                           ? CampInputResult::TookTurn
                           : CampInputResult::None;
            }
            if (resources.campSelection == 2) {
                return cookAtCamp(registry, resources, camp)
                           ? CampInputResult::TookTurn
                           : CampInputResult::None;
            }
            resources.pendingCamp.reset();
            return CampInputResult::Close;
        }
    }

    return CampInputResult::None;
}

void drawCampMenu(ConsoleBuffer& console,
                  const entt::registry& registry,
                  const GameResources& resources)
{
    drawMenuFrame(console, panelX, panelY, panelWidth, panelHeight);
    std::string name = "Camp";
    if (resources.pendingCamp.has_value() &&
        registry.valid(resources.pendingCamp->camp) &&
        registry.all_of<Name>(resources.pendingCamp->camp)) {
        name = registry.get<Name>(resources.pendingCamp->camp).value;
    }

    console.print(panelX + 2, panelY + 1, name, Color::yellow());
    int row = panelY + 3;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const bool selected =
            static_cast<int>(index) == resources.campSelection;
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} +
                          std::string{actions[index]},
                      selected ? Color::yellow() : Color{200, 200, 200, 255});
        ++row;
    }
}

} // namespace rl
