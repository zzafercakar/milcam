# MilCAM — Device Deploy Procedure (VEC-VE panel)

> Reproducible runbook to deploy MilCAM (standalone Qt 2.5D CAM) to a SMB Technics
> VEC-VE panel (NXP i.MX8M, aarch64, glibc 2.29, Qt 5.12.4, linuxfb) and wire the
> CodeSYS HMI ⇄ MilCAM switch. Captures exactly what is placed on the device so
> the same can be done on other units. Background/decisions:
> [ENGINEERING_LOG.md](ENGINEERING_LOG.md), [TOOLCHAIN_NOTES.md](TOOLCHAIN_NOTES.md),
> [CODESYS_RETURN_BUTTON.md](CODESYS_RETURN_BUTTON.md).

## 0. What ends up ON THE DEVICE (the deployed artifacts)

| Path on device | What | Source in repo | Notes |
|---|---|---|---|
| `/root/milcam/milcam` | aarch64 MilCAM GUI binary (~260 KB) | built from `milcam/app` + `milcam/core` | dynamic: needs device Qt5.12 + glibc |
| `/root/milcam/run.sh` | launcher (sets Qt env, exec milcam) | `scripts/device/run-milcam.sh` | CodeSYS launches this |
| `/etc/init.d/S95splash` | boot splash init | `scripts/device/S95splash` | optional branding |
| `/etc/milcam-splash.raw` | 1024×768 BGRX splash image | gen by `scripts/make_splash.py` | optional branding |
| `/etc/init.d/_S40xorg` | Xorg DISABLED (renamed from S40xorg) | n/a (rename on device) | linuxfb path; X not needed |
| `/usr/sbin/dropbear` + `/etc/init.d/S60dropbear` | SSH access | `.ai/DROPBEAR_INSTALL_2026-06-09.md` | for deploy/debug |
| `/root/CODESYSControl.cfg` → `[SysProcess]` | allows CodeSYS to run `run.sh` | this doc §5 | **required** for the switch |
| `/root/milcam-test/` | core_tests + golden (host-test parity check) | built from `tests/` | optional; removable |
| `/root/CODESYSControl.cfg.bak-milcam` | cfg backup before our edit | — | restore point |

The big build inputs (Qt headers, device libs, cross toolchain) live on the BUILD
HOST under `~/milcam-sysroot/` (NOT in git); §2 reproduces them.

## 1. Network / access
- Device default IP `192.168.1.123` (eth1). SSH via dropbear: `ssh -i ~/.ssh/milcam_id root@192.168.1.123`.
- **No sftp on device** → transfer files by streaming over ssh: `ssh DEV 'cat > /dest' < localfile`.
- If a fresh unit has no SSH: install dropbear first (serial console + base64, see `.ai/DROPBEAR_INSTALL_2026-06-09.md`).

## 2. Build-host one-time setup (`~/milcam-sysroot/`)
1. **Cross toolchain (glibc ≤ 2.29):** Bootlin aarch64 glibc-2.27 (gcc 7.3):
   `curl -fsSL -o bootlin.tar.bz2 https://toolchains.bootlin.com/downloads/releases/toolchains/aarch64/tarballs/aarch64--glibc--stable-2018.11-1.tar.bz2 && tar xjf bootlin.tar.bz2`
   → `~/milcam-sysroot/aarch64--glibc--stable-2018.11-1/`. (Host gcc-13 pulls GLIBC_2.32+ → fails on 2.29.)
2. **Qt 5.12.4 headers** (arch-neutral) via aqt:
   `python3 -m venv aqtenv && ./aqtenv/bin/pip install aqtinstall && ./aqtenv/bin/aqt install-qt linux desktop 5.12.4 gcc_64 --archives qtbase --outputdir ~/milcam-sysroot/qt-desktop`
3. **Device Qt libs sysroot** (link against real device ABI): pull `/usr/lib` + `/lib` from the device:
   `ssh -i ~/.ssh/milcam_id root@192.168.1.123 'cd / && tar cf - usr/lib lib | gzip -1' > vecve-libs.tgz && mkdir -p ~/milcam-sysroot/vec-ve && tar xzf vecve-libs.tgz -C ~/milcam-sysroot/vec-ve`
