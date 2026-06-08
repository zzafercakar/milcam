// SPDX-License-Identifier: Proprietary
// CodesysBridgePy — Python bindings for MilCAM::CodesysBridge.
//
// Exposed as the `CodesysBridge` Python extension module so FreeCAD's
// MilCAM workbench can call it from InitGui.py and command handlers:
//
//     import CodesysBridge
//     bridge = CodesysBridge.Bridge()
//     bridge.set_drop_folder("/var/cnc/jobs")
//     path = bridge.drop_gcode("job_42", gcode_string)
//     bridge.notify_job_ready(path)
//     CodesysBridge.raise_window("TargetVisu")
//
// The module follows CPython's stable-ABI C API to avoid PyCXX coupling —
// keeps the build matrix smaller.

#include "CodesysBridge.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <new>
#include <string>

using MilCAM::CodesysBridge;

namespace {

struct PyBridge {
    PyObject_HEAD
    CodesysBridge* bridge;
};

static void Bridge_dealloc(PyBridge* self) {
    delete self->bridge;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

static PyObject* Bridge_new(PyTypeObject* type, PyObject*, PyObject*) {
    auto* self = reinterpret_cast<PyBridge*>(type->tp_alloc(type, 0));
    if (!self) return nullptr;
    self->bridge = new (std::nothrow) CodesysBridge();
    if (!self->bridge) { Py_DECREF(self); return PyErr_NoMemory(); }
    return reinterpret_cast<PyObject*>(self);
}

#define BRIDGE_GETSET(name, getter, setter)                                       \
    static PyObject* Bridge_get_##name(PyBridge* self, void*) {                   \
        return PyUnicode_FromString(self->bridge->getter().c_str());              \
    }                                                                             \
    static int Bridge_set_##name(PyBridge* self, PyObject* value, void*) {        \
        if (!PyUnicode_Check(value)) {                                            \
            PyErr_SetString(PyExc_TypeError, "expected string"); return -1;       \
        }                                                                         \
        self->bridge->setter(PyUnicode_AsUTF8(value));                            \
        return 0;                                                                 \
    }

BRIDGE_GETSET(endpoint_url, endpointUrl, setEndpointUrl)
BRIDGE_GETSET(drop_folder,  dropFolder,  setDropFolder)

static PyObject* Bridge_drop_gcode(PyBridge* self, PyObject* args) {
    const char* jobId;
    const char* gcode;
    if (!PyArg_ParseTuple(args, "ss", &jobId, &gcode)) return nullptr;
    const std::string out = self->bridge->dropGCode(jobId, gcode);
    return PyUnicode_FromString(out.c_str());
}

static PyObject* Bridge_prune_old_jobs(PyBridge* self, PyObject* args) {
    int days = 30;
    if (!PyArg_ParseTuple(args, "|i", &days)) return nullptr;
    return PyLong_FromLong(self->bridge->pruneOldJobs(days));
}

static PyObject* Bridge_connect(PyBridge* self, PyObject*) {
    return PyBool_FromLong(self->bridge->connect());
}

static PyObject* Bridge_disconnect(PyBridge* self, PyObject*) {
    self->bridge->disconnect(); Py_RETURN_NONE;
}

static PyObject* Bridge_is_connected(PyBridge* self, PyObject*) {
    return PyBool_FromLong(self->bridge->isConnected());
}

static PyObject* Bridge_notify_job_ready(PyBridge* self, PyObject* args) {
    const char* path;
    if (!PyArg_ParseTuple(args, "s", &path)) return nullptr;
    return PyBool_FromLong(self->bridge->notifyJobReady(path));
}

static PyObject* Bridge_machine_state(PyBridge* self, PyObject*) {
    return PyLong_FromLong(static_cast<int>(self->bridge->machineState()));
}

static PyObject* Bridge_current_line(PyBridge* self, PyObject*) {
    return PyLong_FromLong(self->bridge->currentLine());
}

