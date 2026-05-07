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

enum class BrushShape { Single, Square, Circle };
enum class Symmetry { None, Horizontal, Vertical, Both, Quad };

[[nodiscard]] std::vector<Point>
brushPoints(Point center, BrushShape shape, int radius);

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