4. Host cross-compiler: `crossbuild-essential-arm64` (only for the static core test; the app uses the Bootlin g++).

## 3. Build
- **Host unit tests (core, no device):** `bash scripts/build-host.sh` → `ALL PASS`.
- **App cross-build (aarch64):** `bash scripts/build-app-arm64.sh` → `build-app-arm64/milcam`.
  Verifies: ELF aarch64, NEEDED = libQt5{Widgets,Gui,Core}.so.5 + libc/m/pthread, glibc refs ≤ `GLIBC_2.17`.
  (Direct g++: Bootlin glibc-2.27 toolchain + desktop Qt headers + device `.so.5` via `-l:`, `-static-libstdc++`.)
- **(optional) static core on device:** cross-build `core_tests` and run on the panel to prove toolchain/glibc — see TOOLCHAIN_NOTES.md.

## 4. Deploy + run (dev helper)
`bash scripts/run-on-device.sh` — streams `milcam` + `run.sh` to `/root/milcam/`, launches it (linuxfb fullscreen), prints pid + log. No chvt / no VT.
Manual equivalent:
```sh
ssh -i ~/.ssh/milcam_id root@192.168.1.123 'mkdir -p /root/milcam'
ssh -i ~/.ssh/milcam_id root@192.168.1.123 'cat > /root/milcam/milcam' < build-app-arm64/milcam
ssh -i ~/.ssh/milcam_id root@192.168.1.123 'cat > /root/milcam/run.sh'  < scripts/device/run-milcam.sh
ssh -i ~/.ssh/milcam_id root@192.168.1.123 'chmod +x /root/milcam/milcam /root/milcam/run.sh'
```
Visual verify (no DRM now): capture the framebuffer:
`ssh ... 'dd if=/dev/fb0 bs=4096 count=768' > fb.raw` → PIL `Image.frombytes('RGB',(1024,768),data,'raw','BGRX')`.

## 5. CodeSYS integration (the HMI ⇄ MilCAM switch)
Switching uses **launch/exit**, NOT VT switching (chvt is dead here — single `/dev/fb0`,
display = last writer, CodeSYS won't release tty1 nor auto-repaint). Full rationale +
ST code: [CODESYS_RETURN_BUTTON.md](CODESYS_RETURN_BUTTON.md). Steps:

1. **Enable SysProcess** in `/root/CODESYSControl.cfg` (else the launch command is denied):
   ```ini
   [SysProcess]
   Command=AllowAll
   ```
   Then **restart the CodeSYS runtime** (do this only when the machine is safe). Back up the cfg first.
2. **"MilCAM" button** (Visu) → toggles `xToMilcam`; task runs:
   `sCmd := 'setsid /root/milcam/run.sh >/tmp/milcam.log 2>&1 &';` via `SysProcessExecuteCommand2`. The `&` is required (non-blocking).
3. **"CNC" button** is inside MilCAM (top-right) → exits the app. CodeSYS detects the
   exit (`pidof milcam` poll) and forces a **full TargetVisu repaint** (bounce `CurrentVisu`,
   or toggle a full-screen curtain rectangle) to reclaim the screen.

## 6. Touch
- Weida Hi-Tech CoolTouch on `/dev/input/event1`. MilCAM auto-detects it via Qt
  `evdevtouch` (no extra config). Verified: taps register (the CNC button exits the app).
- Calibration: `/etc/pointercal` exists (tslib); evdevtouch maps raw 0..32767 linearly.
  If taps feel offset on a new unit, calibrate or add an evdevtouch transform.

## 7. Quick smoke test on a deployed unit
1. From the CodeSYS HMI, press **MilCAM** → MilCAM appears fullscreen (light/blue CAM shell).
2. Press **CNC** (top-right) → MilCAM exits; CodeSYS repaint brings the CNC HMI back.
3. (host parity) `bash scripts/build-host.sh` → `ALL PASS`.

## 8. Rollback
- Remove MilCAM: `rm -rf /root/milcam`; restore cfg: `cp /root/CODESYSControl.cfg.bak-milcam /root/CODESYSControl.cfg` + runtime restart.
- Re-enable X (if ever needed): rename `/etc/init.d/_S40xorg` → `S40xorg`.
- Stock `/usr` backup on device: `/root/usr-stock-backup.tgz`.
