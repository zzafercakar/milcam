# SPDX-License-Identifier: Proprietary
# MilCAM — Gui init: register the MilCAM workbench, hide non-CAM workbenches.

import FreeCAD
import FreeCADGui


class MilCamWorkbench(FreeCADGui.Workbench):
    """Touch-first CAM workbench for CodeSys CNC panel PCs.

    Wraps FreeCAD's own CAM (Path) workbench: same operations, same Job
    object, same toolbits — but with added 'Send to CodeSys' command,
    CODESYS-flavor postprocessor as default, and on-screen keyboard hooks.
    """

    MenuText = "MilCAM"
    ToolTip  = "Touch-first CAM for CodeSys CNC"
    Icon = """/* XPM */
static char * MilCAM_xpm[] = {
"16 16 3 1",
"  c None",
". c #2080C0",
"+ c #FFFFFF",
"                ",
"  ............. ",
"  .+++++++++++. ",
"  .+         +. ",
"  .+  M I L  +. ",
"  .+         +. ",
"  .+  C A M  +. ",
"  .+         +. ",
"  .+++++++++++. ",
"  ............. ",
"                ",
"                ",
"                ",
"                ",
"                ",
"                "};
"""

    def Initialize(self):
        # Pull in FreeCAD's own CAM/Path commands — we are NOT reimplementing
        # CAM, we are wrapping it.
        try:
            import CAMGui            # FreeCAD 1.0+
            import CAM
        except ImportError:
            try:
                import PathGui       # older naming
                import Path
            except ImportError:
                FreeCAD.Console.PrintError(
                    "MilCAM: neither CAMGui nor PathGui module is available. "
                    "Rebuild FreeCAD with BUILD_CAM=ON.\n")
                return

        # Register MilCAM-specific commands.
        from Commands import SendToCodesys, SwitchToHmi, AboutMilCAM
        SendToCodesys.register()
        SwitchToHmi.register()
        AboutMilCAM.register()

        # Compose the toolbar — large icons for touch use.
        self.appendToolbar("MilCAM Job", [
            "MilCAM_SendToCodesys",
            "MilCAM_SwitchToHmi",
        ])
        self.appendToolbar("CAM Operations", [
            "CAM_Profile", "CAM_Pocket", "CAM_Drilling", "CAM_Engrave",
            "CAM_3DSurface", "CAM_Adaptive", "CAM_PostProcess",
        ])
        self.appendMenu("MilCAM", [
            "MilCAM_SendToCodesys", "MilCAM_SwitchToHmi",
            "Separator", "MilCAM_About",
        ])

    def Activated(self):
        # Push touch-friendly Qt application defaults: larger fonts,
        # show the virtual keyboard on text input focus.
        from PySide import QtCore, QtGui, QtWidgets
        app = QtWidgets.QApplication.instance()
        if app is not None:
            font = app.font()
            font.setPointSize(max(font.pointSize(), 12))
            app.setFont(font)

        # Enable Qt virtual keyboard (assumes Qt was built with it).
        import os
        os.environ.setdefault("QT_IM_MODULE", "qtvirtualkeyboard")

        # Set CODESYS as the default postprocessor for new CAM Jobs.
        try:
            params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
            params.SetString("DefaultPostProcessor", "codesys")
            params.SetString("DefaultPostProcessorArgs", "--no-show-editor")
        except Exception as exc:
            FreeCAD.Console.PrintWarning(
                "MilCAM: could not set CODESYS as default post: {}\n".format(exc))

        FreeCAD.Console.PrintMessage("MilCAM workbench activated.\n")

    def Deactivated(self):
        pass

    def GetClassName(self):
        return "Gui::PythonWorkbench"


# Register MilCAM and hide non-CAM workbenches.
FreeCADGui.addWorkbench(MilCamWorkbench)


def _hide_non_milcam_workbenches():
    """Disable non-CAM workbenches in the workbench selector.

    Compile-time module stripping (BUILD_FEM=OFF etc.) removes the bulk of
    the noise. But Sketcher and PartDesign MUST stay compiled (CAM depends
    on them) — we just hide them from the operator's workbench picker.
    """
    keep = {
        "MilCamWorkbench",        # us
        "CAMWorkbench",           # FreeCAD's CAM (we wrap it)
        "PartDesignWorkbench",    # CAM depends on bodies
        "SketcherWorkbench",      # CAM depends on sketches
        "PartWorkbench",          # geometry import
        "DraftWorkbench",         # DXF import
        "MeshWorkbench",          # STL import
        "SpreadsheetWorkbench",   # tool tables
    }
    try:
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Workbenches")
        all_wbs = FreeCADGui.listWorkbenches()
        disabled = [name for name in all_wbs if name not in keep]
        params.SetString("Disabled", ",".join(disabled))
        params.SetString("Ordered",  "MilCamWorkbench,CAMWorkbench,PartDesignWorkbench,SketcherWorkbench,PartWorkbench,DraftWorkbench,MeshWorkbench,SpreadsheetWorkbench")
    except Exception as exc:
        FreeCAD.Console.PrintWarning(
            "MilCAM: could not configure workbench visibility: {}\n".format(exc))


_hide_non_milcam_workbenches()

# Make MilCAM the default workbench on first launch.
try:
    _params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/General")
    if not _params.GetString("AutoloadModule"):
        _params.SetString("AutoloadModule", "MilCamWorkbench")
except Exception:
    pass
