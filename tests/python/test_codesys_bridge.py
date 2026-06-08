"""Smoke tests for the CodesysBridge C extension.

These tests run only if the compiled module is available on PYTHONPATH.
"""
import os
import sys
import tempfile

try:
    import CodesysBridge
except ImportError:
    CodesysBridge = None


def _skip_if_no_bridge():
    if CodesysBridge is None:
        import pytest
        pytest.skip("CodesysBridge extension module not built")


def test_create_bridge():
    _skip_if_no_bridge()
    b = CodesysBridge.Bridge()
    assert b is not None


def test_drop_folder_setter():
    _skip_if_no_bridge()
    with tempfile.TemporaryDirectory() as d:
        b = CodesysBridge.Bridge()
        b.drop_folder = d
        assert b.drop_folder == d


def test_drop_gcode_writes_file():
    _skip_if_no_bridge()
    with tempfile.TemporaryDirectory() as d:
        b = CodesysBridge.Bridge()
        b.drop_folder = d
        out = b.drop_gcode("smoke_42", "G0 X0\nG1 X10 F500\nM30\n")
        assert out
        assert os.path.exists(out)
        with open(out) as f:
            content = f.read()
        assert "M30" in content
        assert "smoke_42" in content       # header comment uses jobId


def test_state_enum_constants_exposed():
    _skip_if_no_bridge()
    assert CodesysBridge.STATE_IDLE   == 1
    assert CodesysBridge.STATE_ESTOP  == 5
