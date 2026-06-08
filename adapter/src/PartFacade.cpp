#include "PartFacade.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectExtension.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Type.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/FeatureExtrusion.h>
#include <Mod/Part/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePad.h>
#include <Mod/PartDesign/App/FeaturePocket.h>
#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureFillet.h>
#include <Mod/PartDesign/App/FeatureChamfer.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/Part/App/FeaturePartBoolean.h>
#include <Mod/Part/App/FeaturePartCut.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/FeaturePartCommon.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

// OCCT — manual face/prism construction when PartDesign::Body is unavailable
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Builder.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <vector>

namespace CADNC {

struct PartFacade::Impl {
    App::Document* doc = nullptr;

    // Find PartDesign::Body if it exists
    PartDesign::Body* findBody() {
        if (!doc) return nullptr;
        for (auto* obj : doc->getObjects()) {
            auto* b = dynamic_cast<PartDesign::Body*>(obj);
            if (b) return b;
        }
        return nullptr;
    }

    // Find an existing Body, or create one. The string factory
    // (`addObject("PartDesign::Body", …)`) would load the Python `PartDesign`
    // package, which we don't ship — it throws `Base::PyException`. Instead
    // we resolve the type directly (C++ `_PartDesign` init already registered
    // it) and mirror the rest of the DoSetup path manually.
    PartDesign::Body* findOrCreateBody() {
        auto* existing = findBody();
        if (existing) return existing;
        if (!doc) return nullptr;

        try {
            const Base::Type t = Base::Type::fromName("PartDesign::Body");
            if (t.isBad()) return nullptr;

            auto* obj = static_cast<App::DocumentObject*>(t.createInstance());
            if (!obj) return nullptr;
            // addObject(DocumentObject*, name) calls setDocument internally.
            doc->addObject(obj, "Body");

            // Runs OriginGroupExtension::onExtendedSetupObject → creates the
            // Origin + datum planes so `Body::execute()` / `getOrigin()` work.
            // `setupObject()` itself is protected; we iterate extensions and
            // call the public `onExtendedSetupObject` directly instead.
            try {
                auto exts = obj->getExtensionsDerivedFromType<App::DocumentObjectExtension>();
                for (auto* ext : exts) {
                    try { ext->onExtendedSetupObject(); } catch (...) {}
                }
            } catch (...) {}

            return dynamic_cast<PartDesign::Body*>(obj);
        } catch (const Base::Exception& e) {
            Base::Console().warning("CADNC: Body creation failed: %s\n", e.what());
        } catch (...) {
            Base::Console().warning("CADNC: Body creation failed (unknown)\n");
        }
        return nullptr;
    }
};

// ── OCCT helpers (used when PartDesign::Body is unavailable) ──────────
namespace {

// Pull all edges from a shape (sketch shape is a compound of edges)
static void collectEdges(const TopoDS_Shape& s, TopTools_HSequenceOfShape& out) {
    for (TopExp_Explorer exp(s, TopAbs_EDGE); exp.More(); exp.Next()) {
        out.Append(exp.Current());
    }
}

// Build closed wires from a sketch's loose edges, then build faces.
// Returns a TopoDS_Compound of TopoDS_Face (one per closed wire).
// Empty compound on failure.
static TopoDS_Shape buildFacesFromSketchShape(const TopoDS_Shape& sketchShape) {
    TopoDS_Compound result;
    BRep_Builder builder;
    builder.MakeCompound(result);

    Handle(TopTools_HSequenceOfShape) edges = new TopTools_HSequenceOfShape();
    for (TopExp_Explorer exp(sketchShape, TopAbs_EDGE); exp.More(); exp.Next())
        edges->Append(exp.Current());
    if (edges->IsEmpty()) return result;

    // Tolerance: 1e-3 (mm) — generous enough for sketcher snap output
    Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edges, 1e-3, /*shared*/ Standard_False, wires);

