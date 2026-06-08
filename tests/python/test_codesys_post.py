"""Smoke tests for the CODESYS postprocessor.

The postprocessor is FreeCAD-aware but pure-Python; we can run its argument
parser and header emitter without a running FreeCAD.
"""
import os
import sys
import tempfile

# Make the postprocessor importable.
HERE = os.path.dirname(__file__)
sys.path.insert(0, os.path.abspath(os.path.join(HERE, "..", "..", "milcam", "Mod", "MilCAM", "PostProcessor")))

import codesys_post as cp


def test_parse_args_defaults():
    p = cp._parse_args("")
    assert p["units"] == "mm"
    assert p["header"] is True
    assert p["modal"] is True
    assert p["show_editor"] is False


def test_parse_args_overrides():
    p = cp._parse_args("--inch --no-header --no-modal --show-editor")
    assert p["units"] == "in"
    assert p["header"] is False
    assert p["modal"] is False
    assert p["show_editor"] is True


def test_header_block_contains_units():
    p = cp._parse_args("--metric")
    lines = cp._header_block(p, [])
    assert any("G21" in l for l in lines)


def test_export_writes_file():
    p = cp.PARAMS.copy()
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "out.gcode")
        gcode = cp.export([], out, "")
        assert os.path.exists(out)
        with open(out) as f:
            content = f.read()
        assert "G21" in content              # metric units header
        assert "M30" in content              # postamble end-of-program
