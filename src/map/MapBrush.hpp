/**
 * **************************************************************************
 * @file     MapBrush.hpp
 * @brief    Declares brush and symmetry utilities for map generation
 * **************************************************************************
 */

#pragma once

#include <vector>

#include "core/Point.hpp"
#include "map/TileType.hpp"

namespace rl {

class Map;

/** Selects the local footprint painted by a generation step. */
enum class BrushShape { Single, Square, Circle };
/** Mirrors a paint operation to create deliberate special-space identities. */
enum class Symmetry { None, Horizontal, Vertical, Both, Quad };

[[nodiscard]] std::vector<Point>
brushPoints(Point center, BrushShape shape, int radius);

/** Expands one point through the selected map-centered symmetry transform. */
[[nodiscard]] std::vector<Point>
symmetricalPoints(const Map& map, Point point, Symmetry symmetry);

std::vector<Point> paintBrush(Map& map,
                              Point center,
                              TileType tile,
                              BrushShape shape,
                              int radius,
                              Symmetry symmetry = Symmetry::None);

std::vector<Point> paintBrushSize(Map& map,
                                  Point center,
                                  TileType tile,
                                  int brushSize,
                                  Symmetry symmetry = Symmetry::None);

} // namespace rl
