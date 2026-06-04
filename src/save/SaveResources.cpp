#include "save/SaveResources.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "save/SaveDungeon.hpp"
#include "save/SaveMap.hpp"
#include "world/WorldGeneration.hpp"

namespace rl::save {
namespace {

[[nodiscard]] Json worldFieldsToJson(const WorldCellFields& fields)
{
    return Json{{"elevation", fields.elevation},
                {"moisture", fields.moisture},
                {"woodland_density", fields.woodlandDensity},
                {"marsh_pressure", fields.marshPressure},
                {"ruin_density", fields.ruinDensity},
                {"danger_pressure", fields.dangerPressure}};
}

[[nodiscard]] Json landmarkToJson(const WorldLandmark& landmark)
{
    return Json{{"id", landmark.id},
                {"name", landmark.name},
                {"fast_travel_unlocked", landmark.fastTravelUnlocked},
                {"region_id", landmark.regionId},
                {"local_screen", pointToJson(landmark.localScreen)}};
}

void applyLandmarkProgress(WorldCellDescriptor& cell, const Json& json)
{
    if (!cell.landmark.has_value()) {
        return;
    }
    const auto id = json.value("id", std::string{});
    if (!id.empty() && id != cell.landmark->id) {
        return;
    }
    cell.landmark->fastTravelUnlocked =
        json.value("fast_travel_unlocked", cell.landmark->fastTravelUnlocked);
}

[[nodiscard]] Json cellProgressToJson(const WorldCellDescriptor& cell)
{
    Json json{{"position", pointToJson(cell.position)},
              {"fields", worldFieldsToJson(cell.fields)},
              {"biome_id", std::string{biomeVariantId(cell.biome)}},
              {"name", cell.name},
              {"builder_id", cell.builderId},
              {"local_seed", cell.localSeed},
              {"danger_tier", cell.dangerTier},
              {"base_safety", cell.baseSafety},
              {"discovered", cell.discovered},
              {"visited", cell.visited},
              {"region_id", cell.regionId}};
    if (cell.landmark.has_value()) {
        json["landmark"] = landmarkToJson(*cell.landmark);
    }
    return json;
}

void applyCellProgress(WorldMapState& world, const Json& json)
{
    const Point position = pointFromJson(json.at("position"));
    if (!worldPointInBounds(position)) {
        throw std::runtime_error("Invalid saved world cell coordinate");
    }
    auto* cell = worldCellAt(world, position);
    if (cell == nullptr) {
        return;
    }
    cell->discovered = json.value("discovered", cell->discovered);
    cell->visited = json.value("visited", cell->visited);
    if (json.contains("landmark")) {
        applyLandmarkProgress(*cell, json.at("landmark"));
    }
}

[[nodiscard]] Json routeProgressToJson(const RouteLink& route)
{
    return Json{{"destination_region_id", route.destinationRegionId},
                {"discovered", route.discovered}};
}

[[nodiscard]] Json regionProgressToJson(const WorldRegionDescriptor& region)
{
    Json routes = Json::array();
    for (const auto& route : region.routes) {
        routes.push_back(routeProgressToJson(route));
    }
    return Json{{"id", region.id},
                {"discovered", region.discovered},
                {"visited", region.visited},
                {"routes", std::move(routes)}};
}

void applyRegionProgress(WorldMapState& world, const Json& json)
{
    auto* region = worldRegionById(world, json.value("id", std::string{}));
    if (region == nullptr) {
        return;
    }
    region->discovered = json.value("discovered", region->discovered);
    region->visited = json.value("visited", region->visited);
    for (const auto& savedRoute : json.value("routes", Json::array())) {
        const auto destination =
            savedRoute.value("destination_region_id", std::string{});
        for (auto& route : region->routes) {
            if (route.destinationRegionId == destination) {
                route.discovered =
                    savedRoute.value("discovered", route.discovered);
            }
        }
    }
}

[[nodiscard]] Json surfaceScreenKeyToJson(const SurfaceScreenKey& key)
{
    return Json{{"macro", pointToJson(key.macro)},
                {"screen", pointToJson(key.screen)},
                {"danger_encounter", key.dangerEncounter}};
}

[[nodiscard]] SurfaceScreenKey surfaceScreenKeyFromJson(const Json& json)
{
    SurfaceScreenKey key;
    key.macro = pointFromJson(json.at("macro"));
    key.screen = pointFromJson(json.at("screen"));
    key.dangerEncounter = json.value("danger_encounter", false);
    if (!worldPointInBounds(key.macro) || !localScreenInBounds(key.screen)) {
        throw std::runtime_error("Invalid saved surface screen key");
    }
    return key;
}

[[nodiscard]] Json surfaceScreenToJson(const SurfaceScreenState& screen)
{
    Json entities = Json::array();
    for (const auto& entity : screen.entities) {
        entities.push_back(storedEntityToJson(entity));
    }
    return Json{{"key", surfaceScreenKeyToJson(screen.key)},
                {"builder_id", screen.builderId},
                {"map", mapToJson(screen.map)},
                {"entities", std::move(entities)}};
}

[[nodiscard]] SurfaceScreenState surfaceScreenFromJson(const Json& json)
{
    SurfaceScreenState screen;
    screen.key = surfaceScreenKeyFromJson(json.at("key"));
    screen.builderId = json.value("builder_id", std::string{});
    screen.map = mapFromJson(json.at("map"));
    for (const auto& entity : json.value("entities", Json::array())) {
        screen.entities.push_back(storedEntityFromJson(entity));
    }
    return screen;
}

[[nodiscard]] Json worldToJson(const WorldMapState& world)
{
    Json cells = Json::array();
    for (const auto& cell : world.cells) {
        cells.push_back(cellProgressToJson(cell));
    }
    Json regions = Json::array();
    for (const auto& [id, region] : world.regions) {
        (void)id;
        regions.push_back(regionProgressToJson(region));
    }
    Json surfaceScreens = Json::array();
    for (const auto& [key, screen] : world.surfaceScreens) {
        (void)key;
        surfaceScreens.push_back(surfaceScreenToJson(screen));
    }

    return Json{{"active", world.active},
                {"on_surface", world.onSurface},
                {"current_macro", pointToJson(world.currentMacro)},
                {"current_screen", pointToJson(world.currentScreen)},
                {"current_danger_encounter", world.currentDangerEncounter},
                {"cursor", pointToJson(world.cursor)},
                {"current_region_id", world.currentRegionId},
                {"underground_entry_depth", world.undergroundEntryDepth},
                {"cells", std::move(cells)},
                {"regions", std::move(regions)},
                {"surface_screens", std::move(surfaceScreens)}};
}

[[nodiscard]] Point savedWorldPoint(const Json& json, const char* key)
{
    const Point point = pointFromJson(json.at(key));
    if (!worldPointInBounds(point)) {
        throw std::runtime_error("Invalid saved world point");
    }
    return point;
}

[[nodiscard]] Point savedLocalScreenPoint(const Json& json, const char* key)
{
    const Point point = pointFromJson(json.at(key));
    if (!localScreenInBounds(point)) {
        throw std::runtime_error("Invalid saved local screen point");
    }
    return point;
}

[[nodiscard]] WorldMapState worldFromJson(const Json& json, Rng::Seed seed)
{
    WorldMapState world = generateWorldMap(seed);
    world.active = json.value("active", world.active);
    world.onSurface = json.value("on_surface", world.onSurface);
    if (json.contains("current_macro")) {
        world.currentMacro = savedWorldPoint(json, "current_macro");
    }
    if (json.contains("current_screen")) {
        world.currentScreen = savedLocalScreenPoint(json, "current_screen");
    }
    world.currentDangerEncounter =
        json.value("current_danger_encounter", world.currentDangerEncounter);
    if (json.contains("cursor")) {
        world.cursor = savedWorldPoint(json, "cursor");
    }
    world.currentRegionId =
        json.value("current_region_id", world.currentRegionId);
    world.undergroundEntryDepth =
        json.value("underground_entry_depth", world.undergroundEntryDepth);
    if (world.undergroundEntryDepth < 1) {
        throw std::runtime_error("Invalid saved underground entry depth");
    }

    for (const auto& cell : json.value("cells", Json::array())) {
        applyCellProgress(world, cell);
    }
    for (const auto& region : json.value("regions", Json::array())) {
        applyRegionProgress(world, region);
    }

    world.surfaceScreens.clear();
    for (const auto& savedScreen :
         json.value("surface_screens", Json::array())) {
        auto screen = surfaceScreenFromJson(savedScreen);
        world.surfaceScreens.emplace(screen.key, std::move(screen));
    }

    if (currentWorldRegion(world) == nullptr) {
        updateCurrentWorldRegion(world);
    }
    return world;
}

} // namespace

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
        {"town_portal", std::move(portal)},
        {"world", worldToJson(resources.world)}};
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
    loaded.world = json.contains("world")
                       ? worldFromJson(json.at("world"), loaded.rng.seed())
                       : generateWorldMap(loaded.rng.seed());

    resources = std::move(loaded);
}

} // namespace rl::save