    // Collect closed wires together with the area of their bounding face.
    // Larger area → outer boundary; smaller wires that lie inside become
    // holes in their containing outer face (rectangle-with-hole pattern).
    struct WireInfo { TopoDS_Wire wire; TopoDS_Face face; double area = 0.0; bool used = false; };
    std::vector<WireInfo> infos;
    infos.reserve(wires->Length());
    for (Standard_Integer i = 1; i <= wires->Length(); ++i) {
        TopoDS_Wire w = TopoDS::Wire(wires->Value(i));
        if (!w.Closed()) continue;
        try {
            BRepBuilderAPI_MakeFace mf(w, /*OnlyPlane*/ Standard_True);
            if (!mf.IsDone()) continue;
            TopoDS_Face f = mf.Face();
            GProp_GProps props;
            BRepGProp::SurfaceProperties(f, props);
            infos.push_back({w, f, std::abs(props.Mass()), false});
        } catch (...) {}
    }
    // Sort largest-first so each candidate-outer is processed before its holes.
    std::sort(infos.begin(), infos.end(),
              [](const WireInfo& a, const WireInfo& b){ return a.area > b.area; });

    auto pointInsideBox = [](const Bnd_Box& outer, const Bnd_Box& inner) -> bool {
        if (outer.IsVoid() || inner.IsVoid()) return false;
        Standard_Real ox1, oy1, oz1, ox2, oy2, oz2;
        Standard_Real ix1, iy1, iz1, ix2, iy2, iz2;
        outer.Get(ox1, oy1, oz1, ox2, oy2, oz2);
        inner.Get(ix1, iy1, iz1, ix2, iy2, iz2);
        return ix1 >= ox1 - 1e-6 && iy1 >= oy1 - 1e-6 &&
               ix2 <= ox2 + 1e-6 && iy2 <= oy2 + 1e-6;
    };

