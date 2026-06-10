# MilCAM — UI v2 (professional CAM shell) + HMI switch · Design Spec

Date: 2026-06-10
Status: Approved direction (brainstorming complete)
Target: SMB Technics VEC-VE 12" panel PC (NXP i.MX8M, aarch64, glibc 2.29, Qt 5.12.4, linuxfb)
Builds on: `docs/superpowers/specs/2026-06-10-milcam-standalone-cam-design.md` (§3 GUI),
Phase 0.3 "hello" window (now on device).

## 1. Context & Why

Phase 0.3 put a minimal Qt Widgets window on the panel's linuxfb (verified via
`/dev/fb0` capture). Two gaps surfaced in live use:

1. **The shell looks like a placeholder**, not a professional CAM product. The
   user wants a layout modeled on FreeCAD CAM / SolidCAM operator screens.
2. **The HMI/Open buttons did nothing useful**, and tapping "switched" to the
   CodeSYS CNC HMI with no way back. Root cause: MilCAM and the CodeSYS
   TargetVisu both draw to `/dev/fb0` on the **same VT (tty1)** with no window
   manager, so they fight over the framebuffer and touch device.

Device probe (2026-06-10) confirmed the clean fix: `chvt` / `openvt` /
`deallocvt` exist; CodeSYS runtime (PID, CNC HMI) binds `tty=/dev/tty1`. Running
MilCAM on its **own VT (tty2)** and switching with `chvt` multiplexes the single
framebuffer without a WM.

This spec covers (a) the modern CAM **shell** (layout, theme, icons) and (b) the
**HMI⇄MilCAM switch** via VT switching. Real CAM functionality (DXF import,
canvas geometry, toolpath ops) remains deferred to P1/P3 — the canvas and panels
here are styled, functional-looking **placeholders**.

## 2. Goals / Non-Goals

**Goals:**
- A professional, modern, touch-first CAM operator shell (light theme, blue
  accent), classic 3-pane CAM layout, consistent SVG icon set, ≥48 px targets.
- A **fixed top-right switch cluster** with an `⇄ HMI` icon button that switches
  to the CodeSYS HMI (`chvt 1`).
- MilCAM runs on **tty2**; documented CodeSYS-side return path (`chvt 2`).
- Host (xcb) smoke + device (linuxfb, fb0 capture) verification.

**Non-Goals (still deferred):**
- DXF import, real 2D canvas geometry, toolpath generation (P1/P3/P4).
- Two-way switch wholly controlled by MilCAM (return is a CodeSYS-side button).
- Boot autostart / kiosk service wiring (P5).
- Replacing the direct-g++ cross build with a CMake-sysroot build (later).

## 3. Layout (1024×768)

```
┌──────────────────────────────────────────────────────────────┐
│ ◆ MilCAM    [+New][⌑Open][⛁Save][▶Post]        ┊ [ ⇄ HMI ]    │  TopBar (fixed switch at right)
├────┬────────────────────────────────────────────┬────────────┤
│ ⌗  │                                            │ JOB TREE    │
│ ▭  │              CanvasView                     │ Stock …     │
│ ◯  │     (QGraphicsView: grid, stock, paths)     │ ▸ Profile 1 │
│ ∿  │                                            ├────────────┤
│ ⟂  │                                            │ PARAMETERS  │
│ ◴  │                                            │ Tool / Depth│
├────┴────────────────────────────────────────────┴────────────┤
│ mm · X:0.000 Y:0.000 · Post: CODESYS ✓ · [ SEND G-CODE ] ✦    │  StatusBar
└──────────────────────────────────────────────────────────────┘
```

**Regions / units (each one responsibility, well-bounded):**
- **TopBar** (`gui/TopBar`): brand label (left), primary actions New/Open/Save/Post
  (center, icon+label), and a **fixed right-aligned switch cluster** (`gui/HmiSwitch`)
  separated by a vertical divider — an `⇄ HMI` icon button. Stays pinned regardless
  of window resize (right-anchored layout with a stretch spacer).
- **OperationToolBar** (`gui/OperationToolBar`): left vertical icon strip —
  Import DXF, Rectangle, Circle, Line, Profile, Pocket, Drill. ≥56 px icons.
  Buttons are visible but non-functional placeholders until P1/P3.
- **CanvasView** (`gui/CanvasView`, `QGraphicsView`/`QGraphicsScene`): center; draws
  a grid + an empty-state hint now ("Import a DXF or add a shape"). Real geometry P1.
- **JobPanel** (`gui/JobPanel`, right dock): top = job tree (Stock + operations,
  sample/disabled rows now), bottom = parameter form for the selected op (static now).
- **StatusBar** (`gui/StatusBar`): units, live cursor coords (from CanvasView mouse),
  post status, and a prominent **SEND G-CODE** accent button (disabled until a job exists).

