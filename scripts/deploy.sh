#!/usr/bin/env bash
# MilCAM deployment to VEC-VE panel PC.
#
# Usage:  ./scripts/deploy.sh [user@host]
# Default target: root@192.168.1.123
#
# Pre-requisites:
#   - cmake --preset panel-pc && cmake --build build-aarch64
#   - DESTDIR=$PWD/install-aarch64 cmake --install build-aarch64
#   - SSH key copied to the device.
#
# What it does:
#   1. rsyncs the installed FreeCAD + MilCAM tree to /opt/milcam/ on the device.
#   2. Installs a systemd unit pre-configured with VEC-VE env vars.
#   3. Configures the workbench-only default via FreeCAD prefs.

set -euo pipefail

TARGET="${1:-root@192.168.1.123}"
LOCAL_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOCAL_INSTALL="${LOCAL_ROOT}/install-aarch64"
REMOTE_ROOT="/opt/milcam"

if [[ ! -d "${LOCAL_INSTALL}" ]]; then
    echo "ERROR: ${LOCAL_INSTALL} not found. Build and install first:"
    echo "  cmake --preset panel-pc"
    echo "  cmake --build build-aarch64 -j\$(nproc)"
    echo "  DESTDIR=${LOCAL_INSTALL} cmake --install build-aarch64"
    exit 1
fi

echo "Deploying MilCAM to ${TARGET}:${REMOTE_ROOT} …"
ssh "${TARGET}" "mkdir -p ${REMOTE_ROOT}"

rsync -avz --delete \
    "${LOCAL_INSTALL}/" \
    "${TARGET}:${REMOTE_ROOT}/"

echo "Installing systemd unit (overwrites if present) …"
ssh "${TARGET}" "cat > /etc/systemd/system/milcam.service" <<'UNIT'
[Unit]
Description=MilCAM (slim FreeCAD CAM for CodeSys CNC)
After=multi-user.target codesys.service
Wants=codesys.service

[Service]
Type=simple
Environment=DISPLAY=:0
Environment=QT_QPA_PLATFORM=xcb
Environment=QT_IM_MODULE=qtvirtualkeyboard
Environment=TSLIB_TSDEVICE=/dev/input/event1
Environment=QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
Environment=XDG_RUNTIME_DIR=/usr/lib/
Environment=LD_LIBRARY_PATH=/opt/milcam/usr/local/lib:/lib:/usr/lib:/usr/lib/ts
ExecStart=/opt/milcam/usr/local/bin/FreeCAD
Restart=on-failure
RestartSec=3
# Pin to non-RT cores; adjust to match the device's isolcpus= setting.
CPUAffinity=0 1

[Install]
WantedBy=multi-user.target
UNIT

# Pre-set the default workbench to MilCAM on first launch so the operator
# does not see the start page or any other default.
echo "Seeding FreeCAD user preferences …"
ssh "${TARGET}" "mkdir -p /root/.FreeCAD && cat > /root/.FreeCAD/user.cfg" <<'CFG'
<?xml version="1.0" encoding="UTF-8"?>
<FCParameters>
  <FCParamGroup Name="Root">
    <FCParamGroup Name="BaseApp">
      <FCParamGroup Name="Preferences">
        <FCParamGroup Name="General">
          <FCText Name="AutoloadModule">MilCamWorkbench</FCText>
          <FCBool Name="ShowSplasher" Value="0"/>
          <FCBool Name="ShowTipOfTheDay" Value="0"/>
        </FCParamGroup>
      </FCParamGroup>
    </FCParamGroup>
  </FCParamGroup>
</FCParameters>
CFG

echo
echo "Done. To enable on boot:"
echo "  ssh ${TARGET} 'systemctl daemon-reload && systemctl enable --now milcam'"
echo
echo "To verify:"
echo "  ssh ${TARGET} 'ls /opt/milcam/usr/local/share/freecad/Mod/MilCAM/'"
