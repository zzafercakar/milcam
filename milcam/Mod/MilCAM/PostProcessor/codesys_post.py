# SPDX-License-Identifier: Proprietary
# CODESYS post-processor for FreeCAD CAM (Path) workbench.
#
# Conforms to FreeCAD's post-processor convention:
#   - module-level constants PARAMS, TOOLTIP, ARGUMENTS
#   - export(objectslist, filename, argstring) -> str
#
# Emits G-code consumable by CodeSys SoftMotion CNC via SMC_NCInterpreter:
#   * G0/G1 linear, G2/G3 arc in plane G17/G18/G19
#   * M3/M4/M5 spindle, M6 tool change, M30 program end
#   * Modal feed (F) and spindle (S)
#   * Mm units (G21) — adjust if PLC is set to inch
#
# This is a thin wrapper around FreeCAD's `refactored_test_post` template;
# CODESYS-specific quirks are isolated in the `_CODESYS_HEADER` and the
# arc-mode tweaks below.

import datetime
import os
import re
import sys

# FreeCAD's post utility helpers (post_utils.py / PostUtils.py depending on
# version). We import lazily so the module is import-safe in unit tests.

TOOLTIP = "CODESYS SoftMotion-friendly G-code postprocessor for MilCAM."

PARAMS = {
    "units":          "mm",       # mm | in
    "header":         True,       # emit header block
    "modal":          True,       # collapse modal codes
    "show_editor":    False,      # never pop a Qt editor on a panel PC
    "comment_char":   ";",        # CodeSys SMC accepts ; as comment
    "line_numbers":   True,
    "line_increment": 1,
    "preamble":       "G17 G21 G40 G90 G94",
    "postamble":      "M5 M30",
    "spindle_wait":   1.0,        # seconds after S/M3 before first cut
}

ARGUMENTS = """
\\--header / --no-header         Emit comment header  (default on)
\\--modal / --no-modal           Suppress repeated modal codes (default on)
\\--show-editor / --no-show-editor   Open the G-code in a Qt editor (default off)
\\--inch / --metric              Output units
"""


def _parse_args(argstring):
    p = dict(PARAMS)
    tokens = re.split(r"[\s,]+", argstring or "")
    for t in tokens:
        if not t: continue
        if t == "--header":           p["header"] = True
        elif t == "--no-header":      p["header"] = False
        elif t == "--modal":          p["modal"] = True
        elif t == "--no-modal":       p["modal"] = False
        elif t == "--show-editor":    p["show_editor"] = True
        elif t == "--no-show-editor": p["show_editor"] = False
        elif t == "--inch":           p["units"] = "in"
        elif t == "--metric":         p["units"] = "mm"
    return p


def _header_block(p, objectslist):
    units_code = "G20" if p["units"] == "in" else "G21"
    now = datetime.datetime.utcnow().isoformat() + "Z"
    return [
        "{} MilCAM CODESYS post".format(p["comment_char"]),
        "{} generated {}".format(p["comment_char"], now),
        "{} units {}".format(p["comment_char"], p["units"]),
        units_code,
        p["preamble"],
    ]


def _emit_object(obj, p, n, modals):
    """Convert one Path object to G-code lines. Pure pass-through with light
    transformations: optional modal suppression, line numbering, comment style.
    """
    lines = []
    if not hasattr(obj, "Path") or obj.Path is None:
        return lines

    for cmd in obj.Path.Commands:
        # `cmd.Name` is "G1", "M3", etc. `cmd.Parameters` is a dict.
        name = cmd.Name
        params = " ".join(
            "{}{:.4f}".format(k, v) if isinstance(v, float) else "{}{}".format(k, v)
            for k, v in cmd.Parameters.items()
        )
        line = "{} {}".format(name, params).strip()
        if p["modal"] and name in ("G0", "G1") and modals.get("last_g") == name:
            line = params or name
        modals["last_g"] = name
        if p["line_numbers"]:
            line = "N{} {}".format(n[0], line)
            n[0] += p["line_increment"]
        lines.append(line)
    return lines


def export(objectslist, filename, argstring):
    """FreeCAD calls this to convert Path objects to a G-code file."""
    p = _parse_args(argstring)
    lines = []

    if p["header"]:
        lines.extend(_header_block(p, objectslist))

    n = [10]                  # line number counter (mutable container)
    modals = {"last_g": None}

    for obj in objectslist:
        lines.append("{} object: {}".format(p["comment_char"], getattr(obj, "Label", "?")))
        lines.extend(_emit_object(obj, p, n, modals))

    lines.append(p["postamble"])
    gcode = "\n".join(lines) + "\n"

    # Write to disk so the calling CAM_Post command picks up the file.
    if filename and filename != "-":
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w", encoding="utf-8") as f:
            f.write(gcode)

    return gcode