    bool any = false;
    for (size_t i = 0; i < infos.size(); ++i) {
        if (infos[i].used) continue;
        infos[i].used = true;
        try {
            BRepBuilderAPI_MakeFace mf(infos[i].wire, /*OnlyPlane*/ Standard_True);
            if (!mf.IsDone()) continue;
            Bnd_Box outerBox; BRepBndLib::Add(infos[i].face, outerBox);
            // Add any smaller wire fully contained in outer's bbox as a hole.
            for (size_t j = i + 1; j < infos.size(); ++j) {
                if (infos[j].used) continue;
                Bnd_Box innerBox; BRepBndLib::Add(infos[j].face, innerBox);
                if (!pointInsideBox(outerBox, innerBox)) continue;
                TopoDS_Wire holeWire = infos[j].wire;
                holeWire.Reverse();  // hole = reversed orientation
                mf.Add(holeWire);
                infos[j].used = true;
            }
            if (mf.IsDone()) { builder.Add(result, mf.Face()); any = true; }
        } catch (...) {}
    }
    if (!any) return TopoDS_Compound();
    return result;
}

// Manual extrusion via OCCT — used when PartDesign::Pad is not available.
// Returns a Part::Feature wrapping the resulting prism (solid), or nullptr.
static App::DocumentObject* makePrismFeature(App::Document* doc,
                                              App::DocumentObject* sketch,
                                              const gp_Vec& dir,
                                              const std::string& featureName)
{
    auto* sketchFeat = dynamic_cast<Part::Feature*>(sketch);
    if (!sketchFeat) return nullptr;

    TopoDS_Shape sketchShape = sketchFeat->Shape.getShape().getShape();
    if (sketchShape.IsNull()) return nullptr;

    TopoDS_Shape faces = buildFacesFromSketchShape(sketchShape);
    if (faces.IsNull()) return nullptr;

    // Extrude every face. Compound the resulting solids.
    TopoDS_Compound solids;
    BRep_Builder builder;
    builder.MakeCompound(solids);
    bool any = false;
    for (TopExp_Explorer exp(faces, TopAbs_FACE); exp.More(); exp.Next()) {
        try {
            BRepPrimAPI_MakePrism prism(exp.Current(), dir, /*Copy*/ Standard_False, /*Canonize*/ Standard_True);
            if (prism.IsDone()) { builder.Add(solids, prism.Shape()); any = true; }
        } catch (...) { /* skip */ }
    }
    if (!any) return nullptr;

    auto* feat = doc->addObject("Part::Feature", featureName.c_str());
    auto* pf = dynamic_cast<Part::Feature*>(feat);
    if (!pf) { if (feat) doc->removeObject(feat->getNameInDocument()); return nullptr; }
    pf->Shape.setValue(solids);
    return feat;
}

// Manual revolution — used when PartDesign::Revolution is unavailable.
static App::DocumentObject* makeRevolutionFeature(App::Document* doc,
                                                   App::DocumentObject* sketch,
                                                   const gp_Ax1& axis,
                                                   double angleRad,
                                                   const std::string& featureName)
{
    auto* sketchFeat = dynamic_cast<Part::Feature*>(sketch);
    if (!sketchFeat) return nullptr;

    TopoDS_Shape sketchShape = sketchFeat->Shape.getShape().getShape();
    if (sketchShape.IsNull()) return nullptr;

    TopoDS_Shape faces = buildFacesFromSketchShape(sketchShape);
    if (faces.IsNull()) return nullptr;

    TopoDS_Compound solids;
    BRep_Builder builder;
    builder.MakeCompound(solids);
    bool any = false;
    for (TopExp_Explorer exp(faces, TopAbs_FACE); exp.More(); exp.Next()) {
        try {
            BRepPrimAPI_MakeRevol rev(exp.Current(), axis, angleRad, Standard_False);
            if (rev.IsDone()) { builder.Add(solids, rev.Shape()); any = true; }
        } catch (...) {}
    }
    if (!any) return nullptr;

    auto* feat = doc->addObject("Part::Feature", featureName.c_str());
    auto* pf = dynamic_cast<Part::Feature*>(feat);
    if (!pf) { if (feat) doc->removeObject(feat->getNameInDocument()); return nullptr; }
    pf->Shape.setValue(solids);
    return feat;
}

} // anonymous namespace

PartFacade::PartFacade(void* document)
    : impl_(std::make_unique<Impl>())
{
    impl_->doc = static_cast<App::Document*>(document);
}

PartFacade::~PartFacade() = default;

std::string PartFacade::pad(const std::string& sketchName, double length)
{
    // Thin compatibility wrapper — defaults to single-side, fixed-length.
    PadOptions opts;
    opts.length = length;
    return padEx(sketchName, opts);
}

std::string PartFacade::padEx(const std::string& sketchName, const PadOptions& opts)
{
    if (!impl_->doc) return {};

    auto* sketch = impl_->doc->getObject(sketchName.c_str());
    if (!sketch) {
        std::fprintf(stderr, "CADNC: Pad failed — sketch '%s' not found\n", sketchName.c_str());
        return {};
    }

    // Try PartDesign::Pad first (proper Body chain).
    // findOrCreateBody() will instantiate one if missing.
    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* padObj = new PartDesign::Pad();
            impl_->doc->addObject(padObj, "Pad");
            padObj->Profile.setValue(sketch);
            // FeatureExtrude uses two enumerations: Type = extrusion
            // method ("Length", "ThroughAll", …), SideType = one/two/
            // symmetric. We map our free-form strings straight through
            // so user-visible labels match FreeCAD docs/tutorials.
            padObj->Type.setValue(opts.method.c_str());
            padObj->Length.setValue(opts.length);
            padObj->Length2.setValue(opts.length2);
            padObj->Reversed.setValue(opts.reversed);
            try { padObj->SideType.setValue(opts.sideType.c_str()); }
            catch (...) { /* older FreeCAD may alias to Midplane */ }
            body->addObject(padObj);
            impl_->doc->recompute();

            if (!padObj->Shape.getShape().getShape().IsNull())
                return padObj->getNameInDocument();

            impl_->doc->removeObject(padObj->getNameInDocument());
        } catch (...) {}
    }

    // Fallback: build face from sketch wires → prism via OCCT. Honours
    // Symmetric / Two sides as best we can without the PartDesign type
    // system — we extrude twice and fuse the results.
    double l1 = opts.length;
    double l2 = opts.length2;
    if (opts.reversed) { l1 = -l1; l2 = -l2; }
    if (opts.sideType == "Symmetric") {
        auto* feat = makePrismFeature(impl_->doc, sketch,
                                       gp_Vec(0, 0, l1 * 0.5 + l2 * 0.5), "Pad");
        if (!feat) return {};
        try { impl_->doc->recompute(); } catch (...) {}
        return feat->getNameInDocument();
    }
    auto* feat = makePrismFeature(impl_->doc, sketch, gp_Vec(0, 0, l1), "Pad");
    if (!feat) return {};
    try { impl_->doc->recompute(); } catch (...) {}
    return feat->getNameInDocument();
}

