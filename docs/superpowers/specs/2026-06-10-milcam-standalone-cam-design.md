# MilCAM — Standalone Qt 2.5D CAM · Design Spec

Date: 2026-06-10
Status: Approved direction (brainstorming complete)
Target: SMB Technics VEC-VE 12" panel PC (NXP i.MX8M, aarch64, glibc 2.29)

## 1. Context & Why

MilCAM was originally planned as a *slim FreeCAD distribution + Python overlay*.
Live testing on the device (see `.ai/DISPLAY_ARCHITECTURE_2026-06-09.md`) proved
this is **not viable**: the panel has **no OpenGL/GLX/EGL**, so FreeCAD's
OCCT/Coin3D 3D viewport cannot render (`QXcbIntegration: Cannot create platform
OpenGL context`). FreeCAD launches (~287 MB RSS) but is unusable for CAM there.

Conversely the stock device already ships a rich **Qt 5.12.4** (Widgets, Quick,
Svg, VirtualKeyboard, SerialPort), **Python 3.7**, and renders via **linuxfb**
directly to `/dev/fb0`. CodeSYS runtime owns the framebuffer when its HMI runs;
otherwise the screen is free (now shows the SMB boot splash).

**Decision:** Build MilCAM as a **standalone Qt Widgets 2.5D CAM**, rendered with
the CPU raster engine (no GL), producing **CODESYS DIN 66025** G-code. This fits
the hardware, the "lightweight" goal, and the operator workflow (panel is usually
the CodeSYS TargetVisu HMI; MilCAM is a secondary mode).

## 2. Goals / Non-Goals

**Goals (v1):**
- Touch-first 2.5D CAM on the panel (linuxfb, no GL, no X).
- Geometry input: **DXF import + built-in primitives** (rectangle/circle/line + manual coordinates), units mm.
- Operations: **Profile (contour), Pocket (area clearing), Drilling**.
- Correct **CODESYS DIN 66025** post (the controller-specific value).
- Output: write `.cnc` to a **drop folder / USB**.
- Cross-compiled aarch64 binary that runs on the device's glibc 2.29.

**Non-Goals (v1, deferred):**
- 3D solid import (STEP/IGES) or 3D surfacing — needs OCCT + GL.
- SVG/curve import, engrave/V-carve (later).
- Live OPC UA notify (P-later; code path reserved, see §7).
- Building the CodeSYS PLC receiver project (separate workstream).
- On-screen 3D preview (2D top view only).

## 3. Architecture

Two layers with a hard boundary: a **GL/Qt-free core** (unit-testable on host)
and a **Qt Widgets GUI**. The existing pure-C++ `CodesysBridge` (drop folder +
future OPC UA) is reused. The FreeCAD overlay (`milcam/Mod/MilCAM/`) is retired
from the primary build (kept in repo/history as an optional headless-FreeCAD path).

### Modules

**`core/` — no Qt, no GL:**
- `geometry/` — `Point`, `Segment`, `Arc`, `Contour` (polyline+arcs), `Toolpath`
  (ordered moves: rapid / feed-linear / arc, each with Z).
- `import/dxf/` — **libdxfrw** wrapper → `Contour` list (LINE, ARC, CIRCLE,
  LWPOLYLINE, POLYLINE).
- `model/` — `Document`: `Stock` (size, origin, units=mm), ordered `Operation`
  list, `ToolLibrary` (diameter, type, feeds/speeds).
- `cam/` — toolpath generators, each `Operation` → `Toolpath`:
  - `ProfileOp` — offset contour by tool radius (inside / outside / on-line),
    lead-in/out, multi-pass step-down.
  - `PocketOp` — concentric Clipper2 insets (and/or zig-zag), multi-pass step-down.
  - `DrillOp` — points → drill/peck cycle **expanded to G0/G1/G4** (no canned cycle).
- `post/codesys/` — **DIN 66025 post** (`Toolpath` → `.cnc` text). Spec source:
  `CODESYS_CNC_CAM_Arastirma_Raporu.txt`. Rules: mandatory `%` header; never emit
  G20/G70/G71/G98/G99 or canned cycles; **F = path-units/second** + `E`/`E-`
  accel/decel words; modal codes + `N` line numbers; **relative I/J arcs**;
  `SubPrg{n}` for subprograms; M-functions are application-defined; auto-inject
  `G75` before any run of >~64 M/G4 codes.
- `thirdparty/clipper2/` — vendored Clipper2 (polygon offset/clip).

**`gui/` — Qt Widgets (raster, no GL):**
- `MainWindow` — **classic 3-pane** layout: top bar (`[SMB] open/save/HMI`), left
  operation toolbar, center `CanvasView`, right `OperationPanel` dock, bottom
  `JobBar`. Touch stylesheet (≥48 px targets), Qt VirtualKeyboard on numeric focus.
- `CanvasView` (`QGraphicsView`/`QGraphicsScene`) — 2D top view: stock, contours,
  toolpaths; touch pan/zoom.
