// milcam/app/src/gui/CncBackdrop.h
#pragma once
#include <vector>

// The /dev/fb0 contents captured at MilCAM launch — i.e. the CodeSYS CNC HMI that
// was on screen when the operator pressed "MilCAM". MilCAM restores it to the
// framebuffer on exit, so returning to CNC does NOT depend on CodeSYS repainting
// (which proved unreliable to trigger on this single-framebuffer device). Empty
// if the capture failed (then MilCAM falls back to clearing the screen white).
std::vector<unsigned char>& cncBackdrop();
