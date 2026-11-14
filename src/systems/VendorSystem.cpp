/**
 * **************************************************************************
 * @file     VendorSystem.cpp
 * @brief    Applies vendor buy/sell transactions to ECS ownership and gold
 * **************************************************************************
 */

#include "systems/VendorSystem.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "ecs/EntityState.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/RawSpawner.hpp"

namespace rl {
namespace {

[[nodiscard]] bool hasBackpackOwner(const entt::registry& registry,
                                    entt::entity item,
                                    entt::entity owner)
{
    return registry.valid(item) && registry.all_of<InBackpack>(item) &&
           registry.get<InBackpack>(item).owner == owner;
}

[[nodiscard]] bool isEquippedBy(const entt::registry& registry,
                                entt::entity item,
                                entt::entity owner)
{
    return registry.valid(item) && registry.all_of<Equipped>(item) &&
           registry.get<Equipped>(item).owner == owner;
}

[[nodiscard]] bool hasGoldPool(const entt::registry& registry,
                               entt::entity entity)
{
    return registry.valid(entity) && registry.all_of<Pools>(entity);
}

[[nodiscard]] int availableGold(const entt::registry& registry,
                                entt::entity entity)
{
    if (registry.valid(entity) && registry.all_of<Vendor>(entity)) {
        return registry.get<Vendor>(entity).gold;
    }
    if (hasGoldPool(registry, entity)) {
        return registry.get<Pools>(entity).gold;
    }
    return 0;
}

[[nodiscard]] bool categoryMatches(std::string_view vendorCategory,
                                   std::string_view itemCategory)
{
    if (vendorCategory.empty() || vendorCategory == "all" ||
        vendorCategory == itemCategory) {
        return true;
    }
    return vendorCategory == "alchemy" &&
           (itemCategory == "potions" || itemCategory == "scrolls" ||
            itemCategory == "food" || itemCategory == "books");
}

void setGold(entt::registry& registry, entt::entity entity, int amount)
{
    if (registry.valid(entity) && registry.all_of<Pools>(entity)) {
        registry.get<Pools>(entity).gold = amount;
    }
    if (registry.valid(entity) && registry.all_of<Vendor>(entity)) {
        registry.get<Vendor>(entity).gold = amount;
    }
}

void transferGold(entt::registry& registry,
                  entt::entity payer,
                  entt::entity recipient,
                  int amount)
{
    setGold(registry, payer, availableGold(registry, payer) - amount);
    setGold(registry, recipient, availableGold(registry, recipient) + amount);
}

void moveItemToOwner(entt::registry& registry,
                     entt::entity item,
                     entt::entity owner)
{
    if (registry.all_of<Position>(item)) {
        registry.remove<Position>(item);
    }
    if (registry.all_of<Equipped>(item)) {
        registry.remove<Equipped>(item);
    }
    registry.emplace_or_replace<InBackpack>(item, InBackpack{owner});
}

void putInBackpack(entt::registry& registry,
                   entt::entity item,
                   entt::entity owner)
{
    if (registry.all_of<Position>(item)) {
        registry.remove<Position>(item);
    }
    registry.emplace_or_replace<InBackpack>(item, InBackpack{owner});
}

} // namespace

bool vendorTradesItem(const entt::registry& registry,
                      entt::entity vendor,
                      entt::entity item)
{
    if (!registry.valid(vendor) || !registry.valid(item) ||
        !registry.all_of<Vendor>(vendor) || !registry.all_of<ItemStats>(item)) {
        return false;
    }

    const auto& vendorData = registry.get<Vendor>(vendor);
    const auto& stats = registry.get<ItemStats>(item);
    return categoryMatches(vendorData.category, stats.category);
}

int itemBuyPrice(const entt::registry& registry, entt::entity item)
{
    if (!registry.valid(item) || !registry.all_of<ItemStats>(item)) {
        return 0;
    }
    return std::max(1, registry.get<ItemStats>(item).baseValue);
}

int itemSellPrice(const entt::registry& registry, entt::entity item)
{
    if (!registry.valid(item) || !registry.all_of<ItemStats>(item)) {
        return 0;
    }
    return std::max(1, registry.get<ItemStats>(item).baseValue / 2);
}

std::vector<entt::entity> vendorStock(const entt::registry& registry,
                                      entt::entity vendor)
{
    std::vector<entt::entity> stock;
    const auto view = registry.view<const Item, const InBackpack>();
    for (const auto item : view) {
        if (view.get<const InBackpack>(item).owner == vendor &&
            vendorTradesItem(registry, vendor, item)) {
            stock.push_back(item);
        }
    }

    std::sort(
        stock.begin(), stock.end(), [](entt::entity left, entt::entity right) {
            return entt::to_integral(left) < entt::to_integral(right);
        });
    return stock;
}

void populateVendorStock(entt::registry& registry,
                         const RawDatabase& raws,
                         entt::entity vendor)
{
    if (!registry.valid(vendor) || !registry.all_of<Vendor>(vendor) ||
        !vendorStock(registry, vendor).empty()) {
        return;
    }

    const auto& vendorData = registry.get<Vendor>(vendor);
    const auto ids = raws.itemIdsForCategory(vendorData.category);
    constexpr std::size_t maximumStock = 6;
    std::size_t stocked = 0;
    for (const auto& id : ids) {
        if (stocked >= maximumStock) {
            break;
        }
        const auto item = spawnRawEntity(registry, raws, id, Point{0, 0});
        if (!item.has_value() || !vendorTradesItem(registry, vendor, *item)) {
            continue;
        }
        putInBackpack(registry, *item, vendor);
        ++stocked;
    }
}

void populateAllVendorStock(entt::registry& registry, const RawDatabase& raws)
{
    const auto view = registry.view<const Vendor>();
    for (const auto vendor : view) {
        populateVendorStock(registry, raws, vendor);
    }
}

bool buyItemFromVendor(entt::registry& registry,
                       GameResources& resources,
                       entt::entity buyer,
                       entt::entity vendor,
                       entt::entity item)
{
    if (!hasGoldPool(registry, buyer) || !registry.valid(vendor) ||
        !registry.all_of<Vendor>(vendor) ||
        !hasBackpackOwner(registry, item, vendor)) {
        resources.log.add("The vendor is not selling that.");
        return false;
    }
    if (!vendorTradesItem(registry, vendor, item)) {
        resources.log.add("The vendor is not interested in that item.");
        return false;
    }

    const int price = itemBuyPrice(registry, item);
    if (registry.get<Pools>(buyer).gold < price) {
        resources.log.add("You cannot afford " + entityName(registry, item) +
                          ".");
        return false;
    }

    transferGold(registry, buyer, vendor, price);
    moveItemToOwner(registry, item, buyer);
    resources.log.add("You buy " + entityName(registry, item) + " for " +
                      std::to_string(price) + " gold.");
    return true;
}

bool sellItemToVendor(entt::registry& registry,
                      GameResources& resources,
                      entt::entity seller,
                      entt::entity vendor,
                      entt::entity item)
{
    if (!hasGoldPool(registry, seller) || !registry.valid(vendor) ||
        !registry.all_of<Vendor>(vendor)) {
        resources.log.add("There is no vendor here.");
        return false;
    }
    if (isEquippedBy(registry, item, seller)) {
        resources.log.add("You must unequip " + entityName(registry, item) +
                          " first.");
        return false;
    }
    if (!hasBackpackOwner(registry, item, seller)) {
        resources.log.add("You are not carrying that.");
        return false;
    }
    if (!vendorTradesItem(registry, vendor, item)) {
        resources.log.add("The vendor is not interested in " +
                          entityName(registry, item) + ".");
        return false;
    }

    const int price = itemSellPrice(registry, item);
    if (availableGold(registry, vendor) < price) {
        resources.log.add("The vendor cannot afford " +
                          entityName(registry, item) + ".");
        return false;
    }

    transferGold(registry, vendor, seller, price);
    moveItemToOwner(registry, item, vendor);
    resources.log.add("You sell " + entityName(registry, item) + " for " +
                      std::to_string(price) + " gold.");
    return true;
}

} // namespace rl
