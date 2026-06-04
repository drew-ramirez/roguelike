/**
 * **************************************************************************
 * @file     VendorSystem.hpp
 * @brief    Declares vendor pricing and item transaction helpers
 * **************************************************************************
 */

#pragma once

#include <vector>

#include <entt/entt.hpp>

namespace rl {

struct GameResources;
class RawDatabase;

[[nodiscard]] bool vendorTradesItem(const entt::registry& registry,
                                    entt::entity vendor,
                                    entt::entity item);

[[nodiscard]] int itemBuyPrice(const entt::registry& registry,
                               entt::entity item);

[[nodiscard]] int itemSellPrice(const entt::registry& registry,
                                entt::entity item);

[[nodiscard]] std::vector<entt::entity>
vendorStock(const entt::registry& registry, entt::entity vendor);

void populateVendorStock(entt::registry& registry,
                         const RawDatabase& raws,
                         entt::entity vendor);

void populateAllVendorStock(entt::registry& registry, const RawDatabase& raws);

[[nodiscard]] bool buyItemFromVendor(entt::registry& registry,
                                     GameResources& resources,
                                     entt::entity buyer,
                                     entt::entity vendor,
                                     entt::entity item);

[[nodiscard]] bool sellItemToVendor(entt::registry& registry,
                                    GameResources& resources,
                                    entt::entity seller,
                                    entt::entity vendor,
                                    entt::entity item);

} // namespace rl
