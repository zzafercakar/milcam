# SPDX-License-Identifier: Proprietary
# MilCAM — non-Gui init (runs on every FreeCAD startup, including console mode).
#
# Console-mode use case: a CI job that loads a STEP, runs Path operations,
# emits G-code via the CODESYS postprocessor, drops the file. No FreeCAD UI.

import os
import sys

# Make sure FreeCAD finds the bundled CODESYS postprocessor in PathScripts/post/.
_THIS_DIR = os.path.dirname(__file__)
_POST_DIR = os.path.join(_THIS_DIR, "PostProcessor")
if _POST_DIR not in sys.path:
    sys.path.insert(0, _POST_DIR)