static PyMethodDef Bridge_methods[] = {
    {"drop_gcode",       (PyCFunction)Bridge_drop_gcode,       METH_VARARGS, "Atomic write of G-code to <dropFolder>/<jobId>.gcode."},
    {"prune_old_jobs",   (PyCFunction)Bridge_prune_old_jobs,   METH_VARARGS, "Remove .gcode files older than N days."},
    {"connect",          (PyCFunction)Bridge_connect,          METH_NOARGS,  "OPC UA connect."},
    {"disconnect",       (PyCFunction)Bridge_disconnect,       METH_NOARGS,  "OPC UA disconnect."},
    {"is_connected",     (PyCFunction)Bridge_is_connected,     METH_NOARGS,  "OPC UA connected state."},
    {"notify_job_ready", (PyCFunction)Bridge_notify_job_ready, METH_VARARGS, "Write MilCAM.JobReadyPath OPC UA symbol."},
    {"machine_state",    (PyCFunction)Bridge_machine_state,    METH_NOARGS,  "Current PLC machine state (int enum)."},
    {"current_line",     (PyCFunction)Bridge_current_line,     METH_NOARGS,  "Current G-code line being executed."},
    {nullptr, nullptr, 0, nullptr}
};

static PyGetSetDef Bridge_getset[] = {
    {(char*)"endpoint_url", (getter)Bridge_get_endpoint_url, (setter)Bridge_set_endpoint_url, (char*)"OPC UA endpoint URL", nullptr},
    {(char*)"drop_folder",  (getter)Bridge_get_drop_folder,  (setter)Bridge_set_drop_folder,  (char*)"G-code drop folder path", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

static PyTypeObject BridgeType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    /* tp_name      */ "CodesysBridge.Bridge",
    /* tp_basicsize */ sizeof(PyBridge),
    /* tp_itemsize  */ 0,
    /* tp_dealloc   */ (destructor)Bridge_dealloc,
};

// Module-level free functions
static PyObject* mod_raise_window(PyObject*, PyObject* args) {
    const char* title;
    if (!PyArg_ParseTuple(args, "s", &title)) return nullptr;
    return PyBool_FromLong(CodesysBridge::raiseWindow(title));
}

static PyMethodDef ModuleMethods[] = {
    {"raise_window", mod_raise_window, METH_VARARGS, "Raise an X11 window by title substring (wmctrl)."},
    {nullptr, nullptr, 0, nullptr}
};

static PyModuleDef ModuleDef = {
    PyModuleDef_HEAD_INIT,
    "CodesysBridge",                              // m_name
    "MilCAM ↔ CodeSys runtime bridge (drop folder + OPC UA).",
    -1,
    ModuleMethods
};

} // namespace

PyMODINIT_FUNC PyInit_CodesysBridge(void) {
    BridgeType.tp_new       = Bridge_new;
    BridgeType.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    BridgeType.tp_doc       = "MilCAM CodeSys bridge — one instance per FreeCAD session.";
    BridgeType.tp_methods   = Bridge_methods;
    BridgeType.tp_getset    = Bridge_getset;
    if (PyType_Ready(&BridgeType) < 0) return nullptr;

    PyObject* m = PyModule_Create(&ModuleDef);
    if (!m) return nullptr;

    Py_INCREF(&BridgeType);
    if (PyModule_AddObject(m, "Bridge", reinterpret_cast<PyObject*>(&BridgeType)) < 0) {
        Py_DECREF(&BridgeType); Py_DECREF(m); return nullptr;
    }

    // Machine state enum as module-level constants
    PyModule_AddIntConstant(m, "STATE_UNKNOWN", 0);
    PyModule_AddIntConstant(m, "STATE_IDLE",    1);
    PyModule_AddIntConstant(m, "STATE_RUNNING", 2);
    PyModule_AddIntConstant(m, "STATE_PAUSED",  3);
    PyModule_AddIntConstant(m, "STATE_ERROR",   4);
    PyModule_AddIntConstant(m, "STATE_ESTOP",   5);

    return m;
}
