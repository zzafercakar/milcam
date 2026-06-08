# SPDX-License-Identifier: Proprietary
# Command: post-process the active CAM job and drop the G-code into the
# CodeSys watch folder, then notify the PLC via OPC UA.

import os
import time

import FreeCAD
import FreeCADGui

try:
    import CodesysBridge
except ImportError:
    CodesysBridge = None


_BRIDGE = None


def _get_bridge():
    global _BRIDGE
    if _BRIDGE is None and CodesysBridge is not None:
        _BRIDGE = CodesysBridge.Bridge()
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/MilCAM")
        _BRIDGE.endpoint_url = params.GetString(
            "EndpointUrl", "opc.tcp://192.168.1.123:4840")
        _BRIDGE.drop_folder = params.GetString(
            "DropFolder", "/var/cnc/jobs")
    return _BRIDGE


class CommandSendToCodesys:
    def GetResources(self):
        return {
            "Pixmap":  "MilCAM_SendToCodesys",
            "MenuText": "Send to CodeSys",
            "ToolTip":  "Post-process the active CAM Job and hand it to the "
                        "running CodeSys CNC via drop folder + OPC UA.",
            "Accel":    "Ctrl+Shift+S",
        }

    def IsActive(self):
        if CodesysBridge is None:
            return False
        doc = FreeCAD.ActiveDocument
        if doc is None:
            return False
        # Active if there's at least one CAM Job in the doc.
        return any(getattr(o, "TypeId", "") == "Path::FeatureCompoundPython"
                   for o in doc.Objects) or any(
                   "CAM::Job" in getattr(o, "TypeId", "") for o in doc.Objects)

    def Activated(self):
        bridge = _get_bridge()
        if bridge is None:
            FreeCAD.Console.PrintError(
                "MilCAM: CodesysBridge module not loaded.\n")
            return

        doc = FreeCAD.ActiveDocument
        job = self._find_active_job(doc)
        if job is None:
            FreeCAD.Console.PrintError("MilCAM: no CAM Job in active document.\n")
            return

        # Run FreeCAD's CAM post-processor. It writes a file to a temp path,
        # we re-read it and hand the contents to CodesysBridge.
        try:
            from CAM.Post import PostProcessor    # FreeCAD 1.0+
        except ImportError:
            try:
                from PathScripts import PostUtils, PathPost
                PostProcessor = None  # use legacy path
            except ImportError:
                FreeCAD.Console.PrintError(
                    "MilCAM: cannot import CAM Post module.\n")
                return

        tmp_dir = "/tmp/milcam"
        os.makedirs(tmp_dir, exist_ok=True)
        tmp_file = os.path.join(tmp_dir, "send_to_codesys.gcode")

        # Configure job to use codesys post + tmp output, then post.
        old_post   = getattr(job, "PostProcessor", None)
        old_output = getattr(job, "PostProcessorOutputFile", None)
        try:
            job.PostProcessor = "codesys"
            job.PostProcessorOutputFile = tmp_file
            FreeCADGui.runCommand("CAM_Post")  # invokes the post
        finally:
            if old_post is not None:    job.PostProcessor = old_post
            if old_output is not None:  job.PostProcessorOutputFile = old_output

        if not os.path.exists(tmp_file):
            FreeCAD.Console.PrintError(
                "MilCAM: post-processor did not produce {}\n".format(tmp_file))
            return

        with open(tmp_file, "r", encoding="utf-8") as f:
            gcode = f.read()

        job_id = "{}_{}".format(
            (doc.Label or "job").replace(" ", "_"),
            int(time.time()))
        final_path = bridge.drop_gcode(job_id, gcode)
        if not final_path:
            FreeCAD.Console.PrintError("MilCAM: drop_gcode failed.\n")
            return

        FreeCAD.Console.PrintMessage(
            "MilCAM: dropped {} bytes to {}\n".format(len(gcode), final_path))

        if bridge.is_connected():
            ok = bridge.notify_job_ready(final_path)
            FreeCAD.Console.PrintMessage(
                "MilCAM: PLC notified ({}).\n".format("OK" if ok else "FAIL"))

    @staticmethod
    def _find_active_job(doc):
        for o in doc.Objects:
            if "CAM::Job" in getattr(o, "TypeId", "") or \
               getattr(o, "TypeId", "") == "Path::FeatureCompoundPython":
                return o
        return None


def register():
    FreeCADGui.addCommand("MilCAM_SendToCodesys", CommandSendToCodesys())
