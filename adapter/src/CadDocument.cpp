#include "CadDocument.h"
#include "SketchFacade.h"
#include "PartFacade.h"

#include <cctype>
#include <cstdio>
#include <unordered_map>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectExtension.h>
#include <App/GroupExtension.h>
#include <App/OriginGroupExtension.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Type.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/AttachExtension.h>
#include <Mod/Part/App/Attacher.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumPlane.h>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_Orientation.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <gp_Pnt.hxx>

#include <cstdio>
#include <fstream>

namespace CADNC {

struct CadDocument::Impl {
    App::Document* doc = nullptr;
    // No cached Body pointer — undoing past the body creation leaves a
    // dangling pointer that we can't detect. Always scan the document.

    // Ensure a PartDesign::Body exists in the document.
    //
    // BUG-010 root cause: Two failure modes conspired.
    //   (1) `new PartDesign::Body()` + `addObject(body, name)` skips
    //       `_addObject(DoSetup=true)`. Without it, `setupObject()` is never
    //       called, so `OriginGroupExtension::onExtendedSetupObject()` doesn't
    //       run and the Body has no Origin. Later `Body::execute()` throws.
    //   (2) The string factory `addObject("PartDesign::Body", name)` calls
    //       `Type::getTypeIfDerivedFrom(…, loadModule=true)`, which invokes
    //       `Interpreter().loadModule("PartDesign")` — a Python-level package
    //       we don't ship. That raises `Base::PyException`.
    //
    // Fix: skip the module-loading path by looking the type up directly
    // with `Type::fromName` (the C++ `_PartDesign` init already registered
    // the types), then `createInstance()` + `setDocument` + `_addObject`.
    // We mirror the exact steps `Document::addObject(sType, ...)` performs
    // but without the importModule call.
    PartDesign::Body* ensureBody() {
        if (!doc) return nullptr;

        // Always scan — undoing past body creation invalidates any cached
        // pointer we might have held, and the user could also create the
        // body externally (e.g. from a script). Fresh lookup is cheap.
        for (auto* obj : doc->getObjects()) {
            auto* b = dynamic_cast<PartDesign::Body*>(obj);
            if (b) return b;
        }

        try {
            // Resolve the type without triggering the Python importModule().
            const Base::Type t = Base::Type::fromName("PartDesign::Body");
            if (t.isBad() || !t.isDerivedFrom(App::DocumentObject::getClassTypeId())) {
                std::fprintf(stderr, "CADNC: PartDesign::Body type not registered\n");
                return nullptr;
            }

            auto* obj = static_cast<App::DocumentObject*>(t.createInstance());
            if (!obj) {
                std::fprintf(stderr, "CADNC: PartDesign::Body createInstance failed\n");
                return nullptr;
            }

            // Register with the document — `addObject(DocumentObject*, name)`
            // calls `setDocument(this)` internally. It doesn't run
            // setupObject though, so we do that manually below.
            doc->addObject(obj, "Body");

            // Manually run the setupObject equivalent: iterate all
            // DocumentObject extensions and call their public
            // `onExtendedSetupObject`. For PartDesign::Body this invokes
            // `OriginGroupExtension::onExtendedSetupObject`, which creates
            // the Origin + three datum planes. Without it, `Body::execute()`
            // throws because `getOrigin()` returns null.
            // (We can't call `obj->setupObject()` directly — it's protected.)
            try {
                auto exts = obj->getExtensionsDerivedFromType<App::DocumentObjectExtension>();
                for (auto* ext : exts) {
                    try { ext->onExtendedSetupObject(); }
                    catch (const Base::Exception& e) {
                        std::fprintf(stderr,
                            "CADNC: extension setup threw: %s\n", e.what());
                    }
                }
            } catch (...) {
                std::fprintf(stderr, "CADNC: extension iteration failed\n");
            }

            return dynamic_cast<PartDesign::Body*>(obj);
        } catch (const Base::Exception& e) {
            std::fprintf(stderr, "CADNC: Body creation threw: %s\n", e.what());
        } catch (const std::exception& e) {
            std::fprintf(stderr, "CADNC: Body creation threw std: %s\n", e.what());
        } catch (...) {
            std::fprintf(stderr, "CADNC: Body creation threw unknown\n");
        }

        // Body unavailable — sketches will be added standalone and Pad/Pocket
        // fall back to the OCCT path in PartFacade.
        return nullptr;
    }
};

CadDocument::CadDocument(const std::string& name)
    : impl_(std::make_unique<Impl>())
{
    try {
        impl_->doc = App::GetApplication().newDocument(name.c_str(), name.c_str());
        // BUG-011: App::Document::undo()/redo() silently no-op unless the
        // undo system is explicitly enabled. FreeCAD ships it off by default
        // for headless use. We also bump the stack size so long sketch
        // editing sessions don't lose history.
        if (impl_->doc) {
            impl_->doc->setUndoMode(1);
            impl_->doc->setMaxUndoStackSize(100);
        }
    } catch (const Base::Exception& e) {
        Base::Console().error("CADNC: newDocument failed: %s\n", e.what());
    } catch (...) {
        Base::Console().error("CADNC: newDocument failed (unknown exception)\n");
    }
}

CadDocument::~CadDocument() = default;

std::string CadDocument::name() const
{
    if (!impl_->doc) return {};
    return impl_->doc->getName();
}

bool CadDocument::save(const std::string& path)
{
    if (!impl_->doc) return false;
    try {
        impl_->doc->saveAs(path.c_str());
        return true;
    } catch (...) {
        return false;
    }
}

bool CadDocument::load(const std::string& path)
{
    try {
        auto* doc = App::GetApplication().openDocument(path.c_str());
        if (doc) {
            impl_->doc = doc;
            return true;
        }
    } catch (const Base::Exception& e) {
        Base::Console().error("CADNC: Failed to open %s: %s\n", path.c_str(), e.what());
    } catch (...) {
        Base::Console().error("CADNC: Failed to open %s\n", path.c_str());
    }
    return false;
}

bool CadDocument::exportTo(const std::string& path) const
{
    if (!impl_->doc) return false;

    // Determine format from file extension
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot + 1);
        // Lowercase
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Native project format. `.cadnc` is a rebrand — the bytes on disk are
    // FreeCAD's FCStd zip, we just keep the neutral UI label.
    if (ext == "fcstd" || ext == "cadnc") {
        try {
            impl_->doc->saveCopy(path.c_str());
            return true;
        } catch (...) {
            Base::Console().error("CADNC: native save failed\n");
            return false;
        }
    }

