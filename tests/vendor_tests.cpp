#include "TestSuites.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "core/GameResources.hpp"
#include "ecs/Components.hpp"
#include "raws/RawDatabase.hpp"
#include "spawn/ItemSpawner.hpp"
#include "spawn/Spawner.hpp"
#include "systems/services/VendorSystem.hpp"

namespace rl::test {
namespace {

[[nodiscard]] entt::entity
makeVendor(entt::registry& registry, std::string category, int gold)
{
    const auto vendor = registry.create();
    registry.emplace<Name>(vendor, Name{"Trader"});
    registry.emplace<Vendor>(vendor, Vendor{std::move(category), gold});
    registry.emplace<Pools>(vendor, Pools{1, 1, 0, 0, 1, 0, gold});
    return vendor;
}

void moveToBackpack(entt::registry& registry,
                    entt::entity item,
                    entt::entity owner)
{
    if (registry.all_of<Position>(item)) {
        registry.remove<Position>(item);
    }
    registry.emplace_or_replace<InBackpack>(item, InBackpack{owner});
}

[[nodiscard]] bool
isOwnedBy(const entt::registry& registry, entt::entity item, entt::entity owner)
{
    return registry.all_of<InBackpack>(item) &&
           registry.get<InBackpack>(item).owner == owner;
}

[[nodiscard]] bool contains(const std::vector<entt::entity>& entities,
                            entt::entity entity)
{
    return std::find(entities.begin(), entities.end(), entity) !=
           entities.end();
}

} // namespace

void runVendorTests(TestRunner& runner)
{
    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        registry.get<Pools>(resources.player).gold = 60;
        const auto vendor = makeVendor(registry, "weapons", 250);
        const auto sword = spawnSword(registry, Point{1, 1});
        moveToBackpack(registry, sword, vendor);

        runner.expect(itemBuyPrice(registry, sword) == 45,
                      "vendor buy price uses item base value");
        runner.expect(contains(vendorStock(registry, vendor), sword),
                      "vendor stock lists matching backpack items");
        runner.expect(buyItemFromVendor(
                          registry, resources, resources.player, vendor, sword),
                      "buyer can purchase matching vendor stock");
        runner.expect(isOwnedBy(registry, sword, resources.player),
                      "purchased item moves into buyer backpack");
        runner.expect(registry.get<Pools>(resources.player).gold == 15,
                      "purchase subtracts buyer gold");
        runner.expect(registry.get<Vendor>(vendor).gold == 295 &&
                          registry.get<Pools>(vendor).gold == 295,
                      "purchase adds vendor gold to both gold stores");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        registry.get<Pools>(resources.player).gold = 0;
        const auto vendor = makeVendor(registry, "weapons", 50);
        const auto dagger = spawnDagger(registry, Point{1, 1});
        moveToBackpack(registry, dagger, resources.player);

        runner.expect(itemSellPrice(registry, dagger) == 6,
                      "vendor sell price uses half item base value");
        runner.expect(
            sellItemToVendor(
                registry, resources, resources.player, vendor, dagger),
            "player can sell matching carried items");
        runner.expect(isOwnedBy(registry, dagger, vendor),
                      "sold item moves into vendor backpack");
        runner.expect(registry.get<Pools>(resources.player).gold == 6,
                      "sale adds player gold");
        runner.expect(registry.get<Vendor>(vendor).gold == 44 &&
                          registry.get<Pools>(vendor).gold == 44,
                      "sale subtracts vendor gold from both gold stores");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        registry.get<Pools>(resources.player).gold = 100;
        const auto vendor = makeVendor(registry, "weapons", 50);
        const auto potion = spawnHealingPotion(registry, Point{1, 1});
        moveToBackpack(registry, potion, resources.player);

        runner.expect(!vendorTradesItem(registry, vendor, potion),
                      "vendor rejects mismatched item categories");
        runner.expect(
            !sellItemToVendor(
                registry, resources, resources.player, vendor, potion),
            "category mismatch blocks selling");
        runner.expect(isOwnedBy(registry, potion, resources.player),
                      "rejected sale keeps item owner unchanged");
        runner.expect(registry.get<Pools>(resources.player).gold == 100,
                      "rejected sale keeps player gold unchanged");
    }

    {
        entt::registry registry;
        GameResources resources;
        resources.player = spawnPlayer(registry, Point{2, 2});
        registry.get<Pools>(resources.player).gold = 20;
        const auto vendor = makeVendor(registry, "weapons", 3);
        const auto sword = spawnSword(registry, Point{1, 1});
        moveToBackpack(registry, sword, vendor);
        const auto dagger = spawnDagger(registry, Point{1, 1});
        moveToBackpack(registry, dagger, resources.player);

        runner.expect(!buyItemFromVendor(
                          registry, resources, resources.player, vendor, sword),
                      "purchase fails when buyer lacks gold");
        runner.expect(isOwnedBy(registry, sword, vendor),
                      "failed purchase keeps vendor stock");
        runner.expect(
            !sellItemToVendor(
                registry, resources, resources.player, vendor, dagger),
            "sale fails when vendor lacks gold");
        runner.expect(isOwnedBy(registry, dagger, resources.player),
                      "failed sale keeps player item");
    }

    {
        entt::registry registry;
        const auto vendor = makeVendor(registry, "alchemy", 250);
        populateVendorStock(registry, defaultRawDatabase(), vendor);
        const auto stock = vendorStock(registry, vendor);
        runner.expect(
            std::any_of(
                stock.begin(),
                stock.end(),
                [&](entt::entity item) {
                    return registry.all_of<RawIdentity>(item) &&
                           registry.get<RawIdentity>(item).templateName ==
                               "food_ration";
                }),
            "alchemy vendors stock food rations");
    }
}

} // namespace rl::test