`MainWindow` assembles these; each sub-widget is independently constructible and
styled, so P1/P3 can fill behavior without touching siblings.

## 4. Theme & icons

- **Light professional theme:** near-white/light-grey surfaces, corporate blue
  `#0F3460` accent (selection, primary buttons, active states), subtle 1 px
  dividers and soft shadows. High daylight readability. Centralized in a single
  Qt stylesheet string/resource (`gui/style.qss`), applied at app root.
- **Typography:** pick a font that actually exists on the device (DejaVu Sans is
  present — Phase 0.3 showed "Helvetica" requests fell back to a box engine). Set
  the application font explicitly to **DejaVu Sans** to guarantee crisp text on
  linuxfb. Fixes the Phase-0.3 stray-glyph/box artifact.
- **Icons:** a consistent flat **SVG set** under `milcam/app/resources/icons/`,
  compiled into the binary via a Qt resource file (`milcam/app/resources/milcam.qrc`)
  so no icon files need deploying. `QIcon` from `:/icons/...`. Touch sizes ≥48 px.

## 5. HMI ⇄ MilCAM switch (VT multiplexing)

- **Run MilCAM on tty2:** launch with `QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0:tty=/dev/tty2`.
  CodeSYS keeps tty1. The deploy/launch helper and `scripts/build-app-arm64.sh`
  run notes are updated; a small `scripts/run-on-device.sh` documents the exact
  env + VT.
- **MilCAM → CodeSYS:** the `⇄ HMI` button runs `chvt 1` (via `QProcess` calling
  `/usr/bin/chvt 1`). The kernel activates VT1; CodeSYS's Qt linuxfb redraws.
- **CodeSYS → MilCAM (PLC-side, documented, not MilCAM code):** add a "MilCAM"
  button to the CodeSYS Visu whose action runs `/usr/bin/chvt 2` through the
  **SysProcess** library:
  ```iecst
  IF xToMilcam THEN
      xToMilcam := FALSE;
      SysProcessExecuteCommand2(pszCommand := ADR('/usr/bin/chvt 2'),
                                pResult    := ADR(result));
  END_IF
  ```
  Prerequisite: `/root/CODESYSControl.cfg` must permit it, then restart the runtime:
  ```ini
  [SysProcess]
  Command=AllowAll        ; or: Command.0=/usr/bin/chvt
  ```
  This PLC-side step is the customer's CodeSYS project change; captured here and in
  `.ai/` so it isn't lost. MilCAM cannot force the return itself by design choice.
- **Touch note:** Qt linuxfb suspends rendering on inactive VTs; validate that the
  inactive app does not consume `/dev/input/event1` events meant for the active one.
  If both grab the touch device, switch MilCAM's evdevtouch to release on VT
  deactivate (Qt handles this for the active VT) — verify during device test.

## 6. Build & run

- Same toolchain as Phase 0.3: Bootlin **glibc-2.27** aarch64 (gcc 7.3), desktop
  Qt 5.12.4 headers (aqt), link device `.so.5`, `-static-libstdc++`. The new
  `.qrc` adds an `rcc` step (use the **desktop Qt's rcc**, host tool, like moc).
  `scripts/build-app-arm64.sh` gains the `rcc` invocation.
- CMake `milcam` target (`MILCAM_BUILD_APP=ON`) keeps AUTOMOC + AUTORCC for the
  eventual host/CMake build; the device spike build stays the direct script.
- Host smoke: build with system Qt5 (xcb) to sanity-check layout; device truth =
  linuxfb + `/dev/fb0` capture → PNG.

## 7. Testing / acceptance

- **Host:** app builds (xcb), window shows the 3-pane shell, switch cluster pinned
  top-right, icons render, theme applied. (Visual smoke.)
- **Device:** cross-build, deploy, run on **tty2**; fb0 capture shows the modern
  shell. Tap `⇄ HMI` → screen switches to the CodeSYS CNC HMI (`chvt 1`). After
  adding the CodeSYS-side `chvt 2` button, tap it → returns to MilCAM. (The return
  half depends on the PLC project; MilCAM half is verified by confirming `chvt 1`
  switches away and a manual `chvt 2` over ssh brings MilCAM back.)
- **Acceptance:** professional shell renders on the panel; HMI button switches to
  CodeSYS; documented `chvt 2` path returns to MilCAM.

## 8. Risks
- **VT switch + touch arbitration** — main unknown; validate the inactive app
  releases the touch device. Mitigation: rely on Qt linuxfb VT-active gating; test
  early on device.
- **Font availability** — fixed by pinning DejaVu Sans (present on device).
- **SysProcess disabled** — return path silently fails until `[SysProcess]` is
  allowed + runtime restarted; documented as an explicit prerequisite.
- **fb0 contention during switch** — brief flicker acceptable; if Qt doesn't
  re-acquire fb cleanly on VT activate, fall back to `openvt`-managed VTs.
