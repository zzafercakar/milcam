# MilCAM

**Touch-first CAM tool for CodeSys CNC panel PCs.**

MilCAM imports neutral geometry (STEP / IGES / BREP / DXF / STL), drives a
small but capable set of CAM operations (Profile, Pocket, Drill, Facing,
Adaptive, Engrave, Helix, Slot), generates G-code through a CODESYS-aware
post-processor, and hands the result to a running CodeSys CNC controller via
OPC UA + a watched drop folder. The UI is built around a 12" touchscreen with
no physical keyboard — text input uses Qt Virtual Keyboard.

## Why MilCAM

Field operators on VEC-VE class panel PCs need to:
- Open a CAD file produced on a workstation
- Set tools, feeds, depths, stepovers
- Generate G-code that the on-board CodeSys CNC can execute
- Do this with their finger, in a noisy environment, without a keyboard

A full CAD application (FreeCAD, CADNC) is too heavy and exposes too many
controls. MilCAM keeps only the CAM workflow.

## Status — `0.1.0-scaffold`

| Layer        | State                                                                 |
| ------------ | --------------------------------------------------------------------- |
| Project tree | Scaffolded                                                            |
| FreeCAD      | Slim subset copied (Base, App, Material, Part — no Sketcher/PartDesign) |
| CAM module   | Inherited from CADNC (8 operations + CODESYS post)                    |
| Adapter      | `CamFacade`, `PartFacade` carried over; `ImportFacade` + `CodesysBridge` stubs |
| Viewport     | OCCT scaffold from CADNC; not yet wired to MilCAM's CAM-job view      |
| UI           | Touch-first QML shell with Virtual Keyboard                           |
| Build        | CMake configured but **not yet validated end-to-end** — see WORKPLAN  |

See [.ai/WORKPLAN.md](.ai/WORKPLAN.md) for the phased roadmap.

## Build

```bash
cmake --preset default
cmake --build build -j$(nproc)
DISPLAY=:0 QT_QPA_PLATFORM=xcb ./build/milcam
```

Cross-compile to the panel PC:

```bash
# Edit scripts/toolchain-aarch64.cmake to point at your sysroot first.
cmake --preset panel-pc
cmake --build build-aarch64 -j$(nproc)
scp build-aarch64/milcam root@<panel-pc>:/root/MilCAM/
```

## Layout

```
MilCAM/
├── adapter/        Facade between QML and FreeCAD/CAM
├── app/            main.cpp + AppVersion.h.in
├── cam/            Toolpath, post-processor (from CADNC)
├── doc/            User-facing docs (light)
├── freecad/        Slim FreeCAD subset (Base, App, Material, Part)
├── resources/      Icons, logos (from CADNC)
├── scripts/        Cross-compile toolchain, deploy helpers
├── tests/          Unit/smoke tests
├── ui/qml/         Touch-first QML UI
├── util/           Shared helpers (from CADNC)
├── viewport/       OCCT viewport
└── .ai/            Project memory for Claude Code
```

## License

Adapter, app, UI, CAM, scripts: **Proprietary**.
FreeCAD subset under `freecad/`: **LGPL-2.1-or-later** (see `freecad/LICENSE`).
