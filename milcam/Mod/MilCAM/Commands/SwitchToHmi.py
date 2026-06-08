# SPDX-License-Identifier: Proprietary
# Command: raise the CodeSys TargetVisu HMI window to the foreground.
# Operator presses this button on the MilCAM toolbar to leave CAM and go
# back to the running machine view.

import FreeCAD
import FreeCADGui

try:
    import CodesysBridge
except ImportError:
    CodesysBridge = None


class CommandSwitchToHmi:
    def GetResources(self):
        return {
            "Pixmap":   "MilCAM_SwitchToHmi",
            "MenuText": "Switch to HMI",
            "ToolTip":  "Raise the CodeSys TargetVisu window (via wmctrl).",
            "Accel":    "Ctrl+Shift+H",
        }

    def IsActive(self):
        return CodesysBridge is not None

    def Activated(self):
        if CodesysBridge is None:
            return
        title = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/MilCAM"
        ).GetString("HmiWindowTitle", "TargetVisu")
        if not CodesysBridge.raise_window(title):
            FreeCAD.Console.PrintWarning(
                "MilCAM: wmctrl could not raise window matching '{}'.\n".format(title))


def register():
    FreeCADGui.addCommand("MilCAM_SwitchToHmi", CommandSwitchToHmi())