    // For geometry formats, find the last Part::Feature with a valid shape
    Part::Feature* bestFeature = nullptr;
    for (auto* obj : impl_->doc->getObjects()) {
        auto* pf = dynamic_cast<Part::Feature*>(obj);
        if (pf && !pf->Shape.getShape().getShape().IsNull()) {
            bestFeature = pf;  // keep last valid — usually the Tip
        }
    }

    if (!bestFeature) {
        Base::Console().error("CADNC: Export failed — no shape found in document\n");
        return false;
    }

    try {
        const auto& topoShape = bestFeature->Shape.getShape();
        const TopoDS_Shape& occShape = topoShape.getShape();

        if (ext == "step" || ext == "stp") {
            topoShape.exportStep(path.c_str());
        } else if (ext == "iges" || ext == "igs" || ext == "igus") {
            // "igus" seen in the wild as a misspelling of IGES — accept
            // it so users aren't punished for a typo in the file dialog.
            topoShape.exportIges(path.c_str());
        } else if (ext == "brep" || ext == "brp") {
            topoShape.exportBrep(path.c_str());
        } else if (ext == "stl") {
            topoShape.exportStl(path.c_str(), 0.1);  // 0.1mm deflection
        } else if (ext == "obj") {
            return exportObj(occShape, path);
        } else if (ext == "dxf") {
            return exportDxf(occShape, path);
        } else if (ext == "dwg") {
            // DWG is a proprietary binary format; we can't write it
            // without ODA's SDK. Fall back to DXF with a warning so the
            // user at least gets an interoperable file.
            Base::Console().warning(
                "CADNC: DWG write not supported — writing DXF instead\n");
            std::string dxfPath = path.substr(0, path.rfind('.')) + ".dxf";
            return exportDxf(occShape, dxfPath);
        } else {
            Base::Console().error("CADNC: Unknown export format: %s\n", ext.c_str());
            return false;
        }
        return true;
    } catch (const Base::Exception& e) {
        Base::Console().error("CADNC: Export failed: %s\n", e.what());
    } catch (...) {
        Base::Console().error("CADNC: Export failed\n");
    }
    return false;
}

