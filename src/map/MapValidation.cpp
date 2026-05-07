/**
 * **************************************************************************
 * @file     MapValidation.cpp
 * @brief    Validates generated map reachability and required anchors
 * **************************************************************************
 */

#include "map/MapValidation.hpp"

#include "map/Map.hpp"
#include "map/MapConnectivity.hpp"

namespace rl {

MapValidationResult validateMapBuild(const Map& map, Point start, Point stairs)
{
    MapValidationResult result;
    result.walkableTiles = countConnectivityPassableTiles(map);

    if (!map.inBounds(start.x, start.y) ||
        !isConnectivityPassable(map, start)) {
        result.errors.push_back("start is not walkable");
    }
    if (!map.inBounds(stairs.x, stairs.y) ||
        !isConnectivityPassable(map, stairs)) {
        result.errors.push_back("stairs are not walkable");
    }

    result.reachableTiles = countReachableTiles(map, start);
    if (result.reachableTiles != result.walkableTiles) {
        result.errors.push_back("not all walkable tiles are reachable");
    }

    result.valid = result.errors.empty();
    return result;
}

} // namespace rl
