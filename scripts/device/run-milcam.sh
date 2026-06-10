#!/bin/sh
# /root/milcam/run.sh  (deployed to the device)
# Launcher the CodeSYS HMI calls to bring up MilCAM. CodeSYS button runs:
#   setsid /root/milcam/run.sh >/tmp/milcam.log 2>&1 &
# MilCAM paints fullscreen to /dev/fb0 (no GL), covering the CodeSYS HMI. The
# operator returns via MilCAM's "CNC" button (the app exits); CodeSYS then forces
# a full TargetVisu repaint to reclaim the screen. No chvt / VT switching is used
# (proved unworkable on this single-framebuffer device — see .ai/ENGINEERING_LOG.md).
cd /root/milcam
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/qt/plugins
export QT_QPA_FONTDIR=/usr/lib/fonts
export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb0
export LD_LIBRARY_PATH=/usr/lib:/lib
exec ./milcam
