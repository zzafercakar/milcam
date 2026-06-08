/**
 * @file ProfileOperation.h
 * @brief Contour/profile milling operation along a 2D polyline.
 *
 * Generates a toolpath that follows a closed or open contour at multiple
 * depth levels, stepping down incrementally until the target Z is reached.
 * Supports professional CAM features: lead-in/out arcs, holding tabs,
 * stock-to-leave allowance, and climb/conventional milling direction.
 */

#ifndef MILCAD_PROFILE_OPERATION_H
#define MILCAD_PROFILE_OPERATION_H

#include "Operation.h"

#include <vector>

namespace MilCAD {

/**
 * @enum LeadType
 * @brief Entry/exit approach type for profile milling.
 */
enum class LeadType {
    None,  ///< Direct entry (no lead motion)
    Line,  ///< Linear tangent entry/exit
    Arc    ///< Circular arc entry/exit (SolidWorks/Fusion 360 style)
};

/**
 * @enum MillDirection
 * @brief Cutting direction relative to tool rotation.
 */
enum class MillDirection {
    Climb,        ///< Climb milling (preferred for CNC — less deflection)
    Conventional  ///< Conventional milling (traditional — more tool wear)
};

/**
 * @class ProfileOperation
 * @brief CAM operation that traces a 2D contour at progressive depth levels.
 *
 * Professional features:
 * - Lead-in/out arc or line for smooth tool engagement/disengagement
 * - Holding tabs (bridges) to keep workpiece attached to stock
 * - Stock-to-leave for roughing passes with finishing allowance
 * - Climb vs conventional milling direction selection
 * - Multi-pass depth stepping with configurable step-down
 */
class ProfileOperation : public Operation
{
public:
    std::string typeName() const override { return "Profile"; }

    // --- Core contour parameters ---

    /// @brief Set the 2D contour as a sequence of points (auto-closed if first != last).
    void setContour(const std::vector<gp_Pnt> &contour) { contour_ = contour; }

    /// @brief Set the final target cutting depth Z coordinate.
    void setTargetZ(double v) { targetZ_ = v; }

    /// @brief Set the Z-axis step-down per depth pass in mm.
    void setStepDown(double v) { stepDown_ = v; }

    /// @brief Set the safe Z height for rapid moves above the workpiece.
    void setSafeZ(double v) { safeZ_ = v; }

    /// @brief Set the cutting feed rate in mm/min.
    void setFeedCut(double v) { feedCut_ = v; }

    /// @brief Set the plunge feed rate in mm/min.
    void setFeedPlunge(double v) { feedPlunge_ = v; }

    // --- Lead-In / Lead-Out (arc or line entry/exit) ---

    /// @brief Set the lead-in approach type (None, Line, or Arc).
    void setLeadInType(LeadType t) { leadInType_ = t; }

    /// @brief Set the lead-in radius (arc) or length (line) in mm.
    void setLeadInRadius(double r) { leadInRadius_ = r; }

    /// @brief Set the lead-in arc sweep angle in degrees (only for Arc type).
    void setLeadInAngle(double deg) { leadInAngle_ = deg; }

    /// @brief Set the lead-out approach type (None, Line, or Arc).
    void setLeadOutType(LeadType t) { leadOutType_ = t; }

    /// @brief Set the lead-out radius (arc) or length (line) in mm.
    void setLeadOutRadius(double r) { leadOutRadius_ = r; }

    /// @brief Set the lead-out arc sweep angle in degrees (only for Arc type).
    void setLeadOutAngle(double deg) { leadOutAngle_ = deg; }

    // --- Tab / Bridge support (hold workpiece during cutting) ---

    /// @brief Enable or disable holding tabs along the profile.
    void setTabsEnabled(bool en) { tabsEnabled_ = en; }

    /// @brief Set the width of each tab in mm along the contour.
    void setTabWidth(double w) { tabWidth_ = w; }

    /// @brief Set the tab height from the bottom (floor) in mm.
    void setTabHeight(double h) { tabHeight_ = h; }

    /// @brief Set the number of evenly-distributed tabs around the contour.
    void setTabCount(int n) { tabCount_ = n; }

    // --- Stock-to-Leave (finishing allowance) ---

    /// @brief Set the radial (wall) stock-to-leave allowance in mm.
    void setStockToLeaveRadial(double v) { stockToLeaveRadial_ = v; }

    /// @brief Set the axial (floor) stock-to-leave allowance in mm.
    void setStockToLeaveAxial(double v) { stockToLeaveAxial_ = v; }

    // --- Milling direction ---

    /// @brief Set the milling direction (Climb or Conventional).
    void setMillDirection(MillDirection d) { millDirection_ = d; }

    /**
     * @brief Generate the profile toolpath with all professional parameters.
     * @return A Toolpath with lead-in/out arcs, tabs, stock-to-leave offsets,
     *         and multi-depth passes. Empty if contour < 2 points or params invalid.
     */
    Toolpath generateToolpath() const override;

private:
    // --- Core parameters ---
    std::vector<gp_Pnt> contour_;         ///< Ordered contour vertices
    double targetZ_   = -2.0;            ///< Final depth Z in mm
    double stepDown_  = 1.0;             ///< Depth per pass in mm
    double safeZ_     = 5.0;             ///< Safe retract height in mm
    double feedCut_   = 1000.0;          ///< Cutting feed rate in mm/min
    double feedPlunge_ = 250.0;          ///< Plunge feed rate in mm/min

    // --- Lead-In / Lead-Out ---
    LeadType leadInType_   = LeadType::None;  ///< Entry approach type
    double leadInRadius_   = 3.0;             ///< Lead-in radius or length (mm)
    double leadInAngle_    = 90.0;            ///< Lead-in arc sweep angle (degrees)
    LeadType leadOutType_  = LeadType::None;  ///< Exit approach type
    double leadOutRadius_  = 3.0;             ///< Lead-out radius or length (mm)
    double leadOutAngle_   = 90.0;            ///< Lead-out arc sweep angle (degrees)

    // --- Tab / Bridge support ---
    bool tabsEnabled_ = false;  ///< Whether holding tabs are active
    double tabWidth_  = 5.0;    ///< Tab width along contour (mm)
    double tabHeight_ = 1.0;    ///< Tab height from bottom (mm)
    int tabCount_     = 4;      ///< Number of evenly-distributed tabs

    // --- Stock-to-Leave ---
    double stockToLeaveRadial_ = 0.0;  ///< Wall finishing allowance (mm)
    double stockToLeaveAxial_  = 0.0;  ///< Floor finishing allowance (mm)

    // --- Milling direction ---
    MillDirection millDirection_ = MillDirection::Climb;
};

} // namespace MilCAD

#endif // MILCAD_PROFILE_OPERATION_H
