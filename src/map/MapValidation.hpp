/**
 * **************************************************************************
 * @file     MapValidation.hpp
 * @brief    Declares reachability checks for generated maps
 * **************************************************************************
 */

#pragma once

#include <string>
#include <vector>

#include "core/Point.hpp"

namespace rl {

class Map;

struct MapValidationResult {
    bool valid = false;
    int walkableTiles = 0;
    int reachableTiles = 0;
    std::vector<std::string> errors;
};

[[nodiscard]] MapValidationResult
validateMapBuild(const Map& map, Point start, Point stairs);

} // namespace rl
