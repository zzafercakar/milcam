# MilCAD — Product Requirements Document (PRD)
## CODESYS-Targeted CAD/CAM Software with G-Code Generation

**Version:** 2.0
**Date:** 2026-03-16
**Author:** Zafer (Product Owner) & Claude AI (Technical Architect)
**Tech Stack:** C++17 / Qt 6.4 / OpenCascade 7.6 / CMake
**Target:** Desktop Application (Windows/Linux)
**Referanslar:** SolidWorks 2026, Fusion 360, FreeCAD (PlaneGCS), OnShape, Eastman CutPro/patternPRO, SigmaNEST, Deepnest

---

## 1. PRODUCT VISION & EXECUTIVE SUMMARY

### 1.1 Problem Statement
Endüstriyel otomasyon sektöründe CODESYS SoftMotion CNC kontrolcülere G-code üretebilen, entegre CAD/CAM çözümü bulunmamaktadır. Mevcut çözümler (Fusion 360, MasterCAM, SolidWorks CAM) genel amaçlı G-code üretir ve CODESYS DIN 66025 dialect'ine, SMC_CNC_REF/SMC_OUTQUEUE yapılarına direkt uyumlu değildir.

### 1.2 Solution
**MilCAD** — OpenCascade kernel üzerinde Qt 6 arayüzü ile çalışan, 2D sketch'ten 3D part modellemeye, toolpath generation'dan CODESYS-uyumlu G-code üretimine kadar tam entegre bir CAD/CAM yazılımı.

### 1.3 Unique Value Proposition
- **CODESYS-Native G-Code:** DIN 66025 uyumlu, SMC_NCInterpreter ile direkt parse edilebilen G-code çıkışı
- **SoftMotion Integration:** SMC_CNC_REF ve SMC_OUTQUEUE compile modlarına uygun çıktı
- **Endüstriyel Odak:** EtherCAT, CANopen, Modbus TCP sürücülerle çalışan CNC sistemleri hedef
- **Açık Kaynak Kernel:** OpenCascade tabanlı, lisans maliyeti yok

### 1.4 Target Users
| Kullanıcı Profili | İhtiyaç |
|---|---|
| CNC Makine Operatörü | Basit 2D/2.5D parça çizimi → G-code |
| Otomasyon Mühendisi | CODESYS PLC'ye entegre CNC programı |
| Makine İmalatçısı | Kendi CNC kontrol sistemi için CAD/CAM |
| Eğitim Kurumları | CNC programlama eğitimi |

### 1.5 Referans Yazılım Karşılaştırması

#### Fonksiyonel Karşılaştırma

| Özellik | SolidWorks | Fusion 360 | FreeCAD | OnShape | CutPro/patternPRO | **MilCAD (Hedef)** |
|---|---|---|---|---|---|---|
| 2D Sketching | ✅ Full (~28 entity) | ✅ Full (~12) | ✅ Full (~12) | ✅ Full (cloud) | ✅ Temel CAD | ✅ **Done** (8 entity) |
| Constraint Solver | Variational | Variational | PlaneGCS | Variational (cloud) | — | Gauss-Newton |
| Auto-Constrain | ✅ | ✅ | ✅ | ✅ | — | ⬜ Phase 1+ |
| 3D Parametric Modeling | ✅ Full | ✅ Full (Timeline) | ✅ Full (Body+Tip) | ✅ Full (cloud) | ❌ | 🟨 **Pad/Pocket/Revolution ✅**, Fillet/Chamfer/Shell ⬜ |
| Feature Types | ~50 | ~75 | ~30 | ~40 | — | 🟨 3/6 core (Pad/Pocket/Revolution ✅) |
| Sheet Metal / Flat Pattern | ✅ | ✅ | ✅ | ✅ (auto flat) | ✅ patternPRO | ⬜ Phase 2+ |
| Assembly | ✅ | ✅ | ✅ | ✅ (real-time collab) | ❌ | ❌ v2.0+ |
| 2D/2.5D CAM | ✅ CAMWorks | ✅ (Adaptive) | ✅ (libarea) | ✅ (Fusion CAM) | ✅ Cutting paths | ✅ **Done** (Facing/Profile/Pocket/Drill) |
| 3D CAM | ✅ | ✅ (HSM) | ✅ | ✅ | ❌ | ⬜ Phase 4+ |
| **2D Nesting** | ❌ (3rd party) | ✅ (basic) | ❌ (3rd party) | ❌ | ✅ **Core** (Nest++) | 🟨 **BBox+BLF ✅**, NFP ⬜ |
| True Shape Nesting | ❌ | ❌ | ❌ | ❌ | ✅ Nest++PRO (93.5%) | ⬜ Phase 7.2 |
| Cut Path Optimization | ❌ | ❌ | ❌ | ❌ | ✅ (InMotion) | ⬜ Phase 7.3 |
| Adaptive Clearing | ❌ | ✅ Signature | ❌ | ❌ | ❌ | ⬜ Phase 4+ |
| Post-Processor | Generic | JS-based (.cps) | Python-based | — | CutPro machine | ✅ **CODESYS Native Done** |
| Simulation | ✅ | ✅ (Cloud FEA) | ✅ | ✅ (FeatureScript) | ❌ | 🟨 **CamSimulator ✅**, UI ⬜ |
| STEP/IGES | ✅ | ✅ | ✅ | ✅ | DXF/AAMA/ASTM | ✅ **Done** (FileManager) |
| DXF Import/Export | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ **Done** |
| Fiyat | $$$$ | $$$ | Free | $$ (subscription) | $$$$ (machine-bundled) | Free/Commercial |

#### Teknik Mimari Karşılaştırması

| Bileşen | SolidWorks | Fusion 360 | FreeCAD | OnShape | CutPro | **MilCAD** |
|---|---|---|---|---|---|---|
| Geometry Kernel | Parasolid | Autodesk ShapeManager | OCCT | Parasolid (cloud) | Proprietary 2D | OCCT 7.6 |
| UI Framework | MFC/WPF | Electron + C++ | Qt5 Widgets + Python | WebGL + React | Qt/Windows | Qt6 Quick/QML |
| Sketch Solver | Proprietary | Proprietary | PlaneGCS (Eigen) | Proprietary (cloud) | — | Gauss-Newton (custom) |
| CAM Engine | CAMWorks | HSMWorks (Autodesk) | libarea + Python | — | CutPro engine | Custom C++ |
| Nesting Engine | — | Basic | — | — | Nest++ / Nest++PRO | Custom NFP-based |
| Post-Processor | Template | JavaScript (.cps) | Python scripts | — | Machine-specific | C++ class hierarchy |
| File Format | .sldprt (binary) | .f3d (cloud) | .FCStd (ZIP+XML) | Cloud JSON | DXF/AAMA | .milcad (ZIP+JSON+BREP) |
| Undo System | Proprietary | Timeline rollback | Transaction-based | Cloud versioning | — | Command pattern |

---

## 2. SYSTEM ARCHITECTURE

### 2.1 High-Level Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                           MilCAD Application                          │
├──────────┬──────────┬───────────┬──────────┬───────────┬─────────────┤
│  Sketch  │   Part   │    CAM    │ Nesting  │  G-Code   │ Simulation  │
│ Workbench│Workbench │ Workbench │Workbench │ Generator │   Engine    │
├──────────┴──────────┴───────────┴───────────┴───────────────┤
│                    Core Services Layer                        │
│  ┌──────────┬───────────┬──────────┬────────────────────┐   │
│  │ Document │ Undo/Redo │ Selection│ Constraint Solver   │   │
│  │ Manager  │  (Command)│  Engine  │ (Gauss-Newton)      │   │
│  └──────────┴───────────┴──────────┴────────────────────┘   │
├──────────────────────────────────────────────────────────────┤
│                    Graphics / Visualization                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Qt 6 QML + OCCT V3d_Viewer + AIS_InteractiveContext │   │
│  │  ViewerItem (QQuickFBO) → OccRenderer (FBO+GLX)      │   │
│  └──────────────────────────────────────────────────────┘   │
├──────────────────────────────────────────────────────────────┤
│                    Geometry Kernel (OpenCascade 7.6)          │
│  ┌────────┬──────────┬──────────┬───────────┬───────────┐   │
│  │ B-Rep  │ Boolean  │  Fillet  │ Data Exch │   Mesh    │   │
│  │Modeling│   Ops    │ /Chamfer │ STEP/IGES │Generation │   │
│  └────────┴──────────┴──────────┴───────────┴───────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Module Dependency Map

```
G-Code Generator ──→ CAM Engine ──→ Part Modeling ──→ Sketch Engine
       │                 │                │                │
       ▼                 ▼                ▼                ▼
  CODESYS Post     Toolpath Algo    OCCT B-Rep      Gauss-Newton
  Processor        (offset/pocket)  TopoDS_Shape    Constraint
                                                     Solver

Nesting Engine ──→ 2D Profiles ──→ Sketch Engine
       │                │
       ▼                ▼
  NFP Algorithm    DXF/SVG I/O
  Cut Path Opt     Material Mgmt
```

### 2.3 Mevcut Modül Yapısı

