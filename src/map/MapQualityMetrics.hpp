/**
 * **************************************************************************
 * @file     MapQualityMetrics.hpp
 * @brief    Declares reusable structural quality measurements for maps
 * **************************************************************************
 */

#pragma once

namespace rl {

class Map;

struct MapQualityMetrics {
    int totalTiles = 0;
    int walkableTiles = 0;
    int broadOpenTiles = 0;
    int narrowTiles = 0;
    int disconnectedRegions = 0;
    int chokepoints = 0;
    int tinyBlockerComponents = 0;
    double walkableCoverage = 0.0;
    double broadOpenPercent = 0.0;
    double narrowPercent = 0.0;
};

/** Measures coverage, topology, and small natural blocker fragments. */
[[nodiscard]] MapQualityMetrics measureMapQuality(const Map& map);

} // namespace rl
