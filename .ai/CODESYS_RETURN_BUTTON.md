# CodeSYS ↔ MilCAM screen switching (launch/exit model)

> PLC-side recipe for the operator switch between the CodeSYS CNC HMI and MilCAM
> on the VEC-VE panel. This is CodeSYS-project work (the customer's project);
> documented here so it isn't lost. Background + why `chvt` doesn't work:
> [ENGINEERING_LOG.md](ENGINEERING_LOG.md) 2026-06-10 (gece).

## Why not `chvt` / VT switching
Single `/dev/fb0` shared by all VTs; both CodeSYS and MilCAM are Qt **linuxfb**
apps writing fb0 directly. The visible app = **whoever wrote fb0 last**, not the
active VT. `chvt 1` doesn't change the display; `chvt 2` hangs (CodeSYS holds
tty1 and won't release). And CodeSYS does **not** auto-repaint when MilCAM goes
away. So switching = control who paints fb0 + force CodeSYS to repaint on return.

## Model: CodeSYS launches MilCAM; MilCAM exits to return

### 1. CodeSYS "MilCAM" button → launch MilCAM (NOT chvt)
The on-device launcher `/root/milcam/run.sh` (deployed from
`scripts/device/run-milcam.sh`) sets the Qt env and execs the binary.

GVL:
```iecst
VAR_GLOBAL
    xToMilcam     : BOOL;                                  // Visu button toggles this
    xMilcamActive : BOOL;
    sCmd          : STRING(120) := 'setsid /root/milcam/run.sh >/tmp/milcam.log 2>&1 &';
    sPidCmd       : STRING(40)  := 'pidof milcam';
    sOut          : STRING(255);
    result        : RTS_IEC_RESULT;
    diRet         : RTS_IEC_RESULT;
END_VAR
```
Slow/visu task:
```iecst
IF xToMilcam THEN
    xToMilcam := FALSE;
    diRet := SysProcessExecuteCommand2(pszCommand := ADR(sCmd),  pszStdOut := ADR(sOut),
                                       udiStdOutLen := SIZEOF(sOut), pResult := ADR(result));
    xMilcamActive := TRUE;
END_IF
```
**Critical:** the trailing `&` in `sCmd` makes the command return immediately;
without it `SysProcessExecuteCommand2` blocks the PLC task until the GUI closes.

### 2. MilCAM "CNC" button → exits the app
Handled in MilCAM (`HmiSwitch::returnToCnc` → `QApplication::quit()`). No PLC work.
MilCAM closes; its last frame stays frozen on fb0 until CodeSYS repaints.

### 3. CodeSYS detects exit → forces a full repaint to reclaim the screen
Poll MilCAM liveness only while active; on exit, force a full TargetVisu repaint.
```iecst
IF xMilcamActive THEN
    diRet := SysProcessExecuteCommand2(pszCommand := ADR(sPidCmd), pszStdOut := ADR(sOut),
                                       udiStdOutLen := SIZEOF(sOut), pResult := ADR(result));
    IF sOut = '' THEN                 // pidof returned nothing -> MilCAM exited
        xMilcamActive := FALSE;
        // --- force full-screen repaint (pick ONE method) ---
        // Method A: bounce the displayed visu (most reliable full redraw)
        //   CurrentVisu := 'Blank'; (next cycle) CurrentVisu := 'MainVisu';
        // Method B: full-screen "curtain" Rectangle whose Invisible := NOT xReclaim;
        //   pulse xReclaim TRUE one cycle, then FALSE.
    END_IF
END_IF
```
Throttle the `pidof` poll (e.g. run it once every ~500 ms, not every cycle) since
each call spawns a process. `[SysProcess] Command=AllowAll` (or `Command.0=...`)
must be enabled in `/root/CODESYSControl.cfg` + runtime restart for any of the
above to run.

## Full-screen repaint methods (detail)
- **A — visu bounce:** TargetVisu fully clears+redraws on a visualization change.
  Switch `CurrentVisu` (or the frame-switch INT used by the Tab pages
  Dialog/Diagnose/Execution/Programming) to another page and back. Cleanest.
- **B — curtain rectangle:** add a 1024×768 Rectangle in the HMI bg color, state
  variable `Invisible := NOT xReclaim`. Toggling it visible paints over MilCAM's
  leftover pixels; the underlying elements redraw on the next cycle.

## Notes
- MilCAM needs no VT (`run.sh` launches it on the default VT; it just paints fb0).
- Touch: MilCAM uses evdevtouch on `/dev/input/event1` (auto-detected). While
  MilCAM is up it owns the screen; CodeSYS touch is irrelevant until MilCAM exits.
- Exit (vs keeping MilCAM resident) also frees ~RAM; cold start is a few seconds.