```
MilCAD/
├── CMakeLists.txt
├── app/                → main.cpp, Qt Quick application entry
├── core/               → OccRenderer, SceneManager, WorkbenchManager, Command, GlTools
│   ├── inc/            → Header files
│   └── src/            → Implementation
├── geometry/           → SketchEntity (7 tip), SketchConstraint (13 tip), SketchSolver,
│   │                     SketchDocument, DrawingTool, DrawingStrategy, ShapeFactory,
│   │                     SketchFilletTool, SketchTrimTool, SketchOffsetTool
│   ├── inc/
│   └── src/
├── viewport/           → ViewerItem (QML bridge), SnapManager (8 snap tipi),
│   │                     GridManager, SnapMarkerPresenter, InferenceLinePresenter
│   ├── inc/
│   └── src/
├── input/              → MouseHandler (AIS_ViewController), KeyboardHandler
│   ├── inc/
│   └── src/
├── ui/                 → PreviewOverlay
│   ├── inc/
│   └── src/
├── util/               → CoordinateMapper, Helpers
│   ├── inc/
│   └── src/
├── cam/                → CamJob, Operations (4 tip), Toolpath, GCode, PostProcessors,
│   ├── inc/              CamSimulator, DexelStockSimulator, ToolpathRenderer (19 header)
│   └── src/              17 source dosyası
├── nesting/            → NestJob, BoundingBoxNester, BottomLeftFillNester, NestingEngine
│   ├── inc/              5 header
│   └── src/              4 source dosyası
├── qml/                → Main.qml, SketchToolbar.qml, ConstraintPanel.qml,
│                         PartToolbar.qml, CAMToolbar.qml, ModelTreePanel.qml,
│                         DimensionInput.qml, buttons/, popup/
├── tests/              → 49 Google Test dosyası (9 modül dizini)
├── resources/          → icons, themes, translations
├── doc/                → ARCHITECTURE.md, TECH_STACK.md
└── .ai/                → PRD.md, ROADMAP.md, WORKPLAN.md, context.yaml
```

---

## 3. SKETCH WORKBENCH — Detaylı Gereksinimler

### 3.1 Genel Bakış
Sketch Workbench, seçili bir düzlem (XY, XZ, YZ veya herhangi bir planar yüzey) üzerinde 2D geometrik çizim yapma ortamıdır. Tüm CAD/CAM yazılımlarının temel taşıdır.

**Mevcut Durum:** Temel altyapı tamamlandı. WorkbenchManager, SketchDocument, 7 entity tipi, 13 constraint, Gauss-Newton solver, DOF analizi, 8 drawing strategy, 8 snap tipi ve inference line sistemi çalışıyor.

### 3.2 Sketch Entities (Çizim Öğeleri)

| Entity | OCCT Karşılığı | MilCAD Sınıfı | Durum |
|---|---|---|---|
| Point | Geom_CartesianPoint | (SketchEntity base) | ✅ Kısmen (snap point olarak) |
| Line | BRepBuilderAPI_MakeEdge + gp_Lin | SketchLine | ✅ Done |
| Arc | BRepBuilderAPI_MakeEdge + GC_MakeArcOfCircle | SketchArc | ✅ Done |
| Circle | BRepBuilderAPI_MakeEdge + gp_Circ | SketchCircle | ✅ Done |
| Rectangle | 4× BRepBuilderAPI_MakeEdge → Wire | SketchRectangle | ✅ Done |
| Ellipse | BRepBuilderAPI_MakeEdge + gp_Elips | SketchEllipse | ✅ Done |
| Spline | BRepBuilderAPI_MakeEdge + Geom_BSplineCurve | SketchSpline | ✅ Done |
| Polygon | N× BRepBuilderAPI_MakeEdge → Wire | SketchPolygon | ✅ Done |
| Construction Line | (Aynılar, `isConstruction_` flag) | SketchEntity::isConstruction_ | ✅ Done |
| Tangent Arc | GC_MakeArcOfCircle + tangent start | — | ⬜ P1 |
| Slot (Straight) | 2× Line + 2× Arc → Wire | — | ⬜ P2 |
| Partial Ellipse | Geom_Ellipse (start/end angle) | — | ⬜ P2 |
| Text/Engrave | Font → wire conversion | — | ⬜ P2 |

**Referans Entity Sayıları:** SolidWorks ~28 entity, Fusion 360 ~12 entity, FreeCAD ~12 entity. MilCAD'in 7+construction entity'si temel kullanımı karşılar. Tangent Arc (P1) ve Slot (P2) sonraki fazlarda eklenecek.

**SketchEntity Base Class:**
```cpp
class SketchEntity {
    int id_;
    bool isConstruction_;
    std::vector<gp_Pnt> points_;       // Control points
    TopoDS_Shape cachedShape_;

    virtual void build() = 0;          // Points → OCCT shape
    virtual TopoDS_Edge toEdge() = 0;
    virtual TopoDS_Wire toWire() = 0;
    virtual std::unique_ptr<SketchEntity> clone() = 0;
};
```

### 3.3 Geometric Constraints (Geometrik Kısıtlamalar)

#### 3.3.1 Desteklenen Constraint Tipleri

| Constraint | Denklem | DOF Azaltma | MilCAD Sınıfı | Durum |
|---|---|---|---|---|
| Coincident | ‖P₁ − P₂‖ = 0 | 2 | SketchConstraintCoincident | ✅ Done |
| Horizontal | P₁.y − P₂.y = 0 | 1 | SketchConstraintHorizontal | ✅ Done |
| Vertical | P₁.x − P₂.x = 0 | 1 | SketchConstraintVertical | ✅ Done |
| Parallel | cross(d₁, d₂) = 0 | 1 | SketchConstraintParallel | ✅ Done |
| Perpendicular | dot(d₁, d₂) = 0 | 1 | SketchConstraintPerpendicular | ✅ Done |
| Tangent | Contact + direction match | 1 | SketchConstraintTangent | ✅ Done |
| Equal | L₁ − L₂ = 0 veya R₁ − R₂ = 0 | 1 | SketchConstraintEqual | ✅ Done |
| Fix/Lock | P = P_initial | 2 | SketchConstraintFixed | ✅ Done |
| Symmetric | Mirror equation | 2 | SketchConstraintSymmetric | ✅ Done |
| Midpoint | t = 0.5 (parametrik) | 2 | SketchConstraintMidpoint | ✅ Done |
| Distance | ‖P₁ − P₂‖ = d | 1 | SketchConstraintDistance | ✅ Done |
| Angle | atan2(cross, dot) = θ | 1 | SketchConstraintAngle | ✅ Done |
| Radius | r = value | 1 | SketchConstraintRadius | ✅ Done |
| Collinear | Parallel + point-on-line | 2 | SketchConstraintCollinear | ✅ Done |
| Concentric | Center₁ = Center₂ | 2 | SketchConstraintConcentric | ✅ Done |
| Coradial | Center + Radius eşit | 3 | — | ⬜ P2 |
| Smooth (G2) | Curvature continuity | 1 | — | ⬜ P2 |

**Referans Constraint Sayıları:**
- **SolidWorks:** 19 relation + 8 dimension tipi (Ordinate, Baseline, Chain, Driven/Reference dahil)
- **Fusion 360:** 22 high-level constraint (Smooth/G2, CircularPattern, RectangularPattern dahil)
- **FreeCAD:** 37 low-level + 22 high-level constraint (PlaneGCS)
- **MilCAD:** 13 constraint — pratik kullanımın ~90%'ını karşılar

**Eksik Ama Önemli (Gelecek):**
- **Driven/Reference Dimension** — salt okunur ölçü, constraint yaratmaz (SolidWorks parantezli gösterim)
- **Fully Define Sketch** — kalan DOF'ları otomatik dimension/relation ile sıfırlama (SolidWorks)
- **Ordinate Dimensions** — origin'den koordinat gösterimi (SolidWorks)

#### 3.3.2 Constraint Solver Mimarisi

**Mevcut İmplementasyon: Gauss-Newton**

```cpp
class SketchSolver {
    // Parametre vektörü: tüm entity control point'lerinin flat listesi
    std::vector<double> params_;
    std::vector<ParamRange> ranges_;     // Entity → param slice mapping

    // Gauss-Newton iteration
    bool solve(std::vector<SketchConstraint*>& constraints,
               int maxIter = 100, double tolerance = 1e-10);

    // Finite-difference Jacobian
    Eigen::MatrixXd computeJacobian(constraints, params, epsilon = 1e-7);
};
```

**Solver Workflow:**
1. Entity control point'leri → flat parametre vektörüne serialize
2. Her constraint → residual vektörü (F) hesapla
3. Jacobian (J) matrisi finite-difference ile hesapla
4. Normal equations: JᵀJ · Δp = −Jᵀ · F
5. Parametre güncelleme: p ← p + Δp
6. ‖F‖ < tolerance olana kadar iterasyon
7. Sonuç parametreleri → entity'lere geri yaz

**FreeCAD PlaneGCS Referansı (Gelecek İyileştirmeler):**

| Özellik | MilCAD (Mevcut) | FreeCAD PlaneGCS | Hedef |
|---|---|---|---|
| Algoritma | Gauss-Newton | DogLeg + BFGS + LevenbergMarquardt | DogLeg eklenmeli |
| Jacobian | Finite-difference | Analitik (DeriVector2 auto-diff) | Analitik geçiş |
| Subsystem Partition | Yok | Bağımsız alt-sistemlere bölme | Phase 1+ |
| DOF Analizi | ✅ SketchDofAnalyzer | Eigen QR decomposition | ✅ Mevcut yeterli |
| Redundancy Detection | Kısmi | Tam (conflicting/redundant ayrımı) | Phase 1+ |

**DOF Göstergesi ve Renk Kodlaması:**
- Under-constrained (mavi): Entity'ler sürüklenebilir
- Fully-constrained (beyaz/siyah): Tüm DOF = 0
- Over-constrained (kırmızı): Çelişen constraint'ler

### 3.4 Sketch Tools (Çizim Araçları)

#### 3.4.1 Drawing Strategies (Strategy Pattern)

