# MilCAM Phase 1 (P1) — DXF Import + Canvas Editing

> Goal: Enable the operator to open a DXF file and see the 2D geometry on the grid canvas.
> Geometry renders as line segments. Coordinate display, pan/zoom, basic undo/redo.
> No CAM operations yet (P3); no 3D (GL-free, raster only).

## Scope

### In Scope
- **DXF parser:** read DXF LINES + LWPOLYLINE sections → `std::vector<LineSegment>`
  - Filter to 2D (z ≈ 0), skip arcs/splines for now
  - Keep segment endpoints: `{Point2D start, Point2D end, string layername}`
- **Canvas rendering:** draw LineSegment[] as white lines on grid (1:1 scale initially)
- **Open dialog:** file picker → parse DXF → populate canvas
- **Coordinate display:** show cursor (x, y) in status bar as mouse moves / clicks
- **Pan/zoom:** mouse wheel zoom, click-drag pan (Qt GraphicsView built-in)
- **Undo/redo:** stack of geometry states, Ctrl+Z/Ctrl+Y
- **Keyboard shortcuts:** Ctrl+O (open), Ctrl+Z/Ctrl+Y (undo/redo)

### Out of Scope
- **DXF arcs/splines** (P2, later)
- **CAM operations** (profiling, pocketing, drilling — P3)
- **3D view** (GL is not available; 2D only)
- **Dimensioning, annotations** (P4)
- **Multi-body, assembly** (future)

## Architecture

### Core Layer (milcam/core/)
```
milcam/core/
  geometry/
    Point2D.h              # (x, y) struct
    LineSegment.h          # {Point2D start, end, layername}
    Bounds2D.h             # axis-aligned bounding box
  dxf/
    DxfParser.h/cpp        # read DXF file → LineSegment[]
    DxfEntity.h            # internal DXF line/polyline enum
  document/
    GeoDocument.h/cpp      # geometry container + undo/redo stack
    UndoStack.h            # command pattern: Command interface + implementations
```

### App Layer (milcam/app/)
```
milcam/app/src/gui/
  CanvasView.h/cpp        # (existing, placeholder) → render LineSegment[]
  CanvasScene.cpp         # (expand) QPainter draw grid + lines
  CanvasModel.*           # NEW: abstraction for canvas state
  FileDialog.*            # NEW: QFileDialog + DXF filter
  CoordinateDisplay.*     # update status bar with (x, y)
```

### Tests (tests/)
```
tests/core/
  test_dxf_parser.cpp     # LineSegment[] from sample DXFs
  test_document.cpp       # undo/redo, bounds computation
  test_geobounds.cpp      # bounding box logic
```

---

## Implementation Roadmap

### Task 1: Core Geometry Types (TDD)

**Files:**
- Create: `milcam/core/geometry/Point2D.h`
- Create: `milcam/core/geometry/LineSegment.h`
- Create: `milcam/core/geometry/Bounds2D.h/cpp`
- Create: `tests/core/test_geobounds.cpp`

**Checklist:**
- [ ] Define `Point2D { double x, y; }` with basic ops (translate, scale, distance)
- [ ] Define `LineSegment { Point2D start, end; std::string layer; }`
- [ ] Define `Bounds2D { double minX, maxX, minY, maxY; }` with merge, expand, scale methods
- [ ] Write tests: bounds of empty set, bounds of single/multiple segments, scale/translate
- [ ] Run: `bash scripts/build-host.sh` → ALL PASS
- [ ] Commit: `feat(core): Point2D, LineSegment, Bounds2D geometry primitives`

---

### Task 2: DXF Parser (TDD, flat DXF only)

**Files:**
- Create: `milcam/core/dxf/DxfEntity.h` (internal enum)
- Create: `milcam/core/dxf/DxfParser.h/cpp`
- Create: `milcam/core/dxf/sample_files/` (test .dxf)
- Create: `tests/core/test_dxf_parser.cpp`
- Modify: `tests/CMakeLists.txt`

**Spec:**
- Input: filename (string)
- Output: `std::vector<LineSegment>`
- Read DXF ENTITIES section, extract LINE + LWPOLYLINE (single-segment only for now)
- Skip Z ≠ 0, skip arcs/splines
- Return all segments in model space (handle layers)

**Checklist:**
- [ ] Write failing test: parse a simple 2-line DXF → 2 segments
- [ ] Implement: regex or naive line-by-line parser (DXF is text-based)
  - Find `SECTION ENTITIES` block
  - For each `LINE`: read start (x, y) + end (x, y) → LineSegment
  - For each `LWPOLYLINE`: read vertices → split into segments
- [ ] Test with hand-crafted minimal DXF (rectangle, triangle)
- [ ] Run: `bash scripts/build-host.sh` → ALL PASS
- [ ] Commit: `feat(core): DXF parser (LINE + LWPOLYLINE, 2D flat)`

---

### Task 3: Document Model + Undo/Redo (TDD)

**Files:**
- Create: `milcam/core/document/UndoStack.h` (Command pattern)
- Create: `milcam/core/document/GeoDocument.h/cpp`
- Create: `tests/core/test_document.cpp`

**Spec:**
- `GeoDocument` owns `std::vector<LineSegment>` + `UndoStack<>`
- Undo/redo commands: `ReplaceGeometry`, `AddSegment`, `Clear`
- Methods: `load(filename)`, `undo()`, `redo()`, `geometry()`, `isDirty()`

**Checklist:**
- [ ] Define `Command` interface: `virtual void execute() = 0; virtual void undo() = 0;`
- [ ] Implement `ReplaceGeometry` command (for open/load)
- [ ] Implement `GeoDocument`: owns segments + stack
- [ ] Test: load DXF → undo → redo → verify state
- [ ] Run: `bash scripts/build-host.sh` → ALL PASS
- [ ] Commit: `feat(core): GeoDocument + undo/redo command stack`

