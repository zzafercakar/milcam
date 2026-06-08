#pragma once

/**
 * @file CadDocument.h
 * @brief Wrapper around FreeCAD App::Document.
 *
 * Provides a clean interface for document operations without exposing
 * FreeCAD types to the UI layer. All FreeCAD interactions go through
 * opaque pointers resolved in the .cpp file.
 */

#include <memory>
#include <string>
#include <vector>

// Forward declaration at global scope — OCCT types live outside CADNC's
// namespace. Keeps the header free of heavy OCCT includes while still
// letting `exportObj`/`exportDxf` receive the shape by reference.
class TopoDS_Shape;

namespace CADNC {

class SketchFacade;
class PartFacade;

/// Minimal info about a feature in the document tree
struct FeatureInfo {
    std::string name;       // internal name (e.g. "Sketch001")
    std::string label;      // user-visible label
    std::string typeName;   // e.g. "Sketcher::SketchObject"
    std::string parent;     // parent group name (Body/Origin) — empty if top-level
};

class CadDocument {
public:
    /// Create a new FreeCAD document with the given name
    explicit CadDocument(const std::string& name);
    ~CadDocument();

    // non-copyable
    CadDocument(const CadDocument&) = delete;
    CadDocument& operator=(const CadDocument&) = delete;

    /// Document name (internal)
    std::string name() const;

    // ── Persistence ─────────────────────────────────────────────────
    bool save(const std::string& path);
    bool load(const std::string& path);

    /// Export document to format determined by extension (STEP, IGES, STL, BREP, etc.)
    bool exportTo(const std::string& path) const;

    /// Import geometry from STEP/IGES/BREP file into the document
    bool importFrom(const std::string& path);

    /// Get the internal App::Document pointer (opaque)
    void* internalDoc() const;

    // ── Feature tree ────────────────────────────────────────────────
    std::vector<FeatureInfo> featureTree() const;
    int featureCount() const;
    bool deleteFeature(const std::string& name);
    bool renameFeature(const std::string& name, const std::string& newLabel);
    /// Deep-clone a feature (copyObject) and return the new internal name.
    /// Empty string on failure.
    std::string duplicateFeature(const std::string& name);

    /// True when the document has unsaved changes. Wraps FreeCAD's
    /// `App::Document::isTouched()` which is set on any property mutation
    /// and cleared on save. Used by the UI to prompt the user before
    /// close/new if they haven't saved their work.
    bool isModified() const;

    /// Toggle the feature's `Visibility` property (FreeCAD PropertyBool).
    /// Returns the new state, or false for unknown names. The viewport
    /// reads this on the next `updateViewportShapes()` pass.
    bool toggleVisibility(const std::string& name);
    bool isVisible(const std::string& name) const;
    /// Force a specific visibility value. Returns true if applied. Use this
    /// (rather than toggleVisibility) when the desired state is known
    /// — e.g. hiding a sketch during edit and restoring on close.
    bool setVisibility(const std::string& name, bool visible);

    /// List names of features that depend on the given name (via
    /// App::DocumentObject::getInList). Used by the tree context menu's
    /// "Show dependencies" action.
    std::vector<std::string> dependents(const std::string& name) const;

    // ── Undo / Redo ─────────────────────────────────────────────────
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    /// Begin a transaction; property changes until commitTransaction() end up
    /// as one undo entry. Pass a human-readable name ("Add Line" etc.) — it
    /// appears in the undo history. No-op if the document is null.
    void openTransaction(const char* name);
    void commitTransaction();
    void abortTransaction();

    // ── Sketch operations ───────────────────────────────────────────
    /// Add a new sketch and return a facade. planeType: 0=XY, 1=XZ, 2=YZ
    std::shared_ptr<SketchFacade> addSketch(const std::string& name = "Sketch", int planeType = 0);

    /// Add a sketch attached to a face of an existing feature. subElement is
    /// a FreeCAD sub-name like "Face1". Uses Attacher::mmFlatFace so the
    /// sketch's placement is derived from the face on recompute.
    std::shared_ptr<SketchFacade> addSketchOnFace(const std::string& name,
                                                   const std::string& featureName,
                                                   const std::string& subElement);

    /// Add a sketch attached to a (datum) plane — either an App::Plane
    /// under Origin or a PartDesign::Plane under the Body. Uses the same
    /// AttachExtension plumbing as `addSketchOnFace` but with mmFlatFace
    /// pointing at the plane itself (no sub-element needed).
    std::shared_ptr<SketchFacade> addSketchOnPlane(const std::string& name,
                                                    const std::string& planeFeatureName);

    /// List "Face1".."FaceN" sub-names of a feature's shape, in TopExp
    /// iteration order (matches FreeCAD's sub-element naming convention).
    std::vector<std::string> featureFaceNames(const std::string& featureName) const;

    /// Add a user-defined datum plane parallel to one of the three base
    /// planes (0=XY, 1=XZ, 2=YZ) at the given offset in mm. Returns the
    /// new plane's internal name (empty on failure). The plane lives
    /// under the active Body so `createSketch(name, -1, datumName)` can
    /// later attach a sketch to it.
    std::string addDatumPlane(int basePlane, double offset, const std::string& label = "DatumPlane");

    /// Rich datum plane with rotation (degrees) around the reference
    /// plane's local X/Y axes and an offset. Used by the enriched
    /// "New Datum Plane" dialog. `rotXDeg` tilts around local X,
    /// `rotYDeg` tilts around local Y — matches FreeCAD's
    /// AttachmentOffset convention.
    std::string addDatumPlaneRotated(int basePlane, double offset,
                                       double rotXDeg, double rotYDeg,
                                       const std::string& label = "DatumPlane");

    /// Datum plane attached to an existing feature's face. `faceName`
    /// should be "Face1"..."FaceN" (see featureFaceNames). The plane
    /// lies on the face plus an optional offset along its normal.
    std::string addDatumPlaneOnFace(const std::string& featureName,
                                     const std::string& faceName,
                                     double offset,
                                     const std::string& label = "DatumPlane");

    /// Get a facade for an existing sketch by name
    std::shared_ptr<SketchFacade> getSketch(const std::string& name);

    // ── Part operations ─────────────────────────────────────────────
    /// Get a facade for PartDesign operations on this document
    std::shared_ptr<PartFacade> partDesign();

    // ── Shape access ──────────────────────────────────────────────
    /// Get the TopoDS_Shape of a named feature (returns as opaque void*)
    /// Returns nullptr if feature not found or has no shape.
    void* getFeatureShape(const std::string& name) const;

    // ── Recompute ───────────────────────────────────────────────────
    void recompute();

    /// Fast-path wrapper: skips the recompute call entirely if the
    /// document has no touched objects. Per WRAPPER_CONTRACT § 2.6,
    /// mutating facade/engine paths should prefer this over the
    /// unconditional recompute().
    void recomputeIfNeeded();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // OBJ/DXF exporters — take the OCCT shape by reference (forward-
    // declared at global scope above). The definitions pull in the full
    // TopoDS_Shape header inside the .cpp.
    bool exportObj(const ::TopoDS_Shape& shape, const std::string& path) const;
    bool exportDxf(const ::TopoDS_Shape& shape, const std::string& path) const;
};

} // namespace CADNC
