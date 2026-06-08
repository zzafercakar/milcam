/**
 * @file DexelStockSimulator.h
 * @brief 2.5D dexel-based stock removal simulation for material volume analysis.
 *
 * Implements a height-field (dexel) model of the workpiece stock, where
 * each grid cell stores the remaining material height. Toolpath segments
 * are sampled along their length to "carve" material away, enabling volume
 * removal estimation without full 3D boolean operations.
 */

#ifndef MILCAD_DEXEL_STOCK_SIMULATOR_H
#define MILCAD_DEXEL_STOCK_SIMULATOR_H

#include "Tooling.h"
#include "Toolpath.h"

#include <vector>

namespace MilCAD {

/**
 * @class DexelStockSimulator
 * @brief Simulates material removal on a 2D height-field grid using cylindrical tool projection.
 *
 * The stock is discretized into a uniform 2D grid of dexels (vertical columns).
 * Each cell tracks the current top-of-material height. When a toolpath is applied,
 * the tool's cylindrical footprint is projected onto the grid and cell heights are
 * lowered to the tool's Z depth where the tool intersects.
 */
class DexelStockSimulator
{
public:
    /**
     * @struct Result
     * @brief Summary statistics from the stock removal simulation.
     */
    struct Result {
        double initialVolume = 0.0;   ///< Original stock volume in mm^3
        double removedVolume = 0.0;   ///< Volume of material removed in mm^3
        double remainingVolume = 0.0; ///< Remaining material volume in mm^3
        double removalRatio = 0.0;    ///< Fraction of material removed (0.0 to 1.0)
        int touchedCells = 0;         ///< Number of grid cells that were carved
        int resolutionX = 0;          ///< Grid resolution along the X axis
        int resolutionY = 0;          ///< Grid resolution along the Y axis
    };

    /**
     * @brief Initialize the dexel grid from stock dimensions.
     * @param stock The workpiece stock defining origin and extents.
     * @param cellSize The side length of each square grid cell in mm.
     * @return true if initialization succeeded; false if stock or cellSize is invalid.
     */
    bool initialize(const Stock &stock, double cellSize);

    /**
     * @brief Reset all dexel heights to the initial stock top, clearing any carved material.
     */
    void reset();

    /**
     * @brief Apply a toolpath to the dexel grid, carving material with a cylindrical tool.
     * @param toolpath The CNC toolpath to simulate (rapid moves are skipped).
     * @param toolDiameter The cutting tool diameter in mm.
     */
    void applyToolpath(const Toolpath &toolpath, double toolDiameter);

    /// @brief Check whether the simulator has been successfully initialized.
    bool valid() const { return valid_; }

    /**
     * @brief Compute and return summary statistics of the current simulation state.
     * @return A Result struct with volume and cell counts. Returns zeroed result if invalid.
     */
    Result result() const;

private:
    /**
     * @brief Carve a single tool position into the dexel grid.
     * @param x Tool center X coordinate.
     * @param y Tool center Y coordinate.
     * @param z Tool bottom Z coordinate (material below this is removed).
     * @param toolRadius Half the tool diameter.
     */
    void carvePoint(double x, double y, double z, double toolRadius);

    /**
     * @brief Convert 2D grid indices to a linear array index.
     * @param ix Column index (X direction).
     * @param iy Row index (Y direction).
     * @return Linear index into the heights_ and touched_ arrays.
     */
    int indexOf(int ix, int iy) const { return iy * nx_ + ix; }

    Stock stock_;                    ///< Copy of the stock definition
    bool valid_ = false;             ///< Whether the grid has been successfully initialized
    double cellSize_ = 0.0;         ///< Side length of each dexel cell in mm
    int nx_ = 0;                     ///< Number of cells along the X axis
    int ny_ = 0;                     ///< Number of cells along the Y axis
    double topZ_ = 0.0;             ///< Top-of-stock Z coordinate
    std::vector<double> heights_;    ///< Per-cell current material height (Z top)
    std::vector<unsigned char> touched_; ///< Per-cell flag: 1 if material was removed
};

} // namespace MilCAD

#endif // MILCAD_DEXEL_STOCK_SIMULATOR_H