std::string PartFacade::pocket(const std::string& sketchName, double depth)
{
    PadOptions opts;
    opts.length = depth;
    return pocketEx(sketchName, opts);
}

std::string PartFacade::pocketEx(const std::string& sketchName, const PadOptions& opts)
{
    if (!impl_->doc) return {};

    auto* sketch = impl_->doc->getObject(sketchName.c_str());
    if (!sketch) return {};

    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* pocketObj = new PartDesign::Pocket();
            impl_->doc->addObject(pocketObj, "Pocket");
            pocketObj->Profile.setValue(sketch);
            pocketObj->Type.setValue(opts.method.c_str());
            pocketObj->Length.setValue(opts.length);
            pocketObj->Length2.setValue(opts.length2);
            pocketObj->Reversed.setValue(opts.reversed);
            try { pocketObj->SideType.setValue(opts.sideType.c_str()); }
            catch (...) {}
            body->addObject(pocketObj);
            impl_->doc->recompute();

            if (!pocketObj->Shape.getShape().getShape().IsNull())
                return pocketObj->getNameInDocument();

            impl_->doc->removeObject(pocketObj->getNameInDocument());
        } catch (...) {}
    }

    // Fallback: build face from sketch wires → prism in -Z direction.
    double d = opts.reversed ? -opts.length : opts.length;
    auto* feat = makePrismFeature(impl_->doc, sketch, gp_Vec(0, 0, -d), "Pocket");
    if (!feat) return {};
    try { impl_->doc->recompute(); } catch (...) {}
    return feat->getNameInDocument();
}

std::string PartFacade::revolution(const std::string& sketchName, double angleDeg)
{
    if (!impl_->doc) return {};

    auto* sketch = impl_->doc->getObject(sketchName.c_str());
    if (!sketch) return {};

    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* revObj = new PartDesign::Revolution();
            impl_->doc->addObject(revObj, "Revolution");
            revObj->Profile.setValue(sketch);
            revObj->Type.setValue("Angle");
            revObj->Angle.setValue(angleDeg);
            revObj->Axis.setValue(Base::Vector3d(0, 1, 0));
            revObj->Base.setValue(Base::Vector3d(0, 0, 0));
            body->addObject(revObj);
            impl_->doc->recompute();

            if (!revObj->Shape.getShape().getShape().IsNull())
                return revObj->getNameInDocument();

            impl_->doc->removeObject(revObj->getNameInDocument());
        } catch (...) {}
    }

    // Fallback: build face from sketch wires → revolve about Y axis.
    gp_Ax1 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
    double angleRad = angleDeg * 3.14159265358979323846 / 180.0;
    auto* feat = makeRevolutionFeature(impl_->doc, sketch, axis, angleRad, "Revolution");
    if (!feat) return {};
    try { impl_->doc->recompute(); } catch (...) {}
    return feat->getNameInDocument();
}

