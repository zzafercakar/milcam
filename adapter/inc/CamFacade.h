#pragma once

/**
 * @file CamFacade.h
 * @brief Facade bridging CadEngine to the MilCAD CAM module.
 *
 * Provides a simplified API for creating CAM operations, generating
 * toolpaths, and exporting G-code. UI layer never touches MilCAD
 * types directly — only this facade.
 */

#include <memory>
#include <string>
#include <vector>

namespace MilCAD {
class CamJob;
}

namespace CADNC {

/// Info about a CAM operation for UI display
struct CamOpInfo {
    int id;
    std::string typeName;    // "Profile", "Pocket", "Drill", etc.
    std::string description; // brief summary
};

class CamFacade {
public:
    CamFacade();
    ~CamFacade();

    CamFacade(const CamFacade&) = delete;
    CamFacade& operator=(const CamFacade&) = delete;

    // ── Stock ───────────────────────────────────────────────────────
    void setStock(double length, double width, double height);

    // ── Tool ────────────────────────────────────────────────────────
    /// Add an end mill tool. Returns tool ID.
    int addEndMill(const std::string& name, double diameter, double fluteLength);

    /// Add a tool controller binding a tool to feed/speed settings. Returns controller ID.
    int addToolController(int toolId, double spindleRpm, double feedXY, double feedZ);

    // ── Operations ──────────────────────────────────────────────────
    /// Add a profile operation tracing a rectangular contour.
    int addProfileOp(int toolControllerId, double depth, double stepDown,
                     double x1, double y1, double x2, double y2);

    /// Add a pocket operation clearing a rectangular area.
    int addPocketOp(int toolControllerId, double depth, double stepDown, double stepOver,
                    double x1, double y1, double x2, double y2);

    /// Add a drill operation at given points.
    int addDrillOp(int toolControllerId, double depth,
                   const std::vector<std::pair<double,double>>& points);

    /// Add a facing operation across the stock top.
    int addFacingOp(int toolControllerId, double depth, double stepOver);

    // ── Query ───────────────────────────────────────────────────────
    std::vector<CamOpInfo> operations() const;
    int operationCount() const;

    // ── G-Code Generation ───────────────────────────────────────────
    /// Generate G-code for all operations (generic Fanuc dialect).
    std::string generateGCode() const;

    /// Generate G-code for CODESYS SoftMotion format.
    std::string generateCodesysGCode() const;

    /// Export G-code to file. Returns true on success.
    bool exportToFile(const std::string& filePath, bool codesys = false) const;

private:
    std::unique_ptr<MilCAD::CamJob> job_;
    int nextToolControllerId_ = 1;
};

} // namespace CADNC