- `OperationPanel` — parameter editor for the selected operation.
- `JobBar` — operation list + **"SEND G-CODE"** button.
- `controllers/` — model↔view glue; recompute `Toolpath` on parameter change;
  invoke post; hand `.cnc` to `io`.

**`io/`:**
- Drop-folder writer (atomic write to configured path / USB mount).
- (Reserved) OPC UA notify — see §7.

### Data flow
```
DXF / built-in shape → Contour → (operation params) → cam generator → Toolpath
   → CanvasView preview → post/codesys → .cnc text → io drop-folder / USB
   → [running CodeSYS reads it via SMC_ReadNCFile2 — PLC side out of v1 scope]
```

## 4. Toolchain & runtime

- Cross-compile aarch64 against a sysroot whose **glibc ≤ 2.29** (device libc) and
  **Qt 5.12.4** match the device, else dynamic binaries won't run (the dropbear
  glibc-mismatch lesson).
- **The on-hand `smb_plc_*defconfig` / `u-Boot-smb` are for a DIFFERENT product**
  (AM335x, 32-bit armhf, Linaro GCC 7.5) — **not** this i.MX8M aarch64 panel. Do
  not use them as-is.
- **Primary path:** reuse / adapt the **SMB-Q6R** (`/home/embed/Dev/QT6/TeachPendant/SMB-Q6R`)
  cross-build — it already targets **Qt5 5.12 aarch64** for this device family.
  If its toolchain+sysroot is reusable, that is the fastest route.
- **Fallback:** build a Buildroot (~2019.08-era) aarch64 SDK with glibc 2.29 + Qt
  5.12 (+ a sysroot from the device's own libs), or a crosstool-NG aarch64
  glibc-2.29 toolchain + device-pulled Qt libs + Qt 5.12.4 headers from source.
- Run on device: `QT_QPA_PLATFORM=linuxfb`, tslib touch (`/dev/input/event1`),
  VirtualKeyboard.
- **C++ standard:** C++17 floor (guaranteed by any candidate GCC; matches
  `CodesysBridge`). Opt into C++20 only if the chosen GCC ≥ 10 supports it.
  (C++23 / "c++2b" not targeted on embedded.)

## 5. UI rationale (Widgets, not QML)
QtQuick (QML) renders its scene graph via OpenGL by default; on this GL-less
device it would need the limited/fragile `software` backend. **Qt Widgets uses
the CPU raster paint engine — zero GL dependency**, matching the hardware
constraint. `QGraphicsView` is purpose-built for the 2D canvas. So: robustness
first, simplicity second.

## 6. Testing
- `core/` is Qt-free → **host unit tests**: toolpath generators against golden
  geometry; **post against golden `.cnc` samples** (the report's Phase-0 set:
  line, arc I/J & R, feed+E, G92 jump, 64-M+G75, SubPrg, tool comp, coord shift,
  M-ack flow).
- GUI smoke: host (xcb) + device (linuxfb + `/dev/fb0` capture, as already done).
- Final post validation: CODESYS CNC editor preview + **virtual axes**.

## 7. OPC UA reuse (deferred phase)
`SMB-Q6R/src/plc_link.{h,cpp}` is a ready **open62541** (portable C lib, static)
client wrapped in a threaded `QObject` (queued connections, Node-ID map), built
against **Qt5 5.12** — directly compatible with MilCAM. When we add live notify,
lift `plc_link` into `io/opcua/` and wire `MilCAM.JobReadyPath` / job-ready
signaling. No new OPC UA stack needed.

## 8. Build sequence (phases → implementation plan)
- **P0 — Toolchain + skeleton.** Stand up the aarch64 Qt5.12 cross-build (mine
  SMB-Q6R first). "Hello MilCAM" Widgets window on device linuxfb, verified via
  `/dev/fb0` capture. Repo: standalone app skeleton; drop FreeCAD overlay from build.
- **P1 — Core + canvas.** geometry + `Document` + DXF import + `CanvasView`
  rendering (show a real DXF on the panel).
- **P2 — CODESYS post + golden tests.** Highest-value, report-driven; fully
  host-tested. (Can run in parallel with P1 — pure core.)
- **P3 — Profile end-to-end.** `ProfileOp` → `Toolpath` → post → `.cnc`; preview
  + drop-folder write.
- **P4 — Pocket + Drill.** `PocketOp` (Clipper2), `DrillOp`.
- **P5 — Touch UX + delivery.** VirtualKeyboard, ≥48 px controls, USB/drop-folder
  config, HMI switch (return to CodeSYS TargetVisu).

## 9. Risks
- **Toolchain/glibc match** — biggest risk; SMB-Q6R reuse mitigates. If no
  matching aarch64 Qt5.12 sysroot is obtainable, P0 expands (build Buildroot SDK).
- **linuxfb + touch calibration** — tslib already on device; validate early.
- **CodeSYS framebuffer coexistence** — MilCAM and CodeSYS HMI share `/dev/fb0`
  (one at a time; mode switch). No side-by-side. Splash bridges idle.
- **DIN 66025 correctness** — mitigated by golden samples + virtual-axes validation.
- **DXF variety** — restrict v1 to common entities; log unsupported.
