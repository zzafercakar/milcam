# SPDX-License-Identifier: Proprietary
# Command: show a minimal About dialog with the MilCAM version + PLC status.

import FreeCAD
import FreeCADGui

try:
    import CodesysBridge
except ImportError:
    CodesysBridge = None

MILCAM_VERSION = "0.2.0-overlay"


class CommandAboutMilCAM:
    def GetResources(self):
        return {
            "Pixmap":   "MilCAM",
            "MenuText": "About MilCAM",
            "ToolTip":  "MilCAM version + PLC link status.",
        }

    def IsActive(self):
        return True

    def Activated(self):
        from PySide import QtWidgets
        bridge_status = "loaded" if CodesysBridge is not None else "not loaded"
        text = (
            "MilCAM " + MILCAM_VERSION + "\n"
            "Touch-first CAM for CodeSys CNC panel PCs.\n\n"
            "FreeCAD: " + FreeCAD.Version()[0] + "." + FreeCAD.Version()[1] + "\n"
            "CodesysBridge module: " + bridge_status + "\n"
        )
        QtWidgets.QMessageBox.information(None, "About MilCAM", text)


def register():
    FreeCADGui.addCommand("MilCAM_About", CommandAboutMilCAM())
