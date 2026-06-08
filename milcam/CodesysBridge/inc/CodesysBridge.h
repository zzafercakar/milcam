// SPDX-License-Identifier: Proprietary
// CodesysBridge — MilCAM ↔ CodeSys runtime communication primitives.
//
// This is a thin C++ core meant to be exposed to Python via PyCXX (see
// CodesysBridgePy.cpp). FreeCAD imports the resulting `CodesysBridge`
// extension module from MilCAM's workbench overlay (Mod/MilCAM/).
//
// Two communication channels:
//   1. G-code drop folder — atomic file write, watched by the PLC.
//   2. OPC UA client      — symbol read/write for status + control.
//
// CodesysBridge is intentionally side-effect-only on the host filesystem and
// network — it does NOT touch FreeCAD's document model. That keeps it
// orthogonal to FreeCAD's threading rules.

#pragma once

#include <string>

namespace MilCAM {

enum class MachineState : int {
    Unknown = 0,
    Idle    = 1,
    Running = 2,
    Paused  = 3,
    Error   = 4,
    EStop   = 5,
};

class CodesysBridge {
public:
    CodesysBridge();
    ~CodesysBridge();

    // ── Configuration ────────────────────────────────────────────────
    void setEndpointUrl(const std::string& url);   // opc.tcp://host:port
    void setDropFolder (const std::string& path);  // /var/cnc/jobs
    const std::string& endpointUrl() const;
    const std::string& dropFolder()  const;

    // ── G-code drop folder ───────────────────────────────────────────
    // Atomic write: <jobId>.gcode.tmp → rename to <jobId>.gcode.
    // Returns final on-disk path, empty on failure.
    std::string dropGCode(const std::string& jobId, const std::string& gcode);

    // Remove .gcode files older than `days` from drop folder. Returns count.
    int pruneOldJobs(int days);

    // ── OPC UA (stubbed until Faz 3) ─────────────────────────────────
    bool         connect();
    void         disconnect();
    bool         isConnected() const;
    bool         notifyJobReady(const std::string& path);
    MachineState machineState() const;
    int          currentLine() const;

    // ── Process / window control ─────────────────────────────────────
    // Used by Mod/MilCAM Send-to-CodeSys command to raise the HMI window
    // after dropping a job. Implemented via wmctrl on Linux/X11.
    static bool raiseWindow(const std::string& titleSubstring);

private:
    struct Impl;
    Impl* impl_;
};

} // namespace MilCAM