// ── Parametric edit ─────────────────────────────────────────────────
// These mutate an existing PartDesign feature's primary parameter (Length
// for Pad/Pocket, Angle for Revolution/Groove) and trigger a document
// recompute. Non-parametric OCCT-fallback features cannot be edited in
// place — getFeatureParams reports editable=false so the UI falls back
// to the create-new flow.
FeatureParams PartFacade::getFeatureParams(const std::string& featureName)
{
    FeatureParams p;
    if (!impl_->doc) return p;
    auto* obj = impl_->doc->getObject(featureName.c_str());
    if (!obj) return p;

    auto profileSketchName = [](App::DocumentObject* profileOwner) -> std::string {
        auto* profProp = dynamic_cast<App::PropertyLink*>(
            profileOwner->getPropertyByName("Profile"));
        if (profProp && profProp->getValue())
            return profProp->getValue()->getNameInDocument();
        return {};
    };

    if (auto* pad = dynamic_cast<PartDesign::Pad*>(obj)) {
        p.typeName   = "PartDesign::Pad";
        p.length     = pad->Length.getValue();
        p.length2    = pad->Length2.getValue();
        p.reversed   = pad->Reversed.getValue();
        p.sketchName = profileSketchName(pad);
        p.editable   = true;
        try { p.sideType = pad->SideType.getValueAsString(); } catch (...) { p.sideType = "One side"; }
        try { p.method   = pad->Type.getValueAsString();     } catch (...) { p.method   = "Length"; }
    }
    else if (auto* pocket = dynamic_cast<PartDesign::Pocket*>(obj)) {
        p.typeName   = "PartDesign::Pocket";
        p.length     = pocket->Length.getValue();
        p.length2    = pocket->Length2.getValue();
        p.reversed   = pocket->Reversed.getValue();
        p.sketchName = profileSketchName(pocket);
        p.editable   = true;
        try { p.sideType = pocket->SideType.getValueAsString(); } catch (...) { p.sideType = "One side"; }
        try { p.method   = pocket->Type.getValueAsString();     } catch (...) { p.method   = "Length"; }
    }
    else if (auto* rev = dynamic_cast<PartDesign::Revolution*>(obj)) {
        p.typeName   = "PartDesign::Revolution";
        p.angle      = rev->Angle.getValue();
        p.reversed   = rev->Reversed.getValue();
        p.sketchName = profileSketchName(rev);
        p.editable   = true;
    }
    else if (auto* grv = dynamic_cast<PartDesign::Groove*>(obj)) {
        p.typeName   = "PartDesign::Groove";
        p.angle      = grv->Angle.getValue();
        p.reversed   = grv->Reversed.getValue();
        p.sketchName = profileSketchName(grv);
        p.editable   = true;
    }
    else {
        // OCCT fallback Part::Feature — shape is static, cannot be edited
        p.typeName = obj->getTypeId().getName();
        p.editable = false;
    }
    return p;
}

bool PartFacade::updatePad(const std::string& featureName, double length, bool reversed)
{
    PadOptions opts;
    opts.length = length;
    opts.reversed = reversed;
    // Preserve existing sideType/method on simple edits so toggling Reverse
    // from the task panel doesn't silently reset "Symmetric" to "One side".
    auto current = getFeatureParams(featureName);
    if (current.editable) {
        if (!current.sideType.empty()) opts.sideType = current.sideType;
        if (!current.method.empty())   opts.method   = current.method;
        opts.length2 = current.length2;
    }
    return updatePadEx(featureName, opts);
}

bool PartFacade::updatePocket(const std::string& featureName, double depth, bool reversed)
{
    PadOptions opts;
    opts.length = depth;
    opts.reversed = reversed;
    auto current = getFeatureParams(featureName);
    if (current.editable) {
        if (!current.sideType.empty()) opts.sideType = current.sideType;
        if (!current.method.empty())   opts.method   = current.method;
        opts.length2 = current.length2;
    }
    return updatePocketEx(featureName, opts);
}

