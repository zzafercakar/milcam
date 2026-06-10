#!/usr/bin/env bash
# scripts/run-on-device.sh — dev helper: deploy the cross-built MilCAM app + its
# launcher to the panel and start it. No sftp on device: stream via ssh+cat.
# NOTE: no chvt / VT switching — proved unworkable on this single-framebuffer
# device. MilCAM paints /dev/fb0 fullscreen (covers the CodeSYS HMI); the operator
# returns via MilCAM's "CNC" button (the app exits) and CodeSYS reclaims the
# screen with a full TargetVisu repaint. See .ai/ENGINEERING_LOG.md.
set -euo pipefail
DEV=${DEV:-root@192.168.1.123}
KEY=${KEY:-$HOME/.ssh/milcam_id}
SSH="ssh -i $KEY -o StrictHostKeyChecking=no -o ConnectTimeout=10"
BIN=${BIN:-build-app-arm64/milcam}

[ -f "$BIN" ] || { echo "ERROR: $BIN not found — run scripts/build-app-arm64.sh first"; exit 1; }

echo "=== deploy $BIN + run.sh -> $DEV:/root/milcam/ ==="
$SSH "$DEV" 'killall milcam 2>/dev/null; sleep 1; mkdir -p /root/milcam'
$SSH "$DEV" 'cat > /root/milcam/milcam' < "$BIN"
$SSH "$DEV" 'cat > /root/milcam/run.sh' < scripts/device/run-milcam.sh

echo "=== launch (linuxfb fullscreen) ==="
$SSH "$DEV" 'chmod +x /root/milcam/milcam /root/milcam/run.sh
  setsid /root/milcam/run.sh >/tmp/milcam.log 2>&1 &
  sleep 4
  echo "pid: $(pidof milcam || echo NONE)"
  tail -3 /tmp/milcam.log'