| Araç | Kısayol | DrawingStrategy Sınıfı | Tıklama | Durum |
|---|---|---|---|---|
| Line | L | LineStrategy | 2 (start, end) | ✅ Done |
| Polyline | — | PolylineStrategy | N + Enter | ✅ Done |
| Rectangle | R | RectangleStrategy | 2 (köşe, köşe) | ✅ Done |
| Circle | C | CircleStrategy | 2 (merkez, radius) | ✅ Done |
| Arc | A | ArcThreePointStrategy | 3 (start, mid, end) | ✅ Done |
| Ellipse | E | EllipseStrategy | 3 (merkez, major, minor) | ✅ Done |
| Spline | S | SplineStrategy | N + Enter | ✅ Done |
| Polygon | P | PolygonStrategy | 2 (merkez, vertex) | ✅ Done |

#### 3.4.2 Editing Tools

| Araç | Kısayol | MilCAD Sınıfı | Durum |
|---|---|---|---|
| Trim | T | SketchTrimTool | ✅ Done |
| Offset | O | SketchOffsetTool | ✅ Done |
| 2D Fillet | F | SketchFilletTool | ✅ Done |
| 2D Chamfer | Shift+F | (SketchFilletTool variant) | ✅ Done |
| Extend | X | — | ⬜ TODO |
| Mirror | M | — | ⬜ TODO |
| Construction Toggle | G | SketchEntity::isConstruction_ | ✅ Done |
| Dimension | D | DimensionInput.qml | ✅ Done |

#### 3.4.3 Auto-Constraining (Gelecek — Ref: FreeCAD/Fusion 360)

FreeCAD'de `DrawSketchHandler::seekAutoConstraint()` mekanizması:
1. Mouse hareketi sırasında yakın vertex/curve tarar
2. `std::vector<AutoConstraint>` ile öneriler oluşturur (Coincident, PointOnObject, Tangent)
3. Commit öncesi `diagnoseAdditionalConstraints()` ile redundancy kontrolü
4. Redundant ise otomatik kaldırır

**MilCAD Hedef:** Phase 1+ — Çizim sırasında otomatik Horizontal/Vertical/Coincident constraint tanıma.

### 3.5 Snap Sistemi

**Mevcut İmplementasyon: SnapManager (8 snap tipi)**

| Snap Tipi | Flag (Bitwise) | Davranış | Durum |
|---|---|---|---|
| Endpoint | SNAP_ENDPOINT | Çizgi uç noktaları | ✅ Done |
| Midpoint | SNAP_MIDPOINT | Çizgi orta noktası | ✅ Done |
| Center | SNAP_CENTER | Daire/yay merkezi | ✅ Done |
| Intersection | SNAP_INTERSECTION | Kesişim noktaları | ✅ Done |
| Tangent | SNAP_TANGENT | Teğet noktaları | ✅ Done |
| Perpendicular | SNAP_PERPENDICULAR | Dik noktalar | ✅ Done |
| Nearest | SNAP_NEAREST | En yakın nokta | ✅ Done |
| Grid | SNAP_GRID | Grid noktaları | ✅ Done |

**Resolution Chain:** Geometry snap → Grid snap → Cardinal alignment (ortho)

**Snap Radius:** 30 piksel (konfigüre edilebilir)

**Thread-Safety:** `findSnap()` önceden toplanmış `TopoDS_Shape` vektörü alır (AIS context kilidini önler)

**Inference Lines (SolidWorks-style):**
- Yatay + dikey referans çizgileri (anchor point üzerinden)
- Önceki segment'in collinear uzantısı
- OCCT viewport'unda kesikli çizgi olarak render

**Eksik:**
- Snap gösterge ikonları (farklı snap tipleri için görsel feedback) — ⬜ TODO

### 3.6 Sketch Davranış Kuralları
1. ✅ Sketch açıldığında kamera seçili düzleme ortogonal döner
2. ✅ Grid seçili düzlem üzerinde görünür
3. ✅ Origin noktası (0,0) referans ile gösterilir
4. ✅ Constraint'ler ConstraintPanel'de listelenir
5. ⬜ "Close Sketch" yapıldığında wire validation (kapalı profil kontrolü)
6. ✅ Over-constrained durumda renk kodlaması

---

## 4. PART WORKBENCH — Detaylı Gereksinimler

### 4.1 Genel Bakış
Part Workbench, 2D sketch'lerden 3D katı modeller oluşturma ortamıdır. Feature-based parametric modeling yaklaşımı kullanılır.

**Referans Mimari (FreeCAD PartDesign):**
```
Part::Feature (OCCT Shape holder)
  → PartDesign::Feature (BaseFeature link, solid enforcement)
    → PartDesign::FeatureAddSub (Additive/Subtractive enum, BooleanOp)
      → PartDesign::ProfileBased (Profile → Sketch link, direction)
        → Pad (Additive extrusion)
        → Pocket (Subtractive extrusion)
        → Revolution (Axis + Angle)
        → FeatureLoft, FeaturePipe (Sweep), FeatureHelix
      → PartDesign::FeatureDressUp
        → Fillet, Chamfer, Draft, Thickness (Shell)
    → PartDesign::FeatureTransformed
      → LinearPattern, PolarPattern, Mirrored, MultiTransform
    → PartDesign::FeatureBoolean
```

**MilCAD Hedef Hiyerarşi:**
```cpp
class Feature {
    QString name_;
    std::map<QString, QVariant> parameters_;
    virtual TopoDS_Shape execute(const TopoDS_Shape& base) = 0;
    virtual bool canExecute() const = 0;
};

class ProfileBasedFeature : public Feature {
    SketchDocument* profile_;
    gp_Dir direction_;
    TopoDS_Wire buildWire();
    TopoDS_Face buildFace();
};

class PadFeature : public ProfileBasedFeature { ... };
class PocketFeature : public ProfileBasedFeature { ... };
class RevolutionFeature : public ProfileBasedFeature { ... };
```

### 4.2 Feature Operasyonları

#### 4.2.1 Temel Extrude/Cut Operasyonları

| Feature | OCCT API | Extrusion Modes (Ref: FreeCAD) | Durum |
|---|---|---|---|
| **Pad (Extrude)** | BRepPrimAPI_MakePrism | Length, UpToLast, UpToFirst, UpToFace, Symmetric, TwoLengths | ⬜ Phase 2 |
| **Pocket (Cut)** | BRepAlgoAPI_Cut + MakePrism | Length, ThroughAll, UpToFirst, UpToFace | ⬜ Phase 2 |
| **Revolution** | BRepPrimAPI_MakeRevol / BRepFeat_MakeRevol | Axis + Angle (0-360°) | ⬜ Phase 2 |
| **Loft** | BRepOffsetAPI_ThruSections | 2+ Profiles, Guide Curves | ⬜ P1 |
| **Sweep** | BRepOffsetAPI_MakePipe | Profile + Path + Twist | ⬜ P1 |
| **Hole** | BRepPrimAPI_MakeCylinder + Cut | Dia, Depth, Thread, Counterbore | ⬜ P1 |

**Pad Modes (SolidWorks/FreeCAD ortak):**
```cpp
enum class ExtrudeType {
    Length,          // Belirli uzunluk (Blind)
    ThroughAll,      // Tamamen geçir (Through All)
    UpToNext,        // Sonraki yüzeye kadar (Up to Next) — SolidWorks
    UpToLast,        // Son yüzeye kadar (Up to Last)
    UpToFirst,       // İlk yüzeye kadar (Up to First)
    UpToFace,        // Seçili yüzeye kadar (Up to Surface)
    UpToVertex,      // Seçili noktaya kadar (Up to Vertex) — SolidWorks
    Symmetric,       // Simetrik (Mid Plane — her iki yöne eşit)
    TwoLengths       // İki yöne farklı uzunluk
};

// Draft desteği (SolidWorks: Boss/Base + Draft On/Off)
struct ExtrudeParams {
    ExtrudeType type;
    double length1;
    double length2;              // TwoLengths modu
    bool reversed;
    bool draftEnabled;
    double draftAngle;           // degrees — taper
    bool thinFeature;
    double thinThickness;
};
```

#### 4.2.2 Modifikasyon Operasyonları

| Feature | OCCT API | Parametreler | Durum |
|---|---|---|---|
| **Fillet** | BRepFilletAPI_MakeFillet | Edge(s), Radius (constant/variable) | ⬜ Phase 2 |
| **Chamfer** | BRepFilletAPI_MakeChamfer | Edge(s), Distance1×Distance2 veya Distance×Angle | ⬜ Phase 2 |
| **Shell** | BRepOffsetAPI_MakeThickSolid | Face(s) to remove, Thickness | ⬜ P1 |
| **Draft** | BRepOffsetAPI_DraftAngle | Face(s), Neutral Plane, Angle | ⬜ P1 |
| **Linear Pattern** | BRepBuilderAPI_Transform + Fuse | Direction, Count, Spacing | ⬜ P1 |
| **Circular Pattern** | BRepBuilderAPI_Transform + Fuse | Axis, Count, Angle | ⬜ P1 |
| **Mirror** | BRepBuilderAPI_Transform | Mirror plane | ⬜ P1 |

#### 4.2.3 Boolean Operasyonları (Mevcut)

| İşlem | OCCT API | Durum |
|---|---|---|
| **Union (Fuse)** | BRepAlgoAPI_Fuse | ✅ Done |
| **Cut (Subtract)** | BRepAlgoAPI_Cut | ✅ Done |
| **Intersection** | BRepAlgoAPI_Common | ✅ Done |

#### 4.2.4 Primitives (Mevcut)

| Primitive | OCCT API | Durum |
|---|---|---|
| Box | BRepPrimAPI_MakeBox | ✅ Done |
| Cylinder | BRepPrimAPI_MakeCylinder | ✅ Done |
| Sphere | BRepPrimAPI_MakeSphere | ✅ Done |
| Cone | BRepPrimAPI_MakeCone | ✅ Done |
| Torus | BRepPrimAPI_MakeTorus | ⬜ TODO |

