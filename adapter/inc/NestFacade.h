#pragma once

/**
 * @file NestFacade.h
 * @brief Facade bridging CadEngine to the MilCAD Nesting module.
 *
 * Provides a simplified API for adding parts, configuring sheets,
 * running nesting algorithms, and reading results. UI layer uses
 * only this facade — never MilCAD types directly.
 */

#include <memory>
#include <string>
#include <vector>

namespace MilCAD {
class NestJob;
}

namespace CADNC {

/// Placement info for UI display
struct NestPlacementInfo {
    std::string partId;
    double x;
    double y;
    double rotation;  // degrees
    int sheetIndex;
};

/// Result summary for UI
struct NestResultInfo {
    int totalPlaced;
    int totalUnplaced;
    double utilization;  // 0.0 – 1.0
    int sheetsUsed;
    std::vector<NestPlacementInfo> placements;
    std::vector<std::string> unplacedIds;
};

class NestFacade {
public:
    NestFacade();
    ~NestFacade();

    NestFacade(const NestFacade&) = delete;
    NestFacade& operator=(const NestFacade&) = delete;

    // ── Parts ───────────────────────────────────────────────────────
    /// Add a rectangular part for nesting. Returns true on success.
    bool addPart(const std::string& id, double width, double height,
                 int quantity = 1, bool allowRotation = true);

    /// Clear all parts.
    void clearParts();

    int partCount() const;

    // ── Sheet ───────────────────────────────────────────────────────
    /// Set the sheet dimensions.
    void setSheet(const std::string& id, double width, double height);

    // ── Parameters ──────────────────────────────────────────────────
    /// Set gap between parts (mm).
    void setPartGap(double gap);

    /// Set gap from sheet edges (mm).
    void setEdgeGap(double gap);

    /// Set rotation mode: 0=None, 1=Quadrant(0/90/180/270), 2=Free(15° steps).
    void setRotationMode(int mode);

    /// Set optimization time budget (seconds). 0 = single pass.
    void setOptimizationTime(double seconds);

    // ── Run ─────────────────────────────────────────────────────────
    /// Run the nesting algorithm. 0=BoundingBoxRows, 1=BottomLeftFill.
    NestResultInfo run(int algorithm = 1);

    /// Get the last result.
    NestResultInfo lastResult() const;

private:
    std::unique_ptr<MilCAD::NestJob> job_;
    NestResultInfo lastResult_;
};

} // namespace CADNC
