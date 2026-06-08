#!/usr/bin/env bash
# MilCAM deployment to VEC-VE panel PC.
#
# Usage:  ./scripts/deploy.sh [user@host]
# Default target: root@192.168.1.123
#
# Pre-requisites:
#   - Cross-build done:  cmake --preset panel-pc && cmake --build build-aarch64
#   - SSH key copied to the device.
#
# What it does:
#   1. rsyncs build-aarch64/milcam + resources to /root/MilCAM/ on the device.
#   2. Optionally drops a sample systemd unit to /etc/systemd/system/.

set -euo pipefail

TARGET="${1:-root@192.168.1.123}"
LOCAL_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOCAL_BUILD="${LOCAL_ROOT}/build-aarch64"
REMOTE_ROOT="/root/MilCAM"

if [[ ! -x "${LOCAL_BUILD}/milcam" ]]; then
    echo "ERROR: ${LOCAL_BUILD}/milcam not found. Run:"
    echo "  cmake --preset panel-pc && cmake --build build-aarch64 -j\$(nproc)"
    exit 1
fi

echo "Deploying MilCAM to ${TARGET}:${REMOTE_ROOT} …"
ssh "${TARGET}" "mkdir -p ${REMOTE_ROOT}"

rsync -avz --delete \
    "${LOCAL_BUILD}/milcam" \
    "${LOCAL_ROOT}/resources" \
    "${TARGET}:${REMOTE_ROOT}/"

echo "Installing systemd unit (if not present) …"
ssh "${TARGET}" "cat > /etc/systemd/system/milcam.service" <<'UNIT'
[Unit]
Description=MilCAM (CAM for CodeSys CNC)
After=multi-user.target codesys.service

[Service]
Type=simple
Environment=DISPLAY=:0
Environment=QT_QPA_PLATFORM=xcb
Environment=QT_IM_MODULE=qtvirtualkeyboard
Environment=TSLIB_TSDEVICE=/dev/input/event1
Environment=QT_QPA_GENERIC_PLUGINS=tslib:/dev/input/event1
Environment=XDG_RUNTIME_DIR=/usr/lib/
Environment=LD_LIBRARY_PATH=/lib:/usr/lib:/usr/lib/ts:/root/MilCAM
ExecStart=/root/MilCAM/milcam
Restart=on-failure
RestartSec=3
# Pin to non-RT cores; adjust to match the device's isolcpus= setting.
CPUAffinity=0 1

[Install]
WantedBy=multi-user.target
UNIT

echo "Done. To enable on boot:"
echo "  ssh ${TARGET} 'systemctl daemon-reload && systemctl enable --now milcam'"