bool CadDocument::importFrom(const std::string& path)
{
    if (!impl_->doc) return false;

    // Determine format from file extension
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        ext = path.substr(dot + 1);
        for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    try {
        Part::TopoShape shape;

        if (ext == "step" || ext == "stp") {
            shape.importStep(path.c_str());
        } else if (ext == "iges" || ext == "igs") {
            shape.importIges(path.c_str());
        } else if (ext == "brep" || ext == "brp") {
            shape.importBrep(path.c_str());
        } else {
            Base::Console().error("CADNC: Unsupported import format: %s\n", ext.c_str());
            return false;
        }

        if (shape.getShape().IsNull()) {
            Base::Console().error("CADNC: Import produced empty shape\n");
            return false;
        }

        // Create a Part::Feature to hold the imported shape
        auto* obj = impl_->doc->addObject("Part::Feature", "Import");
        if (!obj) return false;

        auto* feature = dynamic_cast<Part::Feature*>(obj);
        if (!feature) return false;

        feature->Shape.setValue(shape);
        impl_->doc->recompute();
        return true;
    } catch (const Base::Exception& e) {
        Base::Console().error("CADNC: Import failed: %s\n", e.what());
    } catch (...) {
        Base::Console().error("CADNC: Import failed\n");
    }
    return false;
}

void* CadDocument::internalDoc() const
{
    return static_cast<void*>(impl_->doc);
}