---

### Task 4: Canvas Rendering (Canvas update)

**Files:**
- Modify: `milcam/app/src/gui/CanvasView.cpp`
- Create: `milcam/app/src/gui/CanvasScene.*` (if not exists)

**Spec:**
- Render `std::vector<LineSegment>` as white/gray lines on black grid
- Grid: light gray lines every 10 mm, labels at 50 mm
- Zoom/pan: wheel = zoom 1.1×, Shift+drag = pan
- Cursor coords: display in status bar as user moves mouse

**Checklist:**
- [ ] `CanvasScene::drawBackground()` → grid lines (QBrush, QPen light gray)
- [ ] `CanvasScene::drawForeground()` → LineSegment[] as white lines (QPen white)
- [ ] Override `mouseMoveEvent()` → emit `coordsChanged(x, y)` → status bar slot
- [ ] Wheel event → `scale()`
- [ ] Test on device: `bash scripts/run-on-device.sh` → grid visible, lines render

---

### Task 5: File Open + DXF Loading (UI wiring)

**Files:**
- Modify: `milcam/app/src/gui/TopBar.cpp` (Open button)
- Create: `milcam/app/src/gui/OpenFileDialog.h/cpp`
- Modify: `MainWindow.cpp` → connect Open → dialog → load → canvas update

**Spec:**
- Open button (existing, currently disabled) → file picker (DXF filter)
- On success: `document->load(filename)` → canvas redraw
- On error: status bar error message

**Checklist:**
- [ ] `OpenFileDialog`: wraps `QFileDialog::getOpenFileName(..., "*.dxf")`
- [ ] `MainWindow::onFileOpen()` slot: dialog → if file, call `document->load(file)` → `canvas->update()`
- [ ] Connect: TopBar Open button → `MainWindow::onFileOpen()`
- [ ] Handle errors: show brief message in status bar
- [ ] Test on device: click Open → DXF dialog → select file → geometry renders

---

### Task 6: Coordinate Display + Status Integration

**Files:**
- Modify: `milcam/app/src/gui/StatusBar.cpp`
- Modify: `CanvasView.cpp` (emit coords)

**Spec:**
- Status bar left side: show "X: 123.45  Y: 67.89" (units: mm)
- Update on mouse move + click
- Optional: show segment count, bounds

**Checklist:**
- [ ] `StatusBar::setCoordinates(x, y)` slot
- [ ] `CanvasView::coordsChanged(double x, double y)` signal
- [ ] Connect canvas → status bar
- [ ] Test on device: move mouse → coords update in real-time

---

### Task 7: Keyboard Shortcuts + Polish

**Files:**
- Modify: `MainWindow.cpp`, `CanvasView.cpp`

**Spec:**
- Ctrl+O → Open file
- Ctrl+Z → Undo
- Ctrl+Y (or Ctrl+Shift+Z) → Redo
- Escape → deselect (future, but set up key handler)

**Checklist:**
- [ ] Add `QAction` objects in MainWindow for each shortcut
- [ ] Connect to slots
- [ ] Test: keyboard shortcuts work on device
- [ ] Commit: `feat(ui): keyboard shortcuts (open, undo, redo)`

---

### Task 8: Integration Test + Device Verification

**Files:**
- Create: `tests/integration/test_dxf_to_canvas.cpp` (optional, host only)
- Or: manual device test

**Spec:**
- Load sample DXF file on device → visually verify canvas
- Pan, zoom, open new file, undo → all responsive

**Checklist:**
- [ ] Build: `bash scripts/build-app-arm64.sh`
- [ ] Deploy: `bash scripts/run-on-device.sh`
- [ ] Manual test: Open → geometry renders, pan/zoom work, undo/redo work
- [ ] Verify coordinates display, no crashes
- [ ] Commit: `docs(p1): integration verification on device`

---

## Deliverables

**On Completion:**
- Operator can open a DXF file (rectangle, triangle, simple shape)
- Geometry renders as white lines on grid
- Pan/zoom responsive
- Undo/redo functional
- Status bar shows cursor coordinates
- Keyboard shortcuts work (Ctrl+O, Ctrl+Z/Y)
- Device test: no crashes, responsive UI

**Next Phase (P3):** Add Profile/Pocket/Drill CAM operations (toolpath generation).

---

## Tech Decisions

1. **DXF parsing:** Simple text-based regex parser (no external library) — keeps binary small, glibc compat simple.
2. **Undo/redo:** Command pattern (classic, tested) — scales to complex ops later.
3. **Canvas:** Qt's QGraphicsView (built-in pan/zoom, efficient rendering) — avoids custom painter loops.
4. **No 3D:** Raster 2D only (linuxfb has no GL).

---

## Estimated Effort (for reference)

- Task 1 (geometry): 1–2 hours
- Task 2 (DXF parser): 2–3 hours
- Task 3 (document/undo): 1–2 hours
- Task 4 (canvas render): 1–2 hours
- Task 5 (file dialog): 30 min
- Task 6 (coords): 30 min
- Task 7 (shortcuts): 30 min
- Task 8 (device test): 1 hour
- **Total:** ~10 hours (aggressive; real experience may vary)

---

## Related Documents

- `.ai/context.yaml` — project scope (P1 defined as DXF+canvas)
- `docs/superpowers/plans/2026-06-10-milcam-ui-v2.md` — UI shell (Phase 0.3, done)
- `docs/superpowers/specs/` — detailed specs (TBD, can be written per task)
