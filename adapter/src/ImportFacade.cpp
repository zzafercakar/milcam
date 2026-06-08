// SPDX-License-Identifier: Proprietary
// ImportFacade implementation — STUB.
//
// Wiring plan (see .ai/WORKPLAN.md "Faz 2 — Geometry intake"):
//   - .step / .stp / .iges / .igs → Part::ImportStepParts / ImportIges
//   - .brep                       → BRepTools::Read
//   - .stl                        → Part::Mesh ingest via OCCT StlAPI_Reader
//   - .dxf                        → libArea / FreeCAD's importDXF (read-only)
//   - .obj                        → OCCT-aided OBJ reader
//
// Until the freecad/ Part importers are wired in MilCAM's CMakeLists, this
// implementation returns "unsupported format" and the QML can still develop
// against the public surface.

#include "ImportFacade.h"

#include <QFileInfo>

namespace MilCAM {

struct ImportFacade::Impl {};

ImportFacade::ImportFacade(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>()) {}

ImportFacade::~ImportFacade() = default;

QStringList ImportFacade::supportedExtensions() const {
    return { "step", "stp", "iges", "igs", "brep", "stl", "obj", "dxf" };
}

bool ImportFacade::canImport(const QString& path) const {
    const QString ext = QFileInfo(path).suffix().toLower();
    return supportedExtensions().contains(ext);
}

ImportResult ImportFacade::importFile(const QString& path) {
    Q_EMIT importStarted(path);
    ImportResult r;
    r.sourcePath = path;
    r.format     = QFileInfo(path).suffix().toUpper();
    r.ok         = false;
    r.error      = QStringLiteral("Importer not yet wired. See .ai/WORKPLAN.md Faz 2.");
    Q_EMIT importFinished(r);
    return r;
}

} // namespace MilCAM