std::vector<FeatureInfo> CadDocument::featureTree() const
{
    std::vector<FeatureInfo> tree;
    if (!impl_->doc) return tree;

    const auto& objs = impl_->doc->getObjects();

    // First pass — figure out which Pad/Pocket/Revolution/Groove (if any)
    // consumes each sketch as its Profile. FreeCAD and SolidWorks nest the
    // sketch under the feature that uses it, rather than showing them as
    // siblings under Body. We reuse DocumentObject::getInList() so the
    // relationship tracks every property type (PropertyLink, LinkSub, …)
    // that FreeCAD stores the Profile in.
    std::unordered_map<std::string, std::string> sketchConsumer;
    for (auto* obj : objs) {
        const char* tn = obj->getTypeId().getName();
        if (!tn) continue;
        std::string t(tn);
        bool consumer = t.find("Pad") != std::string::npos
                     || t.find("Pocket") != std::string::npos
                     || t.find("Revolution") != std::string::npos
                     || t.find("Groove") != std::string::npos;
        if (!consumer) continue;

        // Pull the Profile link out of any PartDesign sketch-based feature.
        auto* prof = obj->getPropertyByName("Profile");
        if (!prof) continue;
        // PropertyLink / PropertyLinkSub both derive from PropertyLinkBase,
        // which exposes getValues(). We ask generically — avoids pulling in
        // extra PartDesign headers for a cast.
        std::vector<App::DocumentObject*> links;
        auto* asBase = dynamic_cast<App::PropertyLinkBase*>(prof);
        if (asBase) asBase->getLinks(links);
        for (auto* l : links) {
            if (!l) continue;
            auto* sk = dynamic_cast<Sketcher::SketchObject*>(l);
            if (!sk) continue;
            // First writer wins — usually there is only one feature per sketch
            // anyway; this just keeps the tree deterministic if not.
            sketchConsumer.emplace(sk->getNameInDocument(),
                                   obj->getNameInDocument());
        }
    }

    for (auto* obj : objs) {
        FeatureInfo info;
        info.name = obj->getNameInDocument();
        info.label = obj->Label.getValue();
        info.typeName = obj->getTypeId().getName();

        // Resolve the owning Group (Body/Origin) via GroupExtension — this is
        // the same lookup FreeCAD uses for its tree view. Returns empty if
        // the object is at the document top level.
        if (auto* parent = App::GroupExtension::getGroupOfObject(obj)) {
            info.parent = parent->getNameInDocument();
        }

        // Sketch nesting: if this is a sketch consumed by a PartDesign
        // feature, reparent it under that feature — matches FreeCAD /
        // SolidWorks tree layout. The GroupExtension-derived Body parent
        // is overridden on purpose; the consumer feature is itself a
        // child of Body so the sketch ends up at Body > Pad > Sketch.
        if (info.typeName.find("Sketcher::SketchObject") != std::string::npos) {
            auto it = sketchConsumer.find(info.name);
            if (it != sketchConsumer.end())
                info.parent = it->second;
        }

        tree.push_back(std::move(info));
    }
    return tree;
}

int CadDocument::featureCount() const
{
    if (!impl_->doc) return 0;
    return static_cast<int>(impl_->doc->getObjects().size());
}

bool CadDocument::deleteFeature(const std::string& name)
{
    if (!impl_->doc) return false;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return false;

    try {
        impl_->doc->removeObject(name.c_str());
        impl_->doc->recompute();
        return true;
    } catch (const Base::Exception& e) {
        Base::Console().error("CADNC: Delete failed: %s\n", e.what());
    } catch (...) {
        Base::Console().error("CADNC: Delete failed\n");
    }
    return false;
}

bool CadDocument::renameFeature(const std::string& name, const std::string& newLabel)
{
    if (!impl_->doc) return false;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return false;

    obj->Label.setValue(newLabel);
    return true;
}

bool CadDocument::isModified() const
{
    if (!impl_->doc) return false;
    // FreeCAD's `isTouched()` flips true on any property write and resets
    // after a successful save. Use that as our single source of truth
    // rather than tracking mutations ourselves in the adapter.
    return impl_->doc->isTouched();
}

std::string CadDocument::duplicateFeature(const std::string& name)
{
    if (!impl_->doc) return {};
    auto* src = impl_->doc->getObject(name.c_str());
    if (!src) return {};

    // FreeCAD's copyObject deep-clones the object and its link dependencies,
    // registering the clones in the target document. Pass the source doc as
    // target so the copy lands alongside the original.
    std::vector<App::DocumentObject*> srcs{src};
    auto copies = impl_->doc->copyObject(srcs, /*recursive=*/true);
    if (copies.empty()) return {};

    auto* dup = copies.front();
    // Mirror the Label so the tree shows "Pad (copy)" rather than the raw
    // internal name — users navigate by label, not by internal id.
    std::string baseLabel = src->Label.getValue();
    dup->Label.setValue((baseLabel + " (copy)").c_str());
    impl_->doc->recompute();
    return dup->getNameInDocument();
}

bool CadDocument::toggleVisibility(const std::string& name)
{
    if (!impl_->doc) return false;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return false;
    // Visibility is a standard App::PropertyBool on every DocumentObject.
    // We flip it here; the viewport re-reads it during updateViewportShapes.
    bool now = !obj->Visibility.getValue();
    obj->Visibility.setValue(now);
    return now;
}

