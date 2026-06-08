# MilCAM

**A slim FreeCAD distribution for CodeSys CNC panel PCs.**

MilCAM is not a new application. It is **FreeCAD** — same UI, same CAM
workbench, same 3D viewport — built **without** the non-CAM modules (FEM,
BIM, TechDraw, Assembly, OpenSCAD, Surface, Robot, …) and **with** a thin
overlay that:

- Makes CAM the only visible workbench.
- Adds a **Send to CodeSys** command that post-processes the active job
  and atomically drops the G-code into a folder watched by the running
  CodeSys CNC.
- Adds a **Switch to HMI** command that raises the CodeSys TargetVisu
  window.
- Ships a CODESYS-flavored G-code postprocessor.

Why slim FreeCAD instead of a custom app? Because rewriting CAD/CAM UI from
scratch is years of work that has nothing to do with the actual product
need: helping a touch-screen operator emit G-code to a CodeSys-driven
machine. FreeCAD already has every piece — we just bring focus.

## Status — `0.2.0-overlay`

| Layer                | State                                                                 |
|----------------------|-----------------------------------------------------------------------|
| Top-level CMake      | ✓ Wraps `add_subdirectory(${FREECAD_SOURCE_DIR})` with `BUILD_<X>=OFF` for ~20 non-CAM modules |
| CodesysBridge module | ✓ C++ core (drop folder, atomic write, `wmctrl` raise) + CPython binding. OPC UA stubbed. |
| Workbench overlay    | ✓ `Mod/MilCAM/`: InitGui, three commands (Send-to-CodeSys, Switch-to-HMI, About), CODESYS postprocessor, icons |
| Tests                | ✓ pytest smoke for postprocessor + bridge                              |
| Build validated      | ✗ End-to-end build NOT yet run — see WORKPLAN Faz 0.5 / Faz 1          |
| Branding             | ✗ Splash, app name, default workbench config — Faz 3                   |

## Build

```bash
# 1. Get a FreeCAD source checkout (one-time)
git clone https://github.com/FreeCAD/FreeCAD.git ~/Downloads/FreeCAD

# 2. Configure MilCAM (FreeCAD path goes through FREECAD_SOURCE_DIR)
cmake --preset default \
      -DFREECAD_SOURCE_DIR=$HOME/Downloads/FreeCAD

# 3. Build
cmake --build build -j$(nproc)

# 4. Install to a staging prefix
DESTDIR=$PWD/_inst cmake --install build

# 5. Run
DISPLAY=:0 QT_QPA_PLATFORM=xcb _inst/usr/local/bin/FreeCAD
```

When FreeCAD opens, the workbench picker shows only `MilCAM` and the CAM
support workbenches (Part, PartDesign, Sketcher, Draft, Mesh,
Spreadsheet). Click `MilCAM` to enter the CAM workflow.

### Overlay-only build (faster iteration)

If you only changed `milcam/Mod/MilCAM/*.py` or the postprocessor, you do
not need to rebuild FreeCAD. Use the overlay preset:

```bash
cmake --preset overlay-only
cmake --build build-overlay
sudo cmake --install build-overlay        # installs into /usr/share/freecad/Mod/MilCAM
```

### Cross-compile for the panel PC

```bash
cmake --preset panel-pc
cmake --build build-aarch64 -j$(nproc)
./scripts/deploy.sh root@192.168.1.123
```

The deploy script uses rsync and installs a `freecad-milcam.service`
systemd unit pre-configured with the VEC-VE environment variables (DISPLAY,
tslib, virtual keyboard).

## Layout

```
MilCAM/
├── milcam/
│   ├── CodesysBridge/      # C++ Python extension — drop folder + OPC UA
│   └── Mod/MilCAM/         # FreeCAD Python workbench overlay
│       ├── Init.py / InitGui.py
│       ├── Commands/       # Send-to-CodeSys, Switch-to-HMI, About
│       ├── PostProcessor/  # codesys_post.py — CODESYS G-code post
│       └── Resources/      # icons
├── doc/                    # VEC-VE manuals
├── scripts/                # deploy + cross-compile toolchain
├── tests/                  # pytest smoke
├── CMakeLists.txt
├── CMakePresets.json
└── .ai/                    # Claude project memory
```

## License

- `milcam/` (CodesysBridge + workbench overlay): **Proprietary**.
- CODESYS postprocessor (`milcam/Mod/MilCAM/PostProcessor/codesys_post.py`):
  derivative of FreeCAD postprocessors → **LGPL-2.1-or-later**.
- FreeCAD source built into the result: **LGPL-2.1-or-later** (upstream).