bool PartFacade::updatePadEx(const std::string& featureName, const PadOptions& opts)
{
    if (!impl_->doc) return false;
    auto* pad = dynamic_cast<PartDesign::Pad*>(impl_->doc->getObject(featureName.c_str()));
    if (!pad) return false;
    try {
        pad->Length.setValue(opts.length);
        pad->Length2.setValue(opts.length2);
        pad->Reversed.setValue(opts.reversed);
        try { pad->SideType.setValue(opts.sideType.c_str()); } catch (...) {}
        try { pad->Type.setValue(opts.method.c_str());       } catch (...) {}
        impl_->doc->recompute();
        return !pad->Shape.getShape().getShape().IsNull();
    } catch (...) { return false; }
}

bool PartFacade::updatePocketEx(const std::string& featureName, const PadOptions& opts)
{
    if (!impl_->doc) return false;
    auto* pocket = dynamic_cast<PartDesign::Pocket*>(impl_->doc->getObject(featureName.c_str()));
    if (!pocket) return false;
    try {
        pocket->Length.setValue(opts.length);
        pocket->Length2.setValue(opts.length2);
        pocket->Reversed.setValue(opts.reversed);
        try { pocket->SideType.setValue(opts.sideType.c_str()); } catch (...) {}
        try { pocket->Type.setValue(opts.method.c_str());       } catch (...) {}
        impl_->doc->recompute();
        return !pocket->Shape.getShape().getShape().IsNull();
    } catch (...) { return false; }
}

bool PartFacade::updateRevolution(const std::string& featureName, double angleDeg)
{
    if (!impl_->doc) return false;
    auto* rev = dynamic_cast<PartDesign::Revolution*>(impl_->doc->getObject(featureName.c_str()));
    if (!rev) return false;
    try {
        rev->Angle.setValue(angleDeg);
        impl_->doc->recompute();
        return !rev->Shape.getShape().getShape().IsNull();
    } catch (...) { return false; }
}

bool PartFacade::updateGroove(const std::string& featureName, double angleDeg)
{
    if (!impl_->doc) return false;
    auto* grv = dynamic_cast<PartDesign::Groove*>(impl_->doc->getObject(featureName.c_str()));
    if (!grv) return false;
    try {
        grv->Angle.setValue(angleDeg);
        impl_->doc->recompute();
        return !grv->Shape.getShape().getShape().IsNull();
    } catch (...) { return false; }
}

std::string PartFacade::fillet(const std::vector<std::string>& /*edgeRefs*/, double /*radius*/)
{
    Base::Console().warning("CADNC: PartFacade::fillet not yet implemented\n");
    return {};
}

std::string PartFacade::chamfer(const std::vector<std::string>& /*edgeRefs*/, double /*size*/)
{
    Base::Console().warning("CADNC: PartFacade::chamfer not yet implemented\n");
    return {};
}

std::string PartFacade::linearPattern(const std::string& featureName,
                                       double dirX, double dirY, double dirZ,
                                       double length, int occurrences)
{
    if (!impl_->doc) return {};
    auto* feature = impl_->doc->getObject(featureName.c_str());
    if (!feature) return {};

    auto* body = impl_->findOrCreateBody();
    if (!body) return {};

    try {
        auto* obj = new PartDesign::LinearPattern();
        impl_->doc->addObject(obj, "LinearPattern");
        // Set originals list
        std::vector<App::DocumentObject*> originals = {feature};
        obj->Originals.setValues(originals);
        obj->Length.setValue(length);
        obj->Occurrences.setValue(occurrences);
        body->addObject(obj);
        impl_->doc->recompute();
        if (!obj->Shape.getShape().getShape().IsNull())
            return obj->getNameInDocument();
        impl_->doc->removeObject(obj->getNameInDocument());
    } catch (...) {}
    return {};
}