bool CadDocument::isVisible(const std::string& name) const
{
    if (!impl_->doc) return false;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return false;
    return obj->Visibility.getValue();
}

bool CadDocument::setVisibility(const std::string& name, bool visible)
{
    if (!impl_->doc) return false;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return false;
    obj->Visibility.setValue(visible);
    return true;
}

std::vector<std::string> CadDocument::dependents(const std::string& name) const
{
    std::vector<std::string> out;
    if (!impl_->doc) return out;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return out;
    for (auto* dep : obj->getInList()) {
        if (dep && dep->getNameInDocument())
            out.emplace_back(dep->getNameInDocument());
    }
    return out;
}

void CadDocument::undo()
{
    if (impl_->doc) impl_->doc->undo();
}

void CadDocument::redo()
{
    if (impl_->doc) impl_->doc->redo();
}

bool CadDocument::canUndo() const
{
    return impl_->doc && impl_->doc->getAvailableUndos() > 0;
}

bool CadDocument::canRedo() const
{
    return impl_->doc && impl_->doc->getAvailableRedos() > 0;
}

void CadDocument::openTransaction(const char* name)
{
    if (impl_->doc) impl_->doc->openTransaction(name ? name : "Action");
}

void CadDocument::commitTransaction()
{
    if (impl_->doc) impl_->doc->commitTransaction();
}

void CadDocument::abortTransaction()
{
    if (impl_->doc) impl_->doc->abortTransaction();
}

std::shared_ptr<SketchFacade> CadDocument::addSketch(const std::string& name, int planeType)
{
    if (!impl_->doc) return nullptr;

    // Ensure a PartDesign::Body exists (required for Pad/Pocket/Revolution)
    auto* body = impl_->ensureBody();

    auto* obj = impl_->doc->addObject("Sketcher::SketchObject", name.c_str());
    if (!obj) return nullptr;

    // Add sketch to the Body so PartDesign features can find it
    if (body) {
        body->addObject(obj);
    }

    // Set sketch placement based on plane type:
    // 0 = XY (default, no rotation)
    // 1 = XZ (rotate -90° around X)
    // 2 = YZ (rotate 90° around Z, then -90° around X)
    if (planeType != 0) {
        auto* prop = dynamic_cast<App::PropertyPlacement*>(
            obj->getPropertyByName("Placement"));
        if (prop) {
            Base::Placement placement;
            if (planeType == 1) {
                placement.setRotation(Base::Rotation(Base::Vector3d(1, 0, 0), -M_PI / 2.0));
            } else if (planeType == 2) {
                Base::Rotation r1(Base::Vector3d(0, 0, 1), M_PI / 2.0);
                Base::Rotation r2(Base::Vector3d(1, 0, 0), -M_PI / 2.0);
                placement.setRotation(r2 * r1);
            }
            prop->setValue(placement);
        }
    }

    return std::make_shared<SketchFacade>(static_cast<void*>(obj));
}

std::shared_ptr<SketchFacade> CadDocument::addSketchOnPlane(const std::string& name,
                                                             const std::string& planeFeatureName)
{
    if (!impl_->doc || planeFeatureName.empty()) return nullptr;

    auto* plane = impl_->doc->getObject(planeFeatureName.c_str());
    if (!plane) return nullptr;

    auto* body = impl_->ensureBody();
    auto* obj = impl_->doc->addObject("Sketcher::SketchObject", name.c_str());
    if (!obj) return nullptr;
    if (body) body->addObject(obj);

    auto* attach = dynamic_cast<Part::AttachExtension*>(
        obj->getExtension(Part::AttachExtension::getExtensionClassTypeId()));
    if (!attach) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return nullptr;
    }

    try {
        // Attach directly to the plane — no sub-element needed because the
        // plane *is* the planar face. mmFlatFace matches the face normal
        // to the sketch's local Z on recompute.
        std::vector<App::DocumentObject*> supports = {plane};
        std::vector<std::string> subs = {""};
        attach->AttachmentSupport.setValues(supports, subs);
        attach->MapMode.setValue(Attacher::mmFlatFace);
        impl_->doc->recompute();
    } catch (...) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return nullptr;
    }

    return std::make_shared<SketchFacade>(static_cast<void*>(obj));
}

