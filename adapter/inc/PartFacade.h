#pragma once

/**
 * @file PartFacade.h
 * @brief Facade for FreeCAD PartDesign operations.
 *
 * Wraps FreeCAD PartDesign features (Pad, Pocket, Fillet, Chamfer,
 * Revolution, Loft, Sweep, Pattern, Mirror) without exposing FreeCAD
 * internals to the UI.
 */

#include <memory>
#include <string>
#include <vector>

namespace CADNC {

/// Query result for a feature's editable parameters (used by the task panel
/// when opening an existing Pad/Pocket/Revolution/Groove for edit).
struct FeatureParams {
    std::string typeName;     // "PartDesign::Pad" etc. — empty if feature not found
    std::string sketchName;   // Profile sketch backing this feature (if any)
    double length = 0.0;      // Pad/Pocket Length (mm)
    double length2 = 0.0;     // Pad/Pocket second length for "Two sides"
    double angle = 0.0;       // Revolution/Groove Angle (deg)
    bool reversed = false;
    bool editable = false;    // true = parametric PartDesign feature, false = OCCT fallback

    // UX-008: extrusion method and side mode. Mirrors FeatureExtrude's
    // `Type` + `SideType` enumerations (see FreeCAD's own property model).
    //   sideType: "One side" | "Two sides" | "Symmetric"
    //   method:   "Length"   | "ThroughAll"   (further modes deferred)
    std::string sideType;
    std::string method;
};

/// UX-008: struct-form options for pad/pocket creation. Collapses the
/// would-be overload explosion (length+length2+sideType+method+...) into
/// a single named-field argument the QML panel can populate directly.
struct PadOptions {
    double length   = 10.0;
    double length2  = 10.0;   // only used when sideType == "Two sides"
    bool   reversed = false;
    std::string sideType = "One side";   // "One side" / "Two sides" / "Symmetric"
    std::string method   = "Length";     // "Length" / "ThroughAll"
};

class PartFacade {
public:
    explicit PartFacade(void* document); // opaque — App::Document*
    ~PartFacade();

    PartFacade(const PartFacade&) = delete;
    PartFacade& operator=(const PartFacade&) = delete;

    // ── Sketch-based features ───────────────────────────────────────
    /// Extrude a sketch (Pad). Returns feature name.
    std::string pad(const std::string& sketchName, double length);
    std::string pocket(const std::string& sketchName, double depth);
    std::string revolution(const std::string& sketchName, double angleDeg);

    /// UX-008: rich Pad with SideType + method (ThroughAll, Symmetric,
    /// Two sides) + Length2. Falls back to OCCT for non-parametric doc
    /// state. `opts` carries all optional fields; caller fills what it
    /// needs and defaults kick in for the rest.
    std::string padEx(const std::string& sketchName, const PadOptions& opts);
    std::string pocketEx(const std::string& sketchName, const PadOptions& opts);

    // ── Parametric edit (UX-010 / UX-011 live preview) ──────────────
    /// Read current parameter values of an existing feature. Returns a
    /// struct with editable=false if the feature doesn't exist or was
    /// produced via the OCCT fallback (non-parametric Part::Feature).
    FeatureParams getFeatureParams(const std::string& featureName);

    /// Mutate a PartDesign feature's primary scalar and recompute. Returns
    /// true on success. Feature type is auto-detected from the object class.
    bool updatePad(const std::string& featureName, double length, bool reversed);
    bool updatePocket(const std::string& featureName, double depth, bool reversed);
    bool updateRevolution(const std::string& featureName, double angleDeg);
    bool updateGroove(const std::string& featureName, double angleDeg);

    /// UX-008: rich update covering SideType + method + length2. Returns
    /// true on success. Non-parametric (OCCT fallback) features return
    /// false — the caller should fall back to delete-and-recreate.
    bool updatePadEx(const std::string& featureName, const PadOptions& opts);
    bool updatePocketEx(const std::string& featureName, const PadOptions& opts);

    // ── Dress-up features ───────────────────────────────────────────
    std::string fillet(const std::vector<std::string>& edgeRefs, double radius);
    std::string chamfer(const std::vector<std::string>& edgeRefs, double size);

    // ── Pattern features ────────────────────────────────────────────
    std::string linearPattern(const std::string& featureName,
                              double dirX, double dirY, double dirZ,
                              double length, int occurrences);
    std::string polarPattern(const std::string& featureName,
                             double axisX, double axisY, double axisZ,
                             double angleDeg, int occurrences);
    std::string mirror(const std::string& featureName,
                       double planeNormX, double planeNormY, double planeNormZ);

    // ── Groove (subtractive revolution) ────────────────────────────
    std::string groove(const std::string& sketchName, double angleDeg);

    // ── Boolean operations ─────────────────────────────────────────
    std::string booleanFuse(const std::string& baseName, const std::string& toolName);
    std::string booleanCut(const std::string& baseName, const std::string& toolName);
    std::string booleanCommon(const std::string& baseName, const std::string& toolName);

    // ── Primitives (Part module) ───────────────────────────────────
    std::string addBox(double length, double width, double height);
    std::string addCylinder(double radius, double height, double angle = 360.0);
    std::string addSphere(double radius);
    std::string addCone(double radius1, double radius2, double height);

    // ── Dress-up (all edges) ──────────────────────────────────────
    std::string filletAll(const std::string& featureName, double radius);
    std::string chamferAll(const std::string& featureName, double size);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace CADNC