### 4.3 Feature Tree (FeatureManager)

**Referans Konseptler:**
- **FreeCAD Body:** Ordered container, her feature öncekine referans (BaseFeature), Tip marker
- **Fusion 360 Timeline:** Kronolojik feature strip, rollTo(), reorder(), suppress, healthState

**MilCAD Hedef:**
```cpp
class FeatureManager {
    std::vector<std::unique_ptr<Feature>> features_;
    int tipIndex_;                    // Rollback bar pozisyonu

    TopoDS_Shape recompute();         // feature[0] → feature[tip] sıralı execute
    void insertFeature(Feature*);     // Tip'ten sonra ekle
    void suppress(int index);         // Feature geçici devre dışı
    void reorder(int from, int to);   // Sıra değiştir (dependency check ile)
};
```

- Kronolojik sırada feature listesi (sol panel QML)
- Feature suppress/resume
- Rollback bar (belirli bir feature'a kadar model durumu)
- Parametre değişikliğinde downstream feature'lar yeniden hesaplanır

### 4.4 Parametric Editing
- Her feature parametreleri düzenlenebilir (double-click)
- Parametre değişikliğinde `recompute()` chain çalışır
- Named parameters: `Length1 = 50mm` (P2)
- Formül desteği: `Length2 = Length1 * 2` (P2)

---

## 5. CAM WORKBENCH — Detaylı Gereksinimler

### 5.1 Genel Bakış
CAM Workbench, 3D modelden CNC işleme yolları (toolpath) üretir ve CODESYS-uyumlu G-code'a dönüştürür. Bu MilCAD'in en kritik ve ayırt edici modülüdür.

**Referans Mimariler:**
- **FreeCAD Path:** Python-based operations, `Path::Command` (G-code komut), `Path::Toolpath` (sıralı komut listesi), `ObjectOp` base class
- **Fusion 360 Manufacturing:** Adaptive Clearing (sabit takım engagement açısı), 2D/3D/Multi-axis ayrımı

### 5.2 CAM Job Yapısı

```
CamJob
├── Machine Setup
│   ├── Machine Type (3-axis / 4-axis)
│   ├── Work Envelope (X, Y, Z limits)
│   ├── Spindle Speed Range
│   └── Controller Type (CODESYS / LinuxCNC / Fanuc / Custom)
├── Stock Definition
│   ├── Stock Type (Box / Cylinder / From Solid)
│   ├── Dimensions
│   ├── Material (Aluminum, Steel, Wood, Plastic, ...)
│   └── Offset from model
├── Work Coordinate System (WCS)
│   ├── Origin Point
│   ├── Orientation
│   └── G54-G59 selection
├── Tool Controllers[]
│   ├── Tool Reference → Tool Library
│   ├── Spindle Speed (RPM)
│   ├── Feed Rate (mm/min)
│   ├── Plunge Rate (mm/min)
│   └── Coolant (On/Off/Mist/Flood)
└── Operations[]
    ├── Facing
    ├── 2D Profile
    ├── 2D Pocket
    ├── Drilling
    ├── Adaptive Clearing  ← Fusion 360'dan ilham
    ├── 3D Contour (Waterline)
    ├── 3D Parallel (Raster)
    └── Engraving
```

### 5.3 Tool Library (Takım Kütüphanesi)

#### Tool Types

| Tip | Parametreler | Kullanım |
|---|---|---|
| **Flat End Mill** | Diameter, Flutes, Cutting Length, Total Length, Shank Dia | Roughing, profiling, pocketing |
| **Ball End Mill** | Diameter, Flutes, Cutting Length | 3D finishing, contour |
| **Bull Nose Mill** | Diameter, Corner Radius, Flutes | Roughing + semi-finishing |
| **Drill** | Diameter, Point Angle (118°/135°), Length | Drilling |
| **Center Drill** | Diameter, Point Angle | Center drilling |
| **Tap** | Diameter, Pitch, Thread Type | Threading |
| **Chamfer Mill** | Diameter, Angle (45°/60°/90°) | Chamfering |
| **Engraving Tool** | Tip Diameter, Half Angle | Engraving, V-carve |
| **Face Mill** | Diameter, Inserts Count | Face milling |
| **Spot Drill** | Diameter, Point Angle | Spot drilling |

#### Tool Data Structure
```cpp
struct Tool {
    int id;
    QString name;
    ToolType type;

    // Geometry
    double diameter;          // mm
    double cuttingLength;     // mm
    double totalLength;       // mm
    double shankDiameter;     // mm
    double cornerRadius;      // mm (bull nose)
    double tipAngle;          // degrees (drill, engraving)
    int fluteCount;

    // Material
    ToolMaterial material;    // HSS, Carbide, Diamond, CBN
    ToolCoating coating;      // TiN, TiAlN, AlTiN, DLC, None

    // Default speeds (overridden in ToolController)
    double defaultSpindleSpeed;  // RPM
    double defaultFeedRate;      // mm/min
    double defaultPlungeRate;    // mm/min

    // Holder
    QString holderName;
    double holderDiameter;
    double holderLength;

    // Serialization
    QJsonObject toJson() const;
    static Tool fromJson(const QJsonObject&);
};
```

### 5.4 CAM Operations — Detaylı Tanımlar

#### 5.4.1 Facing (Yüzey Düzleme)

| Parametre | Tip | Varsayılan | Açıklama |
|---|---|---|---|
| Tool | ToolController ref | - | Takım seçimi |
| Pass Depth | mm | Tool dia × 0.5 | Her geçişteki derinlik |
| Step Over | % veya mm | 70% | Yanal bindirme |
| Stock to Leave | mm | 0 | Finishing payı |
| Feed Direction | Conventional/Climb | Climb | Freze yönü |
| Boundary | Auto/Custom | Auto | İşleme sınırları |
| Clear Height | mm | Stock top + 5 | Güvenli yükseklik |

#### 5.4.2 2D Profile (Contour)

| Parametre | Tip | Varsayılan | Açıklama |
|---|---|---|---|
| Geometry Selection | Edge(s)/Face(s) | - | Profil geometrisi |
| Cut Side | Inside/Outside | Outside | Kesim tarafı |
| Direction | CW/CCW | CCW (Climb) | Kesim yönü |
| Multiple Depths | bool | true | Çoklu derinlik |
| Step Down | mm | 5.0 | Her adımda derinlik |
| Finishing Pass | bool | false | Son düzeltme geçişi |
| Lead-In | Arc/Line/None | Arc | Giriş stratejisi |
| Lead-Out | Arc/Line/None | Arc | Çıkış stratejisi |
| Tab Support | bool | false | Holding tab'lar |

#### 5.4.3 2D Pocket

| Parametre | Tip | Varsayılan | Açıklama |
|---|---|---|---|
| Geometry Selection | Face(s) | - | Pocket geometrisi |
| Strategy | Zigzag/Offset/Spiral | Offset | Pocket temizleme stratejisi |
| Step Over | % | 60% | Yanal bindirme |
| Step Down | mm | 5.0 | Derinlik adımı |
| Ramping | Helical/Linear/Plunge | Helical | Dalma stratejisi |
| Ramp Angle | degrees | 5° | Rampa açısı |
| Island Detection | bool | true | Ada algılama |
| Wall Stock | mm | 0.1 | Duvar payı |
| Floor Stock | mm | 0.05 | Taban payı |

#### 5.4.4 Drilling

| Parametre | Tip | Varsayılan | Açıklama |
|---|---|---|---|
| Geometry Selection | Point(s)/Circle(s) | - | Delik merkezleri |
| Drill Cycle | Standard/Peck/Chip-Breaking | Standard | Delme çevrimi |
| Depth | mm | Through | Delik derinliği |
| Peck Depth | mm | 2× Dia | Peck derinliği |
| G-Code Cycle | G81/G82/G83/G73 | G81 | CNC canned cycle |

#### 5.4.5 Adaptive Clearing (Referans: Fusion 360 HSM)

Fusion 360'ın imza stratejisi. Sabit takım engagement açısı ile yüksek verimli roughing.

| Parametre | Tip | Varsayılan | Açıklama |
|---|---|---|---|
| Stock | Auto/Custom | Auto | Hammadde tanımı |
| Optimal Load | % of tool dia | 40% | Sabit radial immersion |
| Step Down | mm | Tool dia × 1.0 | Derinlik adımı |
| Feed Optimization | bool | true | Lokal geometriye göre feed ayarı |
| Smoothing | bool | true | Toolpath yumuşatma (trochoid) |
| Stock to Leave | mm | 0.5 | Finishing payı |

**Teknik Detay:**
- Sabit tool engagement açısı → eşit chip load → az titreşim, az takım aşınması
- Trochoidal hareket ile full-width slotting'den kaçınma
- 2D (2.5-axis) ve 3D (volumetric) varyantlar
- Stock-aware rest machining (Z-level'lar arası)

### 5.5 Toolpath Renk Kodlaması

| Renk | Hareket Tipi | G-Code |
|---|---|---|
| Sarı | Rapid (hızlı hareket) | G0 |
| Yeşil | Cutting feed (kesim) | G1/G2/G3 |
| Kırmızı | Plunge (dalma) | G1 Z-negatif |
| Mavi | Lead-in / Lead-out | Arc entry/exit |
| Gri | Retract | G0 Z+ |

---

## 5B. NESTING WORKBENCH — Detaylı Gereksinimler

### 5B.1 Genel Bakış
Nesting Workbench, 2D parça profillerini levha/malzeme üzerinde optimum yerleşim ile dizme (nesting) ve kesim yolu oluşturma ortamıdır. CutPro/patternPRO, SigmaNEST, Deepnest gibi endüstriyel nesting yazılımlarından referans alınmıştır.

**Hedef Kullanım Alanları:**
- Lazer/plazma/waterjet/oksijen kesim makineleri
- CNC router ile levha kesim (ahşap, plastik, kompozit)
- Tekstil/deri kesim (Eastman CutPro analog)
- Sac metal parça üretimi (flat pattern nesting)
- Cam kesimi

**Referans Yazılımlar:**

| Yazılım | Vendor | Nesting Tipi | Verimlilik |
|---|---|---|---|
| **CutPro** | Eastman | Basit (yön bazlı: aşağı/sağ/yukarı/sol) | ~85% |
| **patternPRO Nest++** | Eastman | Gelişmiş dikdörtgensel | ~90% |
| **patternPRO Nest++PRO** | Eastman | True shape nesting | ~93.5% |
| **SigmaNEST HD SuperNest** | SigmaTEK | Zaman bazlı iteratif | ~95% |
| **Deepnest** | Open Source | NFP tabanlı iteratif | ~90% |
| **ProNest** | Hypertherm | True shape + common line | ~92% |
| **MilCAD (Hedef)** | — | NFP tabanlı + iteratif iyileştirme | **>90%** |

### 5B.2 Nesting İş Akışı

```
1. Parça Profilleri Hazırla
   ├── Sketch'ten 2D profil al (kapalı wire)
   ├── Part'tan flat pattern çıkar (sheet metal)
   ├── DXF/SVG dosyadan import et
   └── Adet bilgisi gir (her parçadan kaç adet)

2. Malzeme/Levha Tanımla
   ├── Levha boyutları (WxH) — standart veya özel
   ├── Malzeme tipi (sac, ahşap, kumaş, cam, vb.)
   ├── Malzeme yönü (grain direction)
   └── Kullanılabilir artık levha (remnant)

3. Nesting Parametreleri
   ├── Parça-parça boşluk (kerf + clearance)
   ├── Parça-kenar boşluk (edge margin)
   ├── Dönme seçenekleri (0°/90°/180°/270° veya serbest)
   ├── Aynalama izni (flip/mirror)
   ├── Malzeme yönü kısıtlaması
   └── Optimizasyon süresi (zaman bütçesi)

4. Nesting Çalıştır
   ├── NFP hesaplama (No-Fit Polygon)
   ├── Bottom-Left Fill yerleşim
   ├── İteratif iyileştirme (zaman bütçesi içinde)
   └── Çoklu levha desteği

5. Sonuç & Kesim Yolu
   ├── Verimlilik raporu (% kullanım, fire miktarı)
   ├── Parça başına maliyet
   ├── Kesim yolu oluşturma (cut path)
   ├── Common line optimizasyonu
   ├── Lead-in/Lead-out ekleme
   └── DXF/G-Code export
```

### 5B.3 Nesting Veri Modeli

```cpp
// Nest edilecek parça profili
struct NestPart {
    int id;
    QString name;
    TopoDS_Wire outerProfile;            // Dış kontur (kapalı wire)
    std::vector<TopoDS_Wire> holes;      // İç delikler (island)
    int quantity;                         // Adet
    bool allowRotation;                   // Dönmeye izin
    std::vector<double> allowedAngles;   // İzin verilen açılar (derece)
    bool allowMirror;                     // Aynalama izni
    int grainDirection;                   // Malzeme yönü kısıtlaması (-1: yok)
    double priority;                      // Yerleşim önceliği (büyük parçalar önce)
};

// Levha/malzeme tanımı
struct NestSheet {
    int id;
    QString name;
    double width;                         // mm
    double height;                        // mm
    QString materialType;                 // "Steel", "Aluminum", "Wood", etc.
    double thickness;                     // mm
    int grainDirection;                   // 0: yok, 1: X, 2: Y
    TopoDS_Wire boundary;                // Dikdörtgen veya özel şekil
    std::vector<TopoDS_Wire> exclusionZones;  // Kullanılamaz bölgeler
    bool isRemnant;                       // Artık levha mı?
};

// Nesting parametreleri
struct NestParams {
    double partToPartGap;                // mm — parça arası boşluk (kerf + clearance)
    double partToEdgeGap;                // mm — kenar boşluğu
    RotationMode rotationMode;           // None, Quadrant (0/90/180/270), Free
    bool allowMirror;                    // Global aynalama izni
    int maxOptimizationTime;             // Saniye — iteratif iyileştirme süresi
    NestingStrategy strategy;            // BottomLeft, Gravity, Genetic
    bool enableCommonLine;               // Ortak kesim çizgisi
    bool enablePartInPart;               // Parça içinde parça (concavity)
};

// Yerleştirilmiş parça
struct NestPlacement {
    int partId;
    int sheetId;
    gp_Trsf2d transform;                // Pozisyon + rotasyon + mirror
    double rotation;                      // Derece
    bool mirrored;
    TopoDS_Wire transformedProfile;      // Son konumdaki profil
};

// Nesting sonucu
struct NestResult {
    std::vector<NestPlacement> placements;
    double efficiency;                    // % malzeme kullanımı
    double wasteArea;                     // mm² fire alanı
    int sheetsUsed;                       // Kullanılan levha sayısı
    double totalCutLength;               // mm toplam kesim uzunluğu
    double estimatedTime;                // Tahmini kesim süresi (dakika)
    std::vector<TopoDS_Wire> cutPaths;   // Optimize edilmiş kesim yolları
};

// Nesting Job (ana konteyner)
class NestJob {
    std::vector<NestPart> parts_;
    std::vector<NestSheet> sheets_;
    NestParams params_;
    NestResult result_;

    void addPart(const NestPart& part);
    void addSheet(const NestSheet& sheet);
    bool runNesting();                    // Ana nesting algoritması
    NestResult getResult() const;
    void exportDXF(const QString& path);
    void exportGCode(const QString& path, PostProcessor* pp);
};
```

### 5B.4 Nesting Algoritmaları

#### 5B.4.1 No-Fit Polygon (NFP)
İki parçanın örtüşmeden yerleşebileceği tüm pozisyonları tanımlayan temel geometrik araç.

**Hesaplama Yöntemleri:**
1. **Orbiting Algorithm** (Mahadevan 1984): Parça B'yi parça A etrafında kaydırarak NFP üret
2. **Minkowski Sum:** Matematiksel olarak daha zarif, konveks parçalar için hızlı
3. **OCCT ile NFP:** `BRepOffsetAPI_MakeOffset` + `BRepAlgoAPI_Fuse` kullanarak

```cpp
class NFPGenerator {
    // İki parça profili arasında No-Fit Polygon hesapla
    TopoDS_Wire computeNFP(const TopoDS_Wire& fixedPart,
                            const TopoDS_Wire& movingPart);

    // Inner-Fit Rectangle: Levha sınırları içinde yerleşim alanı
    TopoDS_Wire computeIFR(const TopoDS_Wire& sheetBoundary,
                            const TopoDS_Wire& part);

    // Feasible placement region: NFP ∩ IFR
    TopoDS_Wire computeFeasibleRegion(
        const std::vector<TopoDS_Wire>& placedNFPs,
        const TopoDS_Wire& ifr);
};
```

#### 5B.4.2 Bottom-Left Fill (BLF)
```cpp
class PartPlacer {
    // Bottom-Left Fill: En altta ve en solda uygun pozisyonu bul
    gp_Pnt2d findBottomLeftPosition(
        const TopoDS_Wire& part,
        const std::vector<NestPlacement>& placed,
        const NestSheet& sheet,
        const NestParams& params);

    // Gravity placement: Ağırlık merkezi bazlı yerleşim
    gp_Pnt2d findGravityPosition(/* ... */);
};
```

#### 5B.4.3 İteratif İyileştirme
```cpp
class NestingOptimizer {
    // Zaman bütçesi içinde sürekli iyileştir
    NestResult optimize(const NestResult& initial,
                        int timeBudgetSeconds);

    // Parça sırasını değiştirerek deneme (genetic / simulated annealing)
    std::vector<int> generatePermutation(const std::vector<int>& order);

    // Rotasyon kombinasyonlarını dene
    void tryRotations(NestResult& result, int partIndex);
};
```

### 5B.5 Cut Path Optimization (Kesim Yolu Optimizasyonu)

Nesting sonrası kesim yolu oluşturma — CutPro'nun en güçlü özelliği.

| Özellik | Açıklama | Referans |
|---|---|---|
| **Cut Order** | Parça kesim sırası optimizasyonu (nearest-neighbor TSP) | CutPro, ProNest |
| **Common Line** | Bitişik parçaların ortak kenar paylaşımı — malzeme + zaman tasarrufu | SigmaNEST, ProNest |
| **Lead-In/Lead-Out** | Kesim giriş/çıkış noktası ve stratejisi (arc/line/none) | Tüm cutting CAM |
| **Bridge/Tab** | Parçaların levhadan düşmemesi için bağlantı noktaları | Lazer/plazma |
| **Kerf Compensation** | Kesim genişliği telafisi (G41/G42 veya offset) | Tüm |
| **Direction Control** | CW/CCW kesim yönü (climb/conventional) | CutPro |
| **Continuous Cut** | Kafa kaldırmadan sürekli kesim (InMotion — CutPro) | CutPro InMotion |
| **Remnant Tracking** | Artık levha kaydı ve sonraki job'da kullanımı | SigmaNEST |

```cpp
class CutPathOptimizer {
    // Kesim yollarını optimize et
    std::vector<TopoDS_Wire> optimizeCutOrder(
        const std::vector<NestPlacement>& placements,
        const NestSheet& sheet);

    // Common line tespit ve birleştirme
    void mergeCommonLines(std::vector<TopoDS_Wire>& cutPaths,
                          double tolerance);

    // Lead-in/Lead-out ekleme
    void addLeadInOut(TopoDS_Wire& cutPath,
                      LeadType leadIn, LeadType leadOut,
                      double radius);

    // Tab/bridge ekleme
    void addTabs(TopoDS_Wire& cutPath,
                 int tabCount, double tabWidth);
};
```

### 5B.6 Nesting UI Gereksinimleri

```
NestingToolbar.qml
├── Parts Panel (Sol)
│   ├── Parça listesi (isim, adet, boyut)
│   ├── Parça ekleme (Sketch'ten / DXF'ten / Part flat pattern)
│   ├── Adet düzenleme
│   ├── Dönme/aynalama seçenekleri (parça bazlı)
│   └── Öncelik sırası
│
├── Sheet Panel (Sağ-üst)
│   ├── Levha boyutları (WxH)
│   ├── Malzeme tipi seçimi
│   ├── Standart levha kütüphanesi
│   ├── Özel şekil levha (custom boundary)
│   └── Artık levha (remnant) ekleme
│
├── Nesting Parameters Panel
│   ├── Parça-parça boşluk (mm)
│   ├── Parça-kenar boşluk (mm)
│   ├── Dönme modu (Yok / 90° / Serbest)
│   ├── Aynalama izni
│   ├── Optimizasyon süresi (slider)
│   └── Nesting stratejisi seçimi
│
├── Viewport (Merkez)
│   ├── 2D levha görünümü (top-down)
│   ├── Yerleştirilmiş parçalar (renk kodlu)
│   ├── Boşluk alanları (yarı-şeffaf)
│   ├── Kesim yolu gösterimi (çizgi renkleri)
│   ├── Zoom/pan (2D modda)
│   └── Parça sürükle-bırak (manuel düzenleme)
│
├── Results Panel (Alt)
│   ├── Verimlilik yüzdesi (% kullanım)
│   ├── Fire alanı (mm²)
│   ├── Toplam kesim uzunluğu (mm)
│   ├── Tahmini kesim süresi
│   ├── Levha başına maliyet
│   └── Levha sayısı
│
└── Actions
    ├── "Nest" butonu (otomatik nesting çalıştır)
    ├── "Optimize" butonu (mevcut sonucu iyileştir)
    ├── DXF Export (yerleşim planı)
    ├── G-Code Export (kesim yolu)
    ├── PDF Report (verimlilik raporu)
    └── Remnant Save (artık levha kaydet)
```

### 5B.7 Nesting Renk Kodlaması

| Renk | Anlam |
|---|---|
| Açık Mavi | Yerleştirilmiş parça profili |
| Koyu Mavi | Seçili parça |
| Yeşil | Ortak kesim çizgisi (common line) |
| Sarı | Kesim yolu (cut path) |
| Kırmızı | Lead-in/Lead-out |
| Gri (yarı-şeffaf) | Fire alanı (waste) |
| Turuncu | Tab/bridge noktaları |
| Beyaz kesikli | Levha sınırları |

---

## 6. G-CODE GENERATOR & CODESYS POST-PROCESSOR

### 6.1 G-Code Output Format

MilCAD'in ürettiği G-code, CODESYS SoftMotion CNC'nin SMC_NCInterpreter function block'u tarafından doğrudan parse edilebilir olmalıdır.

#### 6.1.1 Desteklenen G-Code Komutları (DIN 66025 / CODESYS)

**Hareket Komutları:**

| G-Code | Açıklama | CODESYS Desteği |
|---|---|---|
| G0 | Rapid positioning | ✅ SMC_NCInterpreter |
| G1 | Linear interpolation | ✅ |
| G2 | Circular interpolation CW | ✅ |
| G3 | Circular interpolation CCW | ✅ |
| G4 | Dwell (bekleme) | ✅ |
| G17/G18/G19 | XY/XZ/YZ plane selection | ✅ |
| G20 | Subroutine jump | ✅ CODESYS specific |
| G28 | Return to home | ✅ |
| G40/G41/G42 | Tool compensation cancel/left/right | ✅ SM3_CNC lib |
| G43/G49 | Tool length comp +/cancel | ✅ |
| G53 | Machine coordinate | ✅ |
| G54-G59 | Work coordinate systems | ✅ |
| G73 | Peck drilling (chip break) | ✅ |
| G75 | Reset before jump | ✅ CODESYS specific |
| G80-G83 | Canned drill cycles | ✅ |
| G90/G91 | Absolute/Incremental positioning | ✅ |

**M-Code Komutları:**

| M-Code | Açıklama |
|---|---|
| M0/M1 | Program stop / Optional stop |
| M2/M30 | Program end / End & rewind |
| M3/M4/M5 | Spindle CW/CCW/Stop |
| M6 | Tool change |
| M7/M8/M9 | Mist/Flood coolant on/off |

### 6.2 Post-Processor Mimarisi

**Referans Mimariler:**
- **Fusion 360:** JavaScript-based (.cps), 100+ makine-spesifik post, Autodesk Post Library
- **FreeCAD:** Python scripts, `Processor.py` base class, 38+ post-processor

**MilCAD Yaklaşımı: C++ Class Hierarchy**

```cpp
class PostProcessor {
public:
    virtual QString generatePreamble(const CamJob& job) = 0;
    virtual QString generateToolChange(const ToolController& tc) = 0;
    virtual QString generateMotion(const ToolpathSegment& seg) = 0;
    virtual QString generatePostamble() = 0;
    virtual QString formatCoord(double value) const;

    QString generate(const CamJob& job);   // Full G-code output
};

class CodesysPostProcessor : public PostProcessor {
public:
    enum OutputMode {
        MODE_FILE,            // ASCII G-code dosyası
        MODE_SMC_CNC_REF,     // IEC variable declaration
        MODE_SMC_OUTQUEUE     // SMC_GEOINFO struct array
    };

    struct Config {
        OutputMode mode = MODE_FILE;
        bool useLineNumbers = true;
        int lineNumberIncrement = 10;
        bool useG75ResetBeforeJump = true;
        int maxAxes = 4;
        QString decimalFormat = "%.4f";
        bool useToolChangeM6 = true;
    };
};

class LinuxCNCPostProcessor : public PostProcessor { ... };
class GrblPostProcessor : public PostProcessor { ... };
```

### 6.3 CODESYS G-Code Çıktı Örneği

```gcode
; MilCAD Generated G-Code
; CODESYS SoftMotion CNC Compatible (DIN 66025)
; Date: 2026-03-16
; Job: Part1_Machining
; Machine: 3-Axis Mill
; Controller: CODESYS SoftMotion CNC
; Post-Processor: MilCAD CODESYS v1.0

; === PREAMBLE ===
N10 G90 G21                    ; Absolute mode, Metric (mm)
N20 G17                        ; XY Plane
N30 G40 G49 G80                ; Cancel compensations & cycles
N40 G54                        ; Work Coordinate System 1

; === TOOL CHANGE: 10mm Flat End Mill ===
N50 T1 M6                     ; Tool 1, Tool change
N60 S8000 M3                  ; Spindle 8000 RPM, CW
N70 M8                        ; Coolant ON

; === OPERATION 1: Facing ===
N80 G0 Z50.0000               ; Safe height
N90 G0 X-5.0000 Y-5.0000      ; Rapid to start
N100 G0 Z5.0000               ; Approach
N110 G1 Z0.0000 F400          ; Plunge to Z0
N120 G1 X105.0000 F1200       ; Cut pass 1
N130 G1 Y2.0000               ; Step over
N140 G1 X-5.0000              ; Cut pass 2 (return)

; === POSTAMBLE ===
N2000 G0 Z50.0000             ; Retract
N2010 M5                      ; Spindle stop
N2020 M9                      ; Coolant off
N2030 G28                     ; Home position
N2040 M30                     ; Program end
```

---

## 7. SIMULATION ENGINE

### 7.1 G-Code Simulation
- **Step-by-step execution:** Her G-code satırı ayrı adım
- **Speed control:** 0.1x - 100x hız
- **Pause/Resume/Reset**
- **Material removal visualization:** Stock'tan malzeme kaldırma (dexel-based)
- **Tool collision detection:** Takım, holder, stock çarpışma kontrolü
- **Estimated machining time:** Feed rate × distance hesabı

### 7.2 Material Removal Algorithm
- **Dexel-based rendering:** Z-buffer yaklaşımı ile stock removal
- Her toolpath segmenti için cylindrical/spherical cutter sweep
- Real-time OpenGL rendering ile material kaldırma görselleştirme

---

## 8. VISUALIZATION & UI/UX

### 8.1 3D Viewer (OCCT V3d Integration)

| Özellik | İmplementasyon | Durum |
|---|---|---|
| **Rendering** | OCCT V3d_View + OpenGL FBO (GLX) | ✅ Done |
| **Projection** | Perspective + Orthographic | ✅ Done (toggle eksik) |
| **ViewCube** | Custom OCCT render + click handling | ✅ Done |
| **Mouse Orbit** | AIS_ViewController (middle-button) | ✅ Done |
| **Pan** | Middle-button + Shift | ✅ Done |
| **Zoom** | Scroll wheel | ✅ Done |
| **Selection** | AIS_InteractiveContext | ✅ Done |
| **Highlight** | Hover renk değişimi | ✅ Done |
| **Grid** | OCCT Grid, configurable spacing | ✅ Done |
| **Axis Triad** | OCCT ViewTrihedron | ✅ Done |
| **View Presets** | Front/Back/Right/Left/Top/Bottom/Iso | ✅ Done |
| **Background** | Gradient (koyu tema) | ✅ Done |
| **Multi-select** | Ctrl+Click | ⬜ TODO |
| **Box select** | Sürükle-seç | ⬜ TODO |
| **Selection filter** | Face/Edge/Vertex mode | ⬜ TODO |
| **Shading modes** | Wireframe/Shaded/Shaded+Edges | ⬜ TODO |
| **Section view** | Clipping plane | ⬜ TODO |

### 8.2 Dark Theme

| Element | Hex |
|---|---|
| Background Top | #2D2D30 |
| Background Bottom | #1E1E1E |
| Panel Background | #252526 |
| Accent Color | #1D4ED8 (Blue) |
| Success | #059669 (Green) |
| Warning | #D97706 (Amber) |

### 8.3 Keyboard Shortcuts

| Kısayol | Aksiyon | Durum |
|---|---|---|
| Escape | Cancel / Deselect | ✅ Done |
| Delete | Seçimi sil | ✅ Done |
| Ctrl+Z / Ctrl+Y | Undo / Redo | ✅ Done |
| L | Line tool | ✅ Done |
| C | Circle tool | ✅ Done |
| R | Rectangle tool | ✅ Done |
| A | Arc tool | ✅ Done |
| E | Ellipse tool | ✅ Done |
| S | Spline tool | ✅ Done |
| P | Polygon tool | ✅ Done |
| T | Trim | ✅ Done |
| G | Construction toggle | ✅ Done |
| 1-6 | View presets | ✅ Done |
| 0 | Isometric view | ✅ Done |
| F | Fit all | ✅ Done |

---

## 9. DATA EXCHANGE & FILE I/O

### 9.1 Desteklenen Formatlar

| Format | Import | Export | OCCT API | Öncelik |
|---|---|---|---|---|
| **.milcad** | ✅ | ✅ | Custom (ZIP+JSON+BREP) | P1 |
| **STEP (.step/.stp)** | ✅ | ✅ | STEPControl_Reader/Writer | P0 |
| **IGES (.iges/.igs)** | ✅ | ✅ | IGESControl_Reader/Writer | P1 |
| **STL (.stl)** | ✅ | ✅ | StlAPI_Reader/Writer | P0 |
| **DXF (.dxf)** | ✅ | ✅ | Custom parser | P1 |
| **SVG (.svg)** | ✅ | ✅ | Custom | P2 |
| **G-Code (.nc/.gcode)** | ✅ | ✅ | Custom parser | P1 |

### 9.2 Native File Format (.milcad)

```
.milcad dosyası = ZIP Archive (Ref: FreeCAD .FCStd)
├── document.json          # Feature tree, constraint data, parameters
├── shapes/                # B-Rep geometriler (.brep)
├── sketches/              # SketchDocument serialization (entity + constraint)
├── cam/                   # CAM job definition + G-code
├── thumbnails/            # 256×256 thumbnail
└── metadata.json          # File version, author, dates, units
```

---

## 10. NON-FUNCTIONAL REQUIREMENTS

### 10.1 Performance Targets

| Metrik | Hedef |
|---|---|
| Application startup | < 3 sec |
| File open (10MB STEP) | < 5 sec |
| Boolean operation (complex) | < 2 sec |
| Toolpath generation (simple pocket) | < 3 sec |
| G-code generation | < 1 sec |
| Viewport FPS (10K triangles) | > 60 FPS |
| Viewport FPS (1M triangles) | > 30 FPS |
| Memory usage (typical session) | < 500 MB |
| Undo/Redo response | < 100 ms |
| Constraint solver (20 entities) | < 50 ms |

### 10.2 Feature Priority Matrix — Güncel Durum

#### P0 — Olmazsa Olmaz

**Görüntüleme & Navigasyon**

| ID | Özellik | Durum |
|---|---|---|
| F001 | 3D viewport (OCCT FBO rendering) | ✅ Done |
| F002 | Orbit, pan, zoom (AIS_ViewController) | ✅ Done |
| F003 | Standart görünümler — Üst, Ön, Sağ, İzometrik | ✅ Done |
| F004 | Perspective ↔ Orthographic geçişi | ⬜ TODO |
| F005 | Fit All | ✅ Done |
| F006 | Grid gösterimi | ✅ Done |
| F007 | XYZ eksen göstergesi | ✅ Done |
| F008 | Koyu tema | ✅ Done |

**2D Çizim (Sketch)**

| ID | Özellik | Durum |
|---|---|---|
| F009 | Çizgi çizme (LineStrategy) | ✅ Done |
| F010 | Daire çizme (CircleStrategy) | ✅ Done |
| F011 | Yay çizme (ArcThreePointStrategy) | ✅ Done |
| F012 | Dikdörtgen çizme (RectangleStrategy) | ✅ Done |
| F013 | Çoklu çizgi — polyline (PolylineStrategy) | ✅ Done |
| F014 | Construction line (isConstruction_ flag) | ✅ Done |
| F015 | Sketch düzlem seçimi — XY, XZ, YZ (WorkbenchManager) | ✅ Done |

**Yakalama (Snap) Sistemi**

| ID | Özellik | Durum |
|---|---|---|
| F016 | Uç nokta yakalama (endpoint) | ✅ Done |
| F017 | Orta nokta yakalama (midpoint) | ✅ Done |
| F018 | Merkez yakalama (center) | ✅ Done |
| F019 | Kesişim noktası yakalama (intersection) | ✅ Done |
| F020 | Grid yakalama | ✅ Done |
| F021 | Dik nokta yakalama (perpendicular) | ✅ Done |
| F022 | Teğet noktası yakalama (tangent) | ✅ Done |
| F023 | En yakın nokta yakalama (nearest) | ✅ Done |
| F024 | Snap gösterge ikonları (SnapMarkerPresenter) | 🟨 Partial (text label, ikon yok) |

**Kısıtlamalar (Constraints)**

| ID | Özellik | Durum |
|---|---|---|
| F025 | Yatay kısıtlama (SketchConstraintHorizontal) | ✅ Done |
| F026 | Dikey kısıtlama (SketchConstraintVertical) | ✅ Done |
| F027 | Çakışıklık (SketchConstraintCoincident) | ✅ Done |
| F028 | Dik açı (SketchConstraintPerpendicular) | ✅ Done |
| F029 | Paralel (SketchConstraintParallel) | ✅ Done |
| F030 | Teğet (SketchConstraintTangent) | ✅ Done |
| F031 | Eşit uzunluk/yarıçap (SketchConstraintEqual) | ✅ Done |
| F032 | Sabit (SketchConstraintFixed) | ✅ Done |
| F033 | Simetri (SketchConstraintSymmetric) | ✅ Done |
| F034 | Boyut — distance (SketchConstraintDistance) | ✅ Done |
| F035 | Boyut — radius (SketchConstraintRadius) | ✅ Done |
| F036 | Boyut — açı (SketchConstraintAngle) | ✅ Done |
| F037 | Constraint renk göstergesi (DOF color coding) | ✅ Done |
| F038 | DOF göstergesi (SketchDofAnalyzer) | ✅ Done |

**3D Modelleme**

| ID | Özellik | Durum |
|---|---|---|
| F039 | Extrude (Pad) | ✅ Done (PadFeature) |
| F040 | Cut (Pocket) | ⬜ Phase 2 |
| F041 | Revolution | ⬜ Phase 2 |
| F042 | 3D Fillet | ⬜ Phase 2 |
| F043 | 3D Chamfer | ⬜ Phase 2 |
| F044 | Boolean: union, cut, intersect | ✅ Done |
| F045 | Kutu, silindir, küre, koni primitifleri | ✅ Done |

**Seçim**

| ID | Özellik | Durum |
|---|---|---|
| F046 | Tek tıklama ile seçim | ✅ Done |
| F047 | Çoklu seçim (Ctrl+Click) | ⬜ TODO |
| F048 | Kutu seçim: L→R window (tam içinde), R→L crossing (kesişen) | ⬜ TODO |
| F049 | Seçim filtresi: V=vertex, E=edge, X=face (SolidWorks stili) | ⬜ TODO |
| F050 | Seçim vurgulama (highlight) | ✅ Done |
| F051 | Seçilen elemanın özelliklerini gösterme | ⬜ TODO |

**Model Ağacı**

| ID | Özellik | Durum |
|---|---|---|
| F052 | Feature tree (ModelTreePanel.qml) | 🟨 Partial |
| F053 | Feature seçme → 3D vurgulama | ⬜ TODO |
| F054 | Feature silme ve yeniden hesaplama | ⬜ TODO |
| F055 | Feature düzenleme (parametreleri değiştir) | ⬜ TODO |

**Geri Alma**

| ID | Özellik | Durum |
|---|---|---|
| F056 | Undo (CommandHistory) | ✅ Done |
| F057 | Redo (CommandHistory) | ✅ Done |

**Dosya İşlemleri**

| ID | Özellik | Durum |
|---|---|---|
| F058 | Yeni doküman oluşturma | ✅ Done (FileManager) |
| F059 | Kaydetme / Farklı kaydetme | ✅ Done (FileManager) |
| F060 | STEP dosya import | ✅ Done (FileManager) |
| F061 | STEP dosya export | ✅ Done (FileManager) |
| F062 | STL export | ⬜ TODO |

---

#### P1 — Çok Önemli

**Çizim Araçları (Ek)**

| ID | Özellik | Durum |
|---|---|---|
| F063 | Elips çizme (EllipseStrategy) | ✅ Done |
| F064 | Spline çizme (SplineStrategy) | ✅ Done |
| F065 | Düzgün çokgen (PolygonStrategy) | ✅ Done |
| F066 | Metin/yazı (engrave için) | ⬜ TODO |
| F067 | Offset (SketchOffsetTool) | ✅ Done |
| F068 | Trim (SketchTrimTool) | ✅ Done |
| F069 | Extend | ⬜ TODO |
| F070 | Mirror | ⬜ TODO |
| F071 | 2D fillet (SketchFilletTool) | ✅ Done |
| F072 | 2D chamfer | ✅ Done |

**Kısıtlamalar (Ek)**

| ID | Özellik | Durum |
|---|---|---|
| F073a | Midpoint constraint (SketchConstraintMidpoint) | ✅ Done |
| F073b | Collinear constraint | ⬜ TODO |
| F073c | Concentric constraint | ⬜ TODO |

**Çizim Yardımcıları**

| ID | Özellik | Durum |
|---|---|---|
| F074 | Dinamik boyut girişi (DimensionInput.qml) | ✅ Done |
| F075 | Ortho modu (cardinal alignment) | ✅ Done |
| F076 | Inference lines (InferenceLinePresenter) | ✅ Done |
| F077 | Otomatik constraint tanıma | ⬜ Phase 1+ |
| F078 | Koordinat gösterimi (cursorWorldX/Y/Z) | ✅ Done |
| F079 | Ölçü birimi seçimi (mm / inch) | ⬜ TODO |

**3D Modelleme (Ek)**

| ID | Özellik | Durum |
|---|---|---|
| F080 | Loft | ⬜ P1 |
| F081 | Sweep | ⬜ P1 |
| F082 | Shell | ⬜ P1 |
| F083 | Hole wizard | ⬜ P1 |
| F084 | Linear pattern | ⬜ P1 |
| F085 | Circular pattern | ⬜ P1 |
| F086 | Mirror feature | ⬜ P1 |
| F087 | Draft angle | ⬜ P1 |

**Görüntüleme (Ek)**

| ID | Özellik | Durum |
|---|---|---|
| F088 | Wireframe / Shaded / Shaded+Edges | ⬜ TODO |
| F089 | Şeffaflık ayarı | ⬜ TODO |
| F090 | Kesit görünümü (section view) | ⬜ TODO |
| F091 | ViewCube | ✅ Done |
| F092 | Ölçü aracı — mesafe, açı, alan | ⬜ TODO |

**Dosya İşlemleri (Ek)**

| ID | Özellik | Durum |
|---|---|---|
| F093 | IGES import/export | ✅ Done (FileManager) |
| F094 | DXF import/export (2D) | ✅ Done (FileManager) |
| F095 | Native .milcad format | ⬜ Phase 3 |
| F096 | Son açılan dosyalar listesi | ⬜ TODO |
| F097 | Otomatik kaydetme (auto-save) | ⬜ TODO |

**CAM — Temel**

| ID | Özellik | Durum |
|---|---|---|
| F098 | CAM Job tanımlama | ⬜ Phase 4 |
| F099 | Hammadde (stock) tanımı | ⬜ Phase 4 |
| F100 | Takım kütüphanesi | ⬜ Phase 4 |
| F101 | Facing operasyonu | ⬜ Phase 4 |
| F102 | 2D profil (contour) | ⬜ Phase 4 |
| F103 | 2D pocket | ⬜ Phase 4 |
| F104 | Drilling | ⬜ Phase 4 |
| F105 | Toolpath renk kodlaması | ⬜ Phase 4 |
| F106 | G-Code üretimi (DIN 66025) | ⬜ Phase 5 |
| F107 | G-Code preview panel | ⬜ Phase 5 |
| F108 | G-Code export | ⬜ Phase 5 |

---

#### P2 — Önemli

**CAM — İleri**

| ID | Özellik | Durum |
|---|---|---|
| F109 | Adaptive clearing (HSM) | ⬜ Phase 4+ |
| F110 | 3D contour (waterline) | ⬜ Phase 4+ |
| F111 | 3D parallel (raster) | ⬜ Phase 4+ |
| F112 | Lead-in / Lead-out | ⬜ Phase 4 |
| F113 | Tab support | ⬜ Phase 4 |
| F114 | Rest machining | ⬜ Phase 4+ |
| F115 | Tool radius compensation (G41/G42) | ⬜ Phase 5 |
| F116 | Post-processor seçimi (CODESYS/LinuxCNC/Grbl) | ⬜ Phase 5 |
| F117 | CODESYS SMC_CNC_REF export | ⬜ Phase 5 |
| F118 | CODESYS SMC_OUTQUEUE export | ⬜ Phase 5 |

**Nesting — Temel**

| ID | Özellik | Durum |
|---|---|---|
| F135 | Nesting Workbench UI (NestingToolbar.qml) | ⬜ Phase 7 |
| F136 | Parça profili ekleme (Sketch/DXF/Part flat pattern) | ⬜ Phase 7 |
| F137 | Levha/malzeme tanımlama (boyut, tip, kalınlık) | ⬜ Phase 7 |
| F138 | NFP (No-Fit Polygon) hesaplama | ⬜ Phase 7 |
| F139 | Bottom-Left Fill yerleşim algoritması | ⬜ Phase 7 |
| F140 | Rectangular nesting (basit dikdörtgen yerleşim) | ⬜ Phase 7 |
| F141 | True shape nesting (gerçek profil yerleşimi) | ⬜ Phase 7 |
| F142 | Dönme desteği (0°/90°/180°/270°/serbest) | ⬜ Phase 7 |
| F143 | Parça-parça ve parça-kenar boşluk ayarı | ⬜ Phase 7 |
| F144 | Verimlilik raporu (% kullanım, fire) | ⬜ Phase 7 |

**Nesting — İleri**

| ID | Özellik | Durum |
|---|---|---|
| F145 | İteratif iyileştirme (zaman bütçeli optimizasyon) | ⬜ Phase 7 |
| F146 | Common line optimizasyonu (ortak kesim çizgisi) | ⬜ Phase 7 |
| F147 | Cut path oluşturma ve optimizasyonu | ⬜ Phase 7 |
| F148 | Lead-in/Lead-out ekleme (kesim giriş/çıkış) | ⬜ Phase 7 |
| F149 | Tab/bridge desteği (parça tutma noktaları) | ⬜ Phase 7 |
| F150 | Parça içinde parça yerleşimi (part-in-part) | ⬜ Phase 7 |
| F151 | Aynalama desteği (mirror/flip) | ⬜ Phase 7 |
| F152 | Artık levha yönetimi (remnant tracking) | ⬜ Phase 7 |
| F153 | Çoklu levha nesting | ⬜ Phase 7 |
| F154 | Standart levha kütüphanesi | ⬜ Phase 7 |
| F155 | DXF export (nesting planı) | ⬜ Phase 7 |
| F156 | G-Code export (kesim yolu) | ⬜ Phase 7 |
| F157 | PDF rapor (verimlilik/maliyet) | ⬜ Phase 7 |
| F158 | Malzeme yönü kısıtlaması (grain direction) | ⬜ Phase 7 |
| F159 | Manuel sürükle-bırak düzenleme | ⬜ Phase 7 |
| F160 | Sheet metal flat pattern → nesting entegrasyonu | ⬜ Phase 7 |

**Simülasyon**

| ID | Özellik | Durum |
|---|---|---|
| F119 | G-Code adım adım simülasyon | ⬜ Phase 6 |
| F120 | Takım animasyonu | ⬜ Phase 6 |
| F121 | Malzeme kaldırma görselleştirme | ⬜ Phase 6 |
| F122 | Çarpışma kontrolü | ⬜ Phase 6 |

**Kullanılabilirlik**

| ID | Özellik | Durum |
|---|---|---|
| F123 | Komut satırı (AutoCAD-style) | ⬜ TODO |
| F124 | Klavye kısayol özelleştirme | ⬜ TODO |
| F125 | Çoklu dil desteği (TR, EN) | ✅ Done |
| F126 | Sağ-tık bağlam menüsü | ⬜ TODO |
| F127 | Parametrik: isimli parametreler | ⬜ P2 |
| F128 | Parametrik: formül desteği | ⬜ P2 |

---

#### P3 — İyi Olur (Gelecek Versiyon)

| ID | Özellik | Durum |
|---|---|---|
| F129 | Assembly (montaj) | ⬜ v2.0 |
| F130 | 4/5-axis CAM | ⬜ v2.0 |
| F131 | Turning (torna) | ⬜ v2.0 |
| F132 | SVG import/export | ⬜ TODO |
| F133 | Macro / scripting | ⬜ v2.0 |
| F134 | Plugin sistemi | ⬜ v2.0 |
| F161 | Batch nesting (çoklu job kuyruğu — patternPRO analog) | ⬜ v2.0 |
| F162 | ERP entegrasyonu (iş emri → nesting — EasiSelect analog) | ⬜ v2.0 |
| F163 | Genetik algoritma nesting (meta-heuristic) | ⬜ v2.0 |
| F164 | Continuous cut while convey (CutPro InMotion analog) | ⬜ v2.0 |

---

#### Özet İstatistikler

| Öncelik | Toplam | ✅ Done | 🟨 Partial | ⬜ TODO |
|---|---|---|---|---|
| P0 | 62 | 46 | 2 | 14 |
| P1 | 46 | 16 | 0 | 30 |
| P2 | 46 | 1 | 0 | 45 |
| P3 | 6 | 0 | 0 | 6 |
| **Toplam** | **160** | **63** | **2** | **95** |

**Not:** Nesting özellikleri (F135-F160) P2 kategorisine eklendi. Faz 7 olarak planlandı.

### 10.3 Platform Requirements

| Platform | Minimum | Önerilen |
|---|---|---|
| OS | Windows 10 64-bit / Ubuntu 20.04 | Windows 11 / Ubuntu 22.04+ |
| CPU | 4 core, 2 GHz | 8 core, 3+ GHz |
| RAM | 4 GB | 16 GB |
| GPU | OpenGL 3.3 | OpenGL 4.5 + 4GB VRAM |
| Disk | 500 MB install | SSD önerilir |