std::shared_ptr<SketchFacade> CadDocument::addSketchOnFace(const std::string& name,
                                                            const std::string& featureName,
                                                            const std::string& subElement)
{
    if (!impl_->doc || featureName.empty() || subElement.empty()) return nullptr;

    // Target feature must exist and carry a non-null shape so the attacher
    // has something to anchor on.
    auto* support = impl_->doc->getObject(featureName.c_str());
    if (!support) return nullptr;
    auto* supportPart = dynamic_cast<Part::Feature*>(support);
    if (!supportPart || supportPart->Shape.getShape().getShape().IsNull()) return nullptr;

    auto* body = impl_->ensureBody();

    auto* obj = impl_->doc->addObject("Sketcher::SketchObject", name.c_str());
    if (!obj) return nullptr;
    if (body) body->addObject(obj);

    // Sketcher::SketchObject inherits Part::Part2DObject which owns an
    // AttachExtension. Setting AttachmentSupport + MapMode and recomputing
    // makes FreeCAD compute the sketch placement from the face normal +
    // tangent frame (matches Sketcher's "Sketch on face" in the GUI).
    auto* attachable = dynamic_cast<Part::AttachExtension*>(
        obj->getExtension(Part::AttachExtension::getExtensionClassTypeId()));
    if (!attachable) {
        // Fallback: roll back — the sketch isn't attachable, which shouldn't
        // happen for SketchObject but guard anyway.
        impl_->doc->removeObject(obj->getNameInDocument());
        return nullptr;
    }

    try {
        std::vector<App::DocumentObject*> supports = {support};
        std::vector<std::string> subs = {subElement};
        attachable->AttachmentSupport.setValues(supports, subs);
        attachable->MapMode.setValue(Attacher::mmFlatFace);
        impl_->doc->recompute();
    } catch (...) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return nullptr;
    }

    return std::make_shared<SketchFacade>(static_cast<void*>(obj));
}

std::string CadDocument::addDatumPlane(int basePlane, double offset, const std::string& label)
{
    return addDatumPlaneRotated(basePlane, offset, 0.0, 0.0, label);
}

std::string CadDocument::addDatumPlaneRotated(int basePlane, double offset,
                                                double rotXDeg, double rotYDeg,
                                                const std::string& label)
{
    if (!impl_->doc) return {};
    auto* body = impl_->ensureBody();

    auto* obj = impl_->doc->addObject("PartDesign::Plane", label.c_str());
    if (!obj) return {};
    if (body) body->addObject(obj);

    auto* attach = dynamic_cast<Part::AttachExtension*>(
        obj->getExtension(Part::AttachExtension::getExtensionClassTypeId()));
    if (!attach) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return {};
    }

    Attacher::eMapMode mode = Attacher::mmObjectXY;
    switch (basePlane) {
        case 1: mode = Attacher::mmObjectXZ; break;
        case 2: mode = Attacher::mmObjectYZ; break;
        default: mode = Attacher::mmObjectXY; break;
    }

    try {
        attach->MapMode.setValue(mode);
        Base::Placement pl = attach->AttachmentOffset.getValue();
        Base::Vector3d p = pl.getPosition();
        p.z = offset;
        pl.setPosition(p);

        // Compose rotation around local X and Y. Order: X first, then Y
        // — matches FreeCAD's TaskAttacher UI ordering.
        if (std::abs(rotXDeg) > 1e-9 || std::abs(rotYDeg) > 1e-9) {
            Base::Rotation rx(Base::Vector3d(1, 0, 0), rotXDeg * M_PI / 180.0);
            Base::Rotation ry(Base::Vector3d(0, 1, 0), rotYDeg * M_PI / 180.0);
            pl.setRotation(ry * rx);
        }

        attach->AttachmentOffset.setValue(pl);
        impl_->doc->recompute();
    } catch (...) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return {};
    }

    return obj->getNameInDocument();
}

