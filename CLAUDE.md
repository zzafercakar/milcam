# MilCAM — Claude Code Project Instructions

## First Steps (Every Session)

1. Read `.ai/START_HERE.md`
2. Read `.ai/context.yaml` — full project state
3. Read `.ai/WORKPLAN.md` — current phase and next actions
4. Read `.ai/ARCHITECTURE.md` — the overlay model and why we don't build a custom UI
5. Check `.ai/ENGINEERING_LOG.md` for known issues
6. Read `.ai/VEC_VE_TARGET.md` for the deployment target

## Project Overview — Read This First

MilCAM is **NOT a new CAD/CAM application.** MilCAM is a **slim FreeCAD distribution**:

- The UI is FreeCAD's own UI, **unmodified**.
- Non-CAM workbenches (FEM, BIM, TechDraw, …) are **disabled at build time** via
  FreeCAD's own `BUILD_<X>=OFF` CMake flags.
- On top of the slim FreeCAD build we install **one Python workbench overlay**
  (`Mod/MilCAM/`) that:
    - Hides leftover non-CAM workbenches from the picker.
    - Makes MilCAM the default workbench.
    - Adds two commands: **Send to CodeSys** (post + drop folder + OPC UA
      notify) and **Switch to HMI** (`wmctrl` raise TargetVisu).
    - Registers a CODESYS-flavored G-code postprocessor.
- A **Python C extension** (`milcam/CodesysBridge/`) provides the drop folder
  + OPC UA primitives, imported as `import CodesysBridge` from Python.

**Result:** the operator launches FreeCAD, sees only the CAM workbench, can
post G-code, and hand it to the running CodeSys controller — all with
FreeCAD's own widgets.

## What MilCAM Owns vs What FreeCAD Owns

| Thing                       | Owner                                                 |
|-----------------------------|-------------------------------------------------------|
| Main window, menus, tree    | FreeCAD upstream                                      |
| 3D viewport (Coin3D / OCCT) | FreeCAD upstream                                      |
| Part, Sketcher, PartDesign  | FreeCAD upstream                                      |
| CAM (Path) workbench        | FreeCAD upstream                                      |
| Workbench visibility config | MilCAM overlay (`Mod/MilCAM/InitGui.py`)              |
| Send-to-CodeSys command     | MilCAM overlay (`Mod/MilCAM/Commands/`)               |
| CODESYS postprocessor       | MilCAM overlay (`Mod/MilCAM/PostProcessor/codesys_post.py`) |
| OPC UA + drop folder        | MilCAM Python C extension (`milcam/CodesysBridge/`)   |
| CMake module-disable knobs  | MilCAM top-level CMakeLists                           |
| Branding (splash, name)     | MilCAM (Faz 3 — not yet implemented)                  |

## Sibling Projects

- **CADNC** (`/home/embed/Dev/CADNC/`) — full custom CAD+CAM with QML UI.
  MilCAM is **not** a copy of CADNC. The previous MilCAM scaffold that mirrored
  CADNC's architecture has been deleted — only the VEC-VE docs and project
  memory remain.
- **MilCAD** (`/home/embed/Dev/MilCAD/`) — legacy project, reference only.

## FreeCAD Source — Not Vendored

FreeCAD source is **NOT** bundled in this repo. It's referenced through the
CMake cache variable `FREECAD_SOURCE_DIR`. Default value points to the
maintainer's local clone at `/home/embed/Downloads/FreeCAD-main-1-1-git`.
Why: FreeCAD source is 3 GB; vendoring it would make the MilCAM repo
unworkable on git. Trade-off: fresh checkouts need the user to clone FreeCAD
separately. Documented in `.ai/FREECAD_SUBSET.md`.

## Critical Rules

1. **DO NOT build a custom Qt/QML UI.** FreeCAD's UI is the UI. Period.
2. **DO NOT patch FreeCAD source.** Use CMake flags, runtime overlays, and
   `User parameter:BaseApp/...` preferences instead.
3. **Module disabling lives in MilCAM's CMakeLists.** Not in FreeCAD.
4. **`Mod/MilCAM/` is pure Python.** Do not introduce C++ into the workbench
   overlay itself — the workbench is the integration layer.
5. **`CodesysBridge` is plain C++** (no FreeCAD/Qt dep) exposed via the
   CPython stable ABI. This makes it portable and ABI-stable across FreeCAD
   point releases.
6. **Linux: GLX only, never EGL** (OCCT compiled with GLX).
7. **Touch-first UX:** any new MilCAM UI affordance must be ≥48px tap target.
   FreeCAD's defaults are NOT touch-friendly out of the box; MilCAM overlay
   adjusts font size + enables Qt VirtualKeyboard input method.

## Build

```bash
cmake --preset default            # configures with /home/embed/Downloads/FreeCAD-main-1-1-git
cmake --build build -j$(nproc)
DESTDIR=$PWD/_inst cmake --install build
DISPLAY=:0 QT_QPA_PLATFORM=xcb _inst/usr/local/bin/FreeCAD
```

Overlay-only (for rapid Python iteration, requires system FreeCAD):

```bash
cmake --preset overlay-only
cmake --build build-overlay -j$(nproc)
sudo cmake --install build-overlay
```

## Coding Standards

- Languages: C++17 (CodesysBridge), Python 3 (workbench overlay).
- Comments: English. User communication: Turkish.
- Python style: PEP 8, 4-space indent, type hints encouraged.
- C++ namespace: `MilCAM`.
- Every important operation: an inline comment explaining **why**, not what.

## Reference Sources

- FreeCAD: `/home/embed/Downloads/FreeCAD-main-1-1-git/`
- CADNC: `/home/embed/Dev/CADNC/`  (sibling, NOT a code source for MilCAM anymore)
- VEC-VE docs: `/home/embed/Dev/MilCAM/doc/VEC-VE-*.pdf`
