# MilCAM — Claude Code Project Instructions

## First Steps (Every Session)

1. Read `.ai/START_HERE.md`
2. Read `.ai/context.yaml` — full project state
3. Read `.ai/WORKPLAN.md` — current phase and next actions
4. Check `.ai/ENGINEERING_LOG.md` for known issues
5. Read `.ai/VEC_VE_TARGET.md` — the deployment target shapes every UI/footprint decision

## Project Overview

MilCAM is a **CAM-only** desktop application targeting CodeSys CNC panel PCs
(VEC-VE 12" class). It imports STEP/IGES/BREP/DXF/STL geometry, lets the
operator set up Profile/Pocket/Drill/Facing/Adaptive/Engrave operations on a
touch-first UI, generates G-code with the CODESYS post-processor, and hands
the file to the running PLC via OPC UA + a drop folder.

MilCAM is **NOT a CAD modeller**. No Sketcher, no PartDesign, no parametric
feature tree. Geometry is read-only. See `.ai/FREECAD_SUBSET.md` for what we
keep from FreeCAD and what we deliberately drop.

## Sibling Projects

- **CADNC** (`/home/embed/Dev/CADNC/`) — full CAD+CAM workstation; MilCAM is a
  carve-out of its CAM module repackaged for the panel PC.
- **MilCAD** (`/home/embed/Dev/MilCAD/`) — previous-generation CAD project; CAM
  module was originally developed there before migrating to CADNC.

## Architecture

```
UI Shell (QML + Qt VirtualKeyboard)
        ↓
Adapter Layer (C++17/20)
        ↓
FreeCAD slim subset: Base + App + Material + Part   (LGPL)
        ↓
OCCT Kernel  (geometry import: STEP/IGES/BREP)

Side modules:
  cam/             — toolpath, G-code, CODESYS post (proprietary)
  adapter/CodesysBridge — OPC UA + G-code drop folder
```

## Critical Rules

- **UI code never includes FreeCAD headers** — only via adapter/
- **AIS_InteractiveContext on render thread only** — never UI thread
- **Linux: use GLX, never EGL** — OCCT is GLX-compiled
- **Never re-introduce Sketcher / PartDesign / Nesting** — out of scope by design
- **Touch-first UI** — every interactive control must be ≥48px (we target 56px)
- **No physical keyboard assumption** — Qt VirtualKeyboard handles text input
- **Python dependency accepted** — required by FreeCAD
- **Real-time aware** — the same box runs CodeSys with hard-RT requirements;
  see `.ai/VEC_VE_TARGET.md` "Jitter discipline"

## Build

```bash
cmake --preset default
cmake --build build -j$(nproc)
DISPLAY=:0 QT_QPA_PLATFORM=xcb ./build/milcam
```

## Coding Standards

- Language: C++20
- Code/comments: English
- User communication: Turkish
- Namespace: `MilCAM`
- Classes: PascalCase, variables: camelCase, members: `trailing_underscore_`
- Module layout: `inc/` (headers), `src/` (sources)
- Every important operation: inline comment explaining **why** (not what)

## Reference Sources

- FreeCAD: `/home/embed/Downloads/FreeCAD-main-1-1-git/`
- CADNC: `/home/embed/Dev/CADNC/`  (CAM code's authoritative upstream)
- MilCAD: `/home/embed/Dev/MilCAD/`
- VEC-VE manuals: `/home/embed/Dev/CADNC/doc/VEC-VE-*.pdf`
