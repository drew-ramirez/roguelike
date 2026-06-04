#include "ui/VendorMenu.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "render/ConsoleBuffer.hpp"
#include "systems/InputSystem.hpp"
#include "systems/ItemSystem.hpp"
#include "systems/VendorSystem.hpp"

namespace rl {
namespace {

constexpr int panelX = 12;
constexpr int panelY = 6;
constexpr int panelWidth = 56;
constexpr int panelHeight = 28;

[[nodiscard]] std::string
itemLabel(const entt::registry& registry, entt::entity item, int price)
{
    std::string label = registry.all_of<Name>(item)
                            ? registry.get<Name>(item).value
                            : "Unnamed item";
    if (registry.all_of<MagicItem>(item)) {
        const auto& magic = registry.get<MagicItem>(item);
        if (!magic.identified && !magic.unidentifiedName.empty()) {
            label = magic.unidentifiedName;
        }
    }
    return label + " - " + std::to_string(price) + "g";
}

[[nodiscard]] std::vector<entt::entity> saleItems(
    const entt::registry& registry, entt::entity player, entt::entity vendor)
{
    std::vector<entt::entity> items;
    for (const auto item : inventoryItems(registry, player)) {
        if (vendorTradesItem(registry, vendor, item) &&
            !registry.all_of<Equipped>(item)) {
            items.push_back(item);
        }
    }
    return items;
}

[[nodiscard]] std::vector<entt::entity>
menuItems(const entt::registry& registry, const GameResources& resources)
{
    if (!resources.pendingVendor.has_value()) {
        return {};
    }
    const auto vendor = resources.pendingVendor->vendor;
    if (resources.pendingVendor->mode == VendorMode::Buy) {
        return vendorStock(registry, vendor);
    }
    return saleItems(registry, resources.player, vendor);
}

void drawBox(ConsoleBuffer& console)
{
    for (int y = panelY; y < panelY + panelHeight; ++y) {
        for (int x = panelX; x < panelX + panelWidth; ++x) {
            const bool border = x == panelX || x == panelX + panelWidth - 1 ||
                                y == panelY || y == panelY + panelHeight - 1;
            console.set(x,
                        y,
                        border ? U'#' : U' ',
                        Color{180, 180, 180, 255},
                        Color::black());
        }
    }
}

} // namespace

VendorInputResult handleVendorInput(entt::registry& registry,
                                    GameResources& resources,
                                    const std::vector<InputEvent>& events)
{
    if (!resources.pendingVendor.has_value() ||
        !registry.valid(resources.pendingVendor->vendor)) {
        resources.pendingVendor.reset();
        return VendorInputResult::Close;
    }

    auto items = menuItems(registry, resources);
    if (items.empty()) {
        resources.vendorSelection = 0;
    } else {
        resources.vendorSelection = std::clamp(
            resources.vendorSelection, 0, static_cast<int>(items.size()) - 1);
    }

    for (const auto& event : events) {
        if (event.type != InputEventType::KeyDown) {
            continue;
        }
        if (event.key == Key::Escape || event.key == Key::V) {
            resources.pendingVendor.reset();
            return VendorInputResult::Close;
        }
        if (event.key == Key::B) {
            resources.pendingVendor->mode = VendorMode::Buy;
            resources.vendorSelection = 0;
            return VendorInputResult::None;
        }
        if (event.key == Key::S) {
            resources.pendingVendor->mode = VendorMode::Sell;
            resources.vendorSelection = 0;
            return VendorInputResult::None;
        }

        const auto movement = movementDeltaForKey(event.key);
        if (movement.has_value() && !items.empty() && movement->y != 0) {
            resources.vendorSelection =
                std::clamp(resources.vendorSelection + movement->y,
                           0,
                           static_cast<int>(items.size()) - 1);
            return VendorInputResult::None;
        }

        if (event.key == Key::Enter && !items.empty()) {
            const auto item =
                items[static_cast<std::size_t>(resources.vendorSelection)];
            const auto vendor = resources.pendingVendor->vendor;
            const bool traded =
                resources.pendingVendor->mode == VendorMode::Buy
                    ? buyItemFromVendor(
                          registry, resources, resources.player, vendor, item)
                    : sellItemToVendor(
                          registry, resources, resources.player, vendor, item);
            return traded ? VendorInputResult::TookTurn
                          : VendorInputResult::None;
        }
    }
    return VendorInputResult::None;
}

void drawVendorMenu(ConsoleBuffer& console,
                    const entt::registry& registry,
                    const GameResources& resources)
{
    drawBox(console);
    if (!resources.pendingVendor.has_value()) {
        return;
    }

    const auto vendor = resources.pendingVendor->vendor;
    const std::string vendorName =
        registry.valid(vendor) && registry.all_of<Name>(vendor)
            ? registry.get<Name>(vendor).value
            : "Vendor";
    const bool buying = resources.pendingVendor->mode == VendorMode::Buy;
    console.print(panelX + 2,
                  panelY + 1,
                  vendorName + (buying ? " - Buy" : " - Sell"),
                  Color::yellow());
    console.print(panelX + 2,
                  panelY + 2,
                  "B buy  S sell  Enter trade  Esc close",
                  Color{140, 140, 140, 255});

    const auto items = menuItems(registry, resources);
    if (items.empty()) {
        console.print(panelX + 2,
                      panelY + 4,
                      buying ? "No stock available." : "Nothing to sell.",
                      Color{170, 170, 170, 255});
        return;
    }

    int row = panelY + 4;
    for (std::size_t index = 0;
         index < items.size() && row < panelY + panelHeight - 2;
         ++index) {
        const bool selected =
            static_cast<int>(index) == resources.vendorSelection;
        const Color color =
            selected ? Color::yellow() : Color{200, 200, 200, 255};
        const int price = buying ? itemBuyPrice(registry, items[index])
                                 : itemSellPrice(registry, items[index]);
        console.print(panelX + 2,
                      row,
                      std::string{selected ? "> " : "  "} +
                          itemLabel(registry, items[index], price),
                      color);
        ++row;
    }
}

} // namespace rl