std::string PartFacade::polarPattern(const std::string& featureName,
                                      double axisX, double axisY, double axisZ,
                                      double angleDeg, int occurrences)
{
    if (!impl_->doc) return {};
    auto* feature = impl_->doc->getObject(featureName.c_str());
    if (!feature) return {};

    auto* body = impl_->findOrCreateBody();
    if (!body) return {};

    try {
        auto* obj = new PartDesign::PolarPattern();
        impl_->doc->addObject(obj, "PolarPattern");
        std::vector<App::DocumentObject*> originals = {feature};
        obj->Originals.setValues(originals);
        obj->Angle.setValue(angleDeg);
        obj->Occurrences.setValue(occurrences);
        body->addObject(obj);
        impl_->doc->recompute();
        if (!obj->Shape.getShape().getShape().IsNull())
            return obj->getNameInDocument();
        impl_->doc->removeObject(obj->getNameInDocument());
    } catch (...) {}
    return {};
}

std::string PartFacade::mirror(const std::string& featureName,
                                double planeNormX, double planeNormY, double planeNormZ)
{
    if (!impl_->doc) return {};
    auto* feature = impl_->doc->getObject(featureName.c_str());
    if (!feature) return {};

    auto* body = impl_->findOrCreateBody();
    if (!body) return {};

    try {
        auto* obj = new PartDesign::Mirrored();
        impl_->doc->addObject(obj, "Mirrored");
        std::vector<App::DocumentObject*> originals = {feature};
        obj->Originals.setValues(originals);
        body->addObject(obj);
        impl_->doc->recompute();
        if (!obj->Shape.getShape().getShape().IsNull())
            return obj->getNameInDocument();
        impl_->doc->removeObject(obj->getNameInDocument());
    } catch (...) {}
    return {};
}

std::string PartFacade::groove(const std::string& sketchName, double angleDeg)
{
    if (!impl_->doc) return {};
    auto* sketch = impl_->doc->getObject(sketchName.c_str());
    if (!sketch) return {};

    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* obj = new PartDesign::Groove();
            impl_->doc->addObject(obj, "Groove");
            obj->Profile.setValue(sketch);
            obj->Type.setValue("Angle");
            obj->Angle.setValue(angleDeg);
            obj->Axis.setValue(Base::Vector3d(0, 1, 0));
            obj->Base.setValue(Base::Vector3d(0, 0, 0));
            body->addObject(obj);
            impl_->doc->recompute();
            if (!obj->Shape.getShape().getShape().IsNull())
                return obj->getNameInDocument();
            impl_->doc->removeObject(obj->getNameInDocument());
        } catch (...) {}
    }

    // Fallback: build face from sketch wires → revolve about Y axis (negative angle).
    gp_Ax1 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
    double angleRad = -angleDeg * 3.14159265358979323846 / 180.0;
    auto* feat = makeRevolutionFeature(impl_->doc, sketch, axis, angleRad, "Groove");
    if (!feat) return {};
    try { impl_->doc->recompute(); } catch (...) {}
    return feat->getNameInDocument();
}

