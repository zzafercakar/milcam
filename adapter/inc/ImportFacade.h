// SPDX-License-Identifier: Proprietary
// ImportFacade — read-only geometry intake for MilCAM.
//
// MilCAM is CAM-only: we never author parametric geometry. We only ingest
// foreign formats (STEP, IGES, BREP, DXF, STL, OBJ) and hand them to the
// CAM module. ImportFacade wraps FreeCAD's Part importers and presents a
// minimal CAM-friendly view of the loaded shape (face list, plane analysis,
// bounding box, units).
//
// Lives next to PartFacade so the migration to a slim "input-only" adapter
// can proceed incrementally — see .ai/WORKPLAN.md "Faz 1 — Slim Adapter".

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <vector>

namespace MilCAM {

struct ImportResult {
    bool                ok = false;
    QString             error;
    QString             sourcePath;
    QString             format;        // "STEP", "IGES", "BREP", "DXF", "STL", "OBJ"
    int                 solidCount = 0;
    int                 faceCount = 0;
    int                 edgeCount = 0;
    double              bboxMin[3]{0,0,0};
    double              bboxMax[3]{0,0,0};
    QString             units;         // "mm", "in" — best-effort from header
};

class ImportFacade : public QObject {
    Q_OBJECT
public:
    explicit ImportFacade(QObject* parent = nullptr);
    ~ImportFacade() override;

    Q_INVOKABLE QStringList supportedExtensions() const;
    Q_INVOKABLE bool        canImport(const QString& path) const;

    Q_INVOKABLE ImportResult importFile(const QString& path);

    // After a successful import, the CAM module reads the resulting shape via
    // CamGeometrySource — wired in CamSession. ImportFacade itself is stateless
    // wrt the active job.

Q_SIGNALS:
    void importStarted(const QString& path);
    void importFinished(const ImportResult& result);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace MilCAM
