#!/bin/sh
# /root/milcam/run.sh  (deployed to the device)
# Launcher the CodeSYS HMI calls to bring up MilCAM. MilCAM paints fullscreen to
# /dev/fb0 (no GL); the operator returns via MilCAM's "CNC" button (restores the
# captured CNC frame + exits). No chvt / VT switching.
#
# HARDENED against a caller that fires the launch repeatedly (observed: the
# running CodeSYS program calls this ~every 2-3 s). Two guards keep the screen
# stable regardless:
#   1) cooldown: right after the operator pressed "CNC" (MilCAM wrote
#      /tmp/milcam_cooldown), ignore launches for a short window so MilCAM does
#      not immediately reopen over the CNC screen.
#   2) idempotent: if MilCAM is already running, do nothing (no kill+restart →
#      no flicker) when the launch is called again.
echo "$(date +%s) run.sh invoked" >> /tmp/run_called.log

if [ -f /tmp/milcam_cooldown ] && \
   [ $(( $(date +%s) - $(cat /tmp/milcam_cooldown 2>/dev/null || echo 0) )) -lt 10 ]; then
    echo "$(date +%s) cooldown active, skip" >> /tmp/run_called.log
    exit 0
fi

if pidof milcam >/dev/null 2>&1; then
    exit 0          # already up — leave it alone
fi

cd /root/milcam
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt/plugins
export QT_QPA_FONTDIR=/usr/lib/fonts
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export LD_LIBRARY_PATH=/usr/lib:/lib
exec ./milcam
