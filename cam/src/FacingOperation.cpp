/**
 * @file FacingOperation.cpp
 * @brief Facing (surface machining) CAM operation for leveling the top of stock.
 *
 * Generates a zigzag raster toolpath that machines the top surface of a
 * rectangular stock block down to a specified target-Z depth. The operation
 * proceeds in multiple depth passes (stepDown) with alternating left-to-right
 * and right-to-left rows separated by stepOver, producing a flat finish.
 */

#include "FacingOperation.h"

#include <algorithm>
#include <cmath>

namespace MilCAD {

/**
 * @brief Generate the facing toolpath over the stock bounding rectangle.
 * @return Toolpath with zigzag raster passes from stock top to target depth.
 *
 * Each depth pass sweeps all Y rows with alternating X direction (zigzag).
 * Every row consists of: rapid positioning, plunge to cut depth, linear cut
 * across the stock width, and retract to safe-Z.
 */
Toolpath FacingOperation::generateToolpath() const
{
    Toolpath out;

    if (stock_.sizeX <= 0.0 || stock_.sizeY <= 0.0 || stock_.sizeZ < 0.0)
        return out;
    if (stepOver_ <= 1e-9 || passDepth_ <= 1e-9 || feedCut_ <= 1e-9 || feedPlunge_ <= 1e-9)
        return out;

    const double minX = stock_.origin.X();
    const double minY = stock_.origin.Y();
    const double maxX = stock_.origin.X() + stock_.sizeX;
    const double maxY = stock_.origin.Y() + stock_.sizeY;
    const double topZ = stock_.origin.Z() + stock_.sizeZ;
    const double safeZ = std::max(safeZ_, topZ + 0.5);
    const double finalZ = std::min(targetZ_, topZ);

    for (double z = topZ - passDepth_; z >= finalZ - 1e-9; z -= passDepth_) {
        const double cutZ = std::max(z, finalZ);
        bool leftToRight = true;

        for (double y = minY; y <= maxY + 1e-9; y += stepOver_) {
            const double yRow = std::min(y, maxY);
            const double sx = leftToRight ? minX : maxX;
            const double ex = leftToRight ? maxX : minX;

            ToolpathSegment rapidToStart;
            rapidToStart.type = ToolpathSegmentType::Rapid;
            rapidToStart.start = gp_Pnt(sx, yRow, safeZ);
            rapidToStart.end = gp_Pnt(sx, yRow, safeZ);
            out.add(rapidToStart);

            ToolpathSegment plunge;
            plunge.type = ToolpathSegmentType::Plunge;
            plunge.start = gp_Pnt(sx, yRow, safeZ);
            plunge.end = gp_Pnt(sx, yRow, cutZ);
            plunge.feedRate = feedPlunge_;
            out.add(plunge);

            ToolpathSegment cut;
            cut.type = ToolpathSegmentType::Linear;
            cut.start = gp_Pnt(sx, yRow, cutZ);
            cut.end = gp_Pnt(ex, yRow, cutZ);
            cut.feedRate = feedCut_;
            out.add(cut);

            ToolpathSegment retract;
            retract.type = ToolpathSegmentType::Rapid;
            retract.start = gp_Pnt(ex, yRow, cutZ);
            retract.end = gp_Pnt(ex, yRow, safeZ);
            out.add(retract);

            leftToRight = !leftToRight;
        }
    }

    return out;
}

} // namespace MilCAD