std::string PartFacade::booleanFuse(const std::string& baseName, const std::string& toolName)
{
    if (!impl_->doc) return {};
    auto* base = impl_->doc->getObject(baseName.c_str());
    auto* tool = impl_->doc->getObject(toolName.c_str());
    if (!base || !tool) return {};

    auto* obj = impl_->doc->addObject("Part::Fuse", "Fuse");
    if (!obj) return {};
    auto* fuse = dynamic_cast<Part::Fuse*>(obj);
    if (!fuse) return {};
    fuse->Base.setValue(base);
    fuse->Tool.setValue(tool);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::booleanCut(const std::string& baseName, const std::string& toolName)
{
    if (!impl_->doc) return {};
    auto* base = impl_->doc->getObject(baseName.c_str());
    auto* tool = impl_->doc->getObject(toolName.c_str());
    if (!base || !tool) return {};

    auto* obj = impl_->doc->addObject("Part::Cut", "Cut");
    if (!obj) return {};
    auto* cut = dynamic_cast<Part::Cut*>(obj);
    if (!cut) return {};
    cut->Base.setValue(base);
    cut->Tool.setValue(tool);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::booleanCommon(const std::string& baseName, const std::string& toolName)
{
    if (!impl_->doc) return {};
    auto* base = impl_->doc->getObject(baseName.c_str());
    auto* tool = impl_->doc->getObject(toolName.c_str());
    if (!base || !tool) return {};

    auto* obj = impl_->doc->addObject("Part::Common", "Common");
    if (!obj) return {};
    auto* common = dynamic_cast<Part::Common*>(obj);
    if (!common) return {};
    common->Base.setValue(base);
    common->Tool.setValue(tool);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::addBox(double length, double width, double height)
{
    if (!impl_->doc) return {};
    auto* obj = impl_->doc->addObject("Part::Box", "Box");
    if (!obj) return {};
    auto* box = dynamic_cast<Part::Box*>(obj);
    if (!box) return {};
    box->Length.setValue(length);
    box->Width.setValue(width);
    box->Height.setValue(height);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::addCylinder(double radius, double height, double angle)
{
    if (!impl_->doc) return {};
    auto* obj = impl_->doc->addObject("Part::Cylinder", "Cylinder");
    if (!obj) return {};
    auto* cyl = dynamic_cast<Part::Cylinder*>(obj);
    if (!cyl) return {};
    cyl->Radius.setValue(radius);
    cyl->Height.setValue(height);
    cyl->Angle.setValue(angle);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::addSphere(double radius)
{
    if (!impl_->doc) return {};
    auto* obj = impl_->doc->addObject("Part::Sphere", "Sphere");
    if (!obj) return {};
    auto* sphere = dynamic_cast<Part::Sphere*>(obj);
    if (!sphere) return {};
    sphere->Radius.setValue(radius);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::addCone(double radius1, double radius2, double height)
{
    if (!impl_->doc) return {};
    auto* obj = impl_->doc->addObject("Part::Cone", "Cone");
    if (!obj) return {};
    auto* cone = dynamic_cast<Part::Cone*>(obj);
    if (!cone) return {};
    cone->Radius1.setValue(radius1);
    cone->Radius2.setValue(radius2);
    cone->Height.setValue(height);

    try { impl_->doc->recompute(); }
    catch (...) { impl_->doc->removeObject(obj->getNameInDocument()); return {}; }
    return obj->getNameInDocument();
}

std::string PartFacade::filletAll(const std::string& featureName, double radius)
{
    if (!impl_->doc) return {};
    auto* feature = impl_->doc->getObject(featureName.c_str());
    if (!feature) return {};

    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* obj = new PartDesign::Fillet();
            impl_->doc->addObject(obj, "Fillet");
            obj->Base.setValue(feature);
            obj->Radius.setValue(radius);
            obj->UseAllEdges.setValue(true);
            body->addObject(obj);
            impl_->doc->recompute();
            if (!obj->Shape.getShape().getShape().IsNull())
                return obj->getNameInDocument();
            impl_->doc->removeObject(obj->getNameInDocument());
        } catch (...) {}
    }
    return {};
}

std::string PartFacade::chamferAll(const std::string& featureName, double size)
{
    if (!impl_->doc) return {};
    auto* feature = impl_->doc->getObject(featureName.c_str());
    if (!feature) return {};

    auto* body = impl_->findOrCreateBody();
    if (body) {
        try {
            auto* obj = new PartDesign::Chamfer();
            impl_->doc->addObject(obj, "Chamfer");
            obj->Base.setValue(feature);
            obj->Size.setValue(size);
            obj->UseAllEdges.setValue(true);
            body->addObject(obj);
            impl_->doc->recompute();
            if (!obj->Shape.getShape().getShape().IsNull())
                return obj->getNameInDocument();
            impl_->doc->removeObject(obj->getNameInDocument());
        } catch (...) {}
    }
    return {};
}

} // namespace CADNC