std::string CadDocument::addDatumPlaneOnFace(const std::string& featureName,
                                               const std::string& faceName,
                                               double offset,
                                               const std::string& label)
{
    if (!impl_->doc || featureName.empty() || faceName.empty()) return {};

    auto* support = impl_->doc->getObject(featureName.c_str());
    if (!support) return {};

    auto* body = impl_->ensureBody();
    auto* obj = impl_->doc->addObject("PartDesign::Plane", label.c_str());
    if (!obj) return {};
    if (body) body->addObject(obj);

    auto* attach = dynamic_cast<Part::AttachExtension*>(
        obj->getExtension(Part::AttachExtension::getExtensionClassTypeId()));
    if (!attach) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return {};
    }

    try {
        std::vector<App::DocumentObject*> supports = {support};
        std::vector<std::string> subs = {faceName};
        attach->AttachmentSupport.setValues(supports, subs);
        attach->MapMode.setValue(Attacher::mmFlatFace);

        if (std::abs(offset) > 1e-9) {
            Base::Placement pl = attach->AttachmentOffset.getValue();
            Base::Vector3d p = pl.getPosition();
            p.z = offset;  // local Z = face normal in mmFlatFace
            pl.setPosition(p);
            attach->AttachmentOffset.setValue(pl);
        }
        impl_->doc->recompute();
    } catch (...) {
        impl_->doc->removeObject(obj->getNameInDocument());
        return {};
    }

    return obj->getNameInDocument();
}

std::vector<std::string> CadDocument::featureFaceNames(const std::string& featureName) const
{
    std::vector<std::string> out;
    if (!impl_->doc) return out;
    auto* obj = impl_->doc->getObject(featureName.c_str());
    if (!obj) return out;
    auto* feat = dynamic_cast<Part::Feature*>(obj);
    if (!feat) return out;

    TopoDS_Shape shape = feat->Shape.getShape().getShape();
    if (shape.IsNull()) return out;

    // Iterate TopAbs_FACE children. Sub-name convention "Face1", "Face2"...
    // (1-based). Matches what PartDesign/Sketcher uses internally so the
    // same string can be passed back to AttachmentSupport.
    int i = 1;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next(), ++i) {
        out.emplace_back("Face" + std::to_string(i));
    }
    return out;
}

std::shared_ptr<SketchFacade> CadDocument::getSketch(const std::string& name)
{
    if (!impl_->doc) return nullptr;

    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return nullptr;

    auto* sketch = dynamic_cast<Sketcher::SketchObject*>(obj);
    if (!sketch) return nullptr;

    return std::make_shared<SketchFacade>(static_cast<void*>(sketch));
}

std::shared_ptr<PartFacade> CadDocument::partDesign()
{
    if (!impl_->doc) return nullptr;
    return std::make_shared<PartFacade>(static_cast<void*>(impl_->doc));
}

void* CadDocument::getFeatureShape(const std::string& name) const
{
    if (!impl_->doc) return nullptr;
    auto* obj = impl_->doc->getObject(name.c_str());
    if (!obj) return nullptr;

    // Return pointer to Part::Feature itself (not the transient TopoDS_Shape).
    // Caller extracts the shape on the render-thread side where it's safe.
    auto* partFeature = dynamic_cast<Part::Feature*>(obj);
    if (partFeature && !partFeature->Shape.getShape().getShape().IsNull()) {
        return static_cast<void*>(partFeature);
    }
    return nullptr;
}

void CadDocument::recompute()
{
    if (impl_->doc) impl_->doc->recompute();
}

