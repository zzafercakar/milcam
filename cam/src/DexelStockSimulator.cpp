/**
 * @file DexelStockSimulator.cpp
 * @brief Dexel-based stock material removal simulator for CNC machining visualization.
 *
 * Implements a 2D heightmap (dexel grid) approach to simulate material removal
 * from a rectangular stock block. Each cell in the grid stores the current top-Z
 * height of the remaining material. Toolpath segments are sampled along their
 * length and the tool footprint is subtracted from the heightmap at each sample
 * point, enabling fast approximate volume-removal calculations.
 */

#include "DexelStockSimulator.h"

#include <algorithm>
#include <cmath>

namespace MilCAD {

/**
 * @brief Initialize the dexel grid from the given stock definition and cell size.
 * @param stock  Rectangular stock block (origin, sizeX/Y/Z).
 * @param cellSize  Side length of each square dexel cell in XY (mm).
 * @return true if the grid was successfully created, false on invalid input.
 */
bool DexelStockSimulator::initialize(const Stock &stock, double cellSize)
{
    stock_ = stock;
    valid_ = false;

    if (stock_.sizeX <= 0.0 || stock_.sizeY <= 0.0 || stock_.sizeZ <= 0.0)
        return false;
    if (cellSize <= 0.0)
        return false;

    cellSize_ = cellSize;
    nx_ = std::max(1, static_cast<int>(std::ceil(stock_.sizeX / cellSize_)));
    ny_ = std::max(1, static_cast<int>(std::ceil(stock_.sizeY / cellSize_)));
    topZ_ = stock_.origin.Z() + stock_.sizeZ;

    heights_.assign(static_cast<std::size_t>(nx_ * ny_), topZ_);
    touched_.assign(static_cast<std::size_t>(nx_ * ny_), 0);
    valid_ = true;
    return true;
}

/**
 * @brief Reset all dexel heights back to the original stock top-Z.
 *
 * Restores the grid to an uncut state without reallocating memory.
 */
void DexelStockSimulator::reset()
{
    if (!valid_)
        return;
    std::fill(heights_.begin(), heights_.end(), topZ_);
    std::fill(touched_.begin(), touched_.end(), 0);
}

/**
 * @brief Carve material at a single tool contact point.
 * @param x  Tool center X coordinate (mm).
 * @param y  Tool center Y coordinate (mm).
 * @param z  Tool tip Z coordinate (mm) — material below this level is removed.
 * @param toolRadius  Radius of the cylindrical tool footprint (mm).
 *
 * Iterates over all dexel cells within the tool's bounding circle and lowers
 * any cell whose center falls inside the tool radius, clamping to the stock floor.
 */
void DexelStockSimulator::carvePoint(double x, double y, double z, double toolRadius)
{
    if (!valid_ || toolRadius <= 0.0)
        return;

    const double relMinX = (x - toolRadius - stock_.origin.X()) / cellSize_;
    const double relMaxX = (x + toolRadius - stock_.origin.X()) / cellSize_;
    const double relMinY = (y - toolRadius - stock_.origin.Y()) / cellSize_;
    const double relMaxY = (y + toolRadius - stock_.origin.Y()) / cellSize_;

    const int ix0 = std::clamp(static_cast<int>(std::floor(relMinX)), 0, nx_ - 1);
    const int ix1 = std::clamp(static_cast<int>(std::ceil(relMaxX)), 0, nx_ - 1);
    const int iy0 = std::clamp(static_cast<int>(std::floor(relMinY)), 0, ny_ - 1);
    const int iy1 = std::clamp(static_cast<int>(std::ceil(relMaxY)), 0, ny_ - 1);

    const double rr = toolRadius * toolRadius;
    for (int iy = iy0; iy <= iy1; ++iy) {
        const double cy = stock_.origin.Y() + (static_cast<double>(iy) + 0.5) * cellSize_;
        for (int ix = ix0; ix <= ix1; ++ix) {
            const double cx = stock_.origin.X() + (static_cast<double>(ix) + 0.5) * cellSize_;
            const double dx = cx - x;
            const double dy = cy - y;
            if ((dx * dx + dy * dy) > rr)
                continue;

            const int idx = indexOf(ix, iy);
            const double prev = heights_[static_cast<std::size_t>(idx)];
            const double next = std::max(stock_.origin.Z(), std::min(prev, z));
            if (next < prev) {
                heights_[static_cast<std::size_t>(idx)] = next;
                touched_[static_cast<std::size_t>(idx)] = 1;
            }
        }
    }
}

/**
 * @brief Apply an entire toolpath to the dexel grid, simulating material removal.
 * @param toolpath  The toolpath whose cutting segments will be applied.
 * @param toolDiameter  Diameter of the cutting tool (mm).
 *
 * Each non-rapid segment is sampled at regular intervals along its length.
 * At every sample point, carvePoint() is called with the tool radius to update
 * the heightmap. Rapid moves are skipped as they do not contact material.
 */
void DexelStockSimulator::applyToolpath(const Toolpath &toolpath, double toolDiameter)
{
    if (!valid_ || toolDiameter <= 0.0)
        return;

    const double toolRadius = toolDiameter * 0.5;
    const double sampleStep = std::max(cellSize_ * 0.5, 0.1);

    for (const auto &seg : toolpath.segments()) {
        if (seg.type == ToolpathSegmentType::Rapid)
            continue;

        const double segLen = seg.length();
        const int steps = std::max(1, static_cast<int>(std::ceil(segLen / sampleStep)));
        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(steps);
            const double x = seg.start.X() + (seg.end.X() - seg.start.X()) * t;
            const double y = seg.start.Y() + (seg.end.Y() - seg.start.Y()) * t;
            const double z = seg.start.Z() + (seg.end.Z() - seg.start.Z()) * t;
            carvePoint(x, y, z, toolRadius);
        }
    }
}

/**
 * @brief Compute and return the simulation result (volumes and statistics).
 * @return Result struct with initial, removed, and remaining volumes plus ratios.
 */
DexelStockSimulator::Result DexelStockSimulator::result() const
{
    Result out;
    if (!valid_)
        return out;

    out.initialVolume = stock_.sizeX * stock_.sizeY * stock_.sizeZ;
    out.resolutionX = nx_;
    out.resolutionY = ny_;

    const double cellArea = cellSize_ * cellSize_;
    double removed = 0.0;
    int touched = 0;

    for (std::size_t i = 0; i < heights_.size(); ++i) {
        const double dz = std::max(0.0, topZ_ - heights_[i]);
        removed += dz * cellArea;
        if (touched_[i] != 0)
            ++touched;
    }

    out.removedVolume = std::min(out.initialVolume, removed);
    out.remainingVolume = std::max(0.0, out.initialVolume - out.removedVolume);
    out.removalRatio = out.initialVolume > 1e-9 ? (out.removedVolume / out.initialVolume) : 0.0;
    out.touchedCells = touched;
    return out;
}

} // namespace MilCAD
