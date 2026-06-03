#include "save/SaveResources.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "save/SaveEntities.hpp"

namespace rl::save {

[[nodiscard]] Json resourcesToJson(const entt::registry& registry,
                                   const GameResources& resources)
{
    Json messages = Json::array();
    for (const auto message : resources.log.messages()) {
        messages.push_back(std::string{message});
    }
    Json entries = Json::array();
    for (const auto& entry : resources.log.entries()) {
        entries.push_back(logEntryToJson(entry));
    }
    Json counters = Json::object();
    for (const auto& [id, amount] : resources.log.counters()) {
        counters[id] = amount;
    }

    Json portal = Json::object();
    portal["active"] = resources.townPortalActive;
    portal["return_depth"] = resources.townPortalReturnDepth;
    if (resources.townPortalReturnPosition.has_value()) {
        portal["return_position"] =
            pointToJson(*resources.townPortalReturnPosition);
    }
    if (resources.townPortalPosition.has_value()) {
        portal["town_position"] = pointToJson(*resources.townPortalPosition);
    }

    return Json{
        {"depth", resources.depth},
        {"player", entityId(registry, resources.player)},
        {"map_builder", resources.mapBuilderId},
        {"rng_seed", resources.rng.seed()},
        {"rng_engine", resources.rng.engineState()},
        {"log", std::move(messages)},
        {"log_entries", std::move(entries)},
        {"log_counters", std::move(counters)},
        {"progress",
         Json{{"artifact_claimed", resources.progress.artifactClaimed}}},
        {"town_portal", std::move(portal)}};
}

void loadResources(const Json& json,
                   const std::unordered_map<EntityId, entt::entity>& entities,
                   GameResources& resources)
{
    GameResources loaded;
    loaded.depth = json.at("depth").get<int>();
    if (loaded.depth < 1) {
        throw std::runtime_error("Invalid saved dungeon depth");
    }

    loaded.rng.reseed(json.at("rng_seed").get<Rng::Seed>());
    loaded.mapBuilderId = json.value("map_builder", std::string{"simple"});
    loaded.rng.restoreEngineState(json.at("rng_engine").get<std::string>());
    const auto playerId = json.at("player").get<EntityId>();
    const auto player = entities.find(playerId);
    if (player == entities.end()) {
        throw std::runtime_error("Saved player entity is missing");
    }
    loaded.player = player->second;

    if (json.contains("log_entries")) {
        std::vector<LogEntry> entries;
        for (const auto& entry : json.value("log_entries", Json::array())) {
            entries.push_back(logEntryFromJson(entry));
        }
        loaded.log.replaceEntries(std::move(entries));
    } else {
        for (const auto& message : json.value("log", Json::array())) {
            loaded.log.add(message.get<std::string>());
        }
    }
    const auto savedCounters = json.value("log_counters", Json::object());
    for (const auto& [id, amount] : savedCounters.items()) {
        loaded.log.setCounter(id, amount.get<int>());
    }
    if (json.contains("progress")) {
        const auto& progress = json.at("progress");
        loaded.progress.artifactClaimed =
            progress.value("artifact_claimed", false);
    }

    if (json.contains("town_portal")) {
        const auto& portal = json.at("town_portal");
        loaded.townPortalActive = portal.value("active", false);
        loaded.townPortalReturnDepth = portal.value("return_depth", 1);
        if (portal.contains("return_position")) {
            loaded.townPortalReturnPosition =
                pointFromJson(portal.at("return_position"));
        }
        if (portal.contains("town_position")) {
            loaded.townPortalPosition =
                pointFromJson(portal.at("town_position"));
        }
    }

    resources = std::move(loaded);
}

} // namespace rl::save