void CadDocument::recomputeIfNeeded()
{
    if (!impl_->doc) return;
    // FreeCAD's Document::isTouched() aggregates every DocumentObject's
    // touched flag; a clean doc means nothing needs recomputing.
    if (!impl_->doc->isTouched()) return;
    impl_->doc->recompute();
}

// ── OBJ writer ──────────────────────────────────────────────────────
// Wavefront OBJ via OCCT tessellation. We refine the mesh with
// BRepMesh_IncrementalMesh using a modest linear deflection (0.1mm)
// and a 0.5rad angular deflection so curved faces come out smooth.
bool CadDocument::exportObj(const TopoDS_Shape& shape, const std::string& path) const
{
    if (shape.IsNull()) return false;

    try {
        BRepMesh_IncrementalMesh mesher(shape, 0.1, false, 0.5, true);
        mesher.Perform();

        std::ofstream out(path);
        if (!out) return false;
        out << "# CADNC OBJ export\n";

        int vertexBase = 1;  // OBJ indices are 1-based
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull()) continue;

            gp_Trsf trsf = loc.Transformation();
            int n = tri->NbNodes();
            for (int i = 1; i <= n; ++i) {
                gp_Pnt p = tri->Node(i).Transformed(trsf);
                out << "v " << p.X() << ' ' << p.Y() << ' ' << p.Z() << '\n';
            }
            int t = tri->NbTriangles();
            bool flip = (face.Orientation() == TopAbs_REVERSED);
            for (int i = 1; i <= t; ++i) {
                int n1, n2, n3;
                tri->Triangle(i).Get(n1, n2, n3);
                if (flip) std::swap(n1, n2);
                out << "f " << (vertexBase + n1 - 1) << ' '
                           << (vertexBase + n2 - 1) << ' '
                           << (vertexBase + n3 - 1) << '\n';
            }
            vertexBase += n;
        }
        return true;
    } catch (...) {
        Base::Console().error("CADNC: OBJ export failed\n");
        return false;
    }
}

// ── DXF writer (R12 ASCII, LINE-only) ───────────────────────────────
// Minimal AutoCAD R12 file: LINE entities only, no layers/blocks. Each
// edge is sampled by GCPnts_TangentialDeflection so curves become
// polylines fine-grained enough to be visually identical to the model.
// Good enough for 2D downstream (laser cut, CAM import).
bool CadDocument::exportDxf(const TopoDS_Shape& shape, const std::string& path) const
{
    if (shape.IsNull()) return false;
    try {
        std::ofstream out(path);
        if (!out) return false;

        // DXF R12 header (smallest valid skeleton AutoCAD accepts)
        out << "0\nSECTION\n2\nHEADER\n0\nENDSEC\n"
            << "0\nSECTION\n2\nENTITIES\n";

        auto emitLine = [&](const gp_Pnt& a, const gp_Pnt& b) {
            out << "0\nLINE\n8\n0\n"
                << "10\n" << a.X() << "\n20\n" << a.Y() << "\n30\n" << a.Z() << '\n'
                << "11\n" << b.X() << "\n21\n" << b.Y() << "\n31\n" << b.Z() << '\n';
        };

        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
            try {
                BRepAdaptor_Curve curve(edge);
                GCPnts_TangentialDeflection sampler(curve, 0.1, 0.05);
                int nb = sampler.NbPoints();
                if (nb < 2) continue;
                gp_Pnt prev = sampler.Value(1);
                for (int i = 2; i <= nb; ++i) {
                    gp_Pnt cur = sampler.Value(i);
                    emitLine(prev, cur);
                    prev = cur;
                }
            } catch (...) { /* skip this edge */ }
        }

        out << "0\nENDSEC\n0\nEOF\n";
        return true;
    } catch (...) {
        Base::Console().error("CADNC: DXF export failed\n");
        return false;
    }
}

} // namespace CADNC
