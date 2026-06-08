// SPDX-License-Identifier: Proprietary
// CodesysBridge — MilCAM ↔ CodeSys runtime communication.
//
// Two channels (see .ai/CODESYS_BRIDGE.md for the full protocol):
//
//   1. G-code drop folder
//        MilCAM writes /var/cnc/jobs/<id>.gcode (atomic rename).
//        PLC reads via SMC_ReadNCFile2 + SMC_NCInterpreter.
//
//   2. OPC UA control/status
//        MilCAM is an OPC UA client (open62541 under the hood). It subscribes
//        to machine state symbols (MachineState, CurrentLine, EStop) and writes
//        a "JobReady" symbol with the path to the freshly dropped file.
//
// Both channels are optional — MilCAM still functions as a standalone CAM tool
// without a panel PC reachable.

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <memory>

namespace MilCAM {

enum class MachineState {
    Unknown = 0,
    Idle,
    Running,
    Paused,
    Error,
    EStop
};

class CodesysBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString endpointUrl READ endpointUrl WRITE setEndpointUrl NOTIFY endpointUrlChanged)
    Q_PROPERTY(QString dropFolder READ dropFolder WRITE setDropFolder NOTIFY dropFolderChanged)
    Q_PROPERTY(MachineState machineState READ machineState NOTIFY machineStateChanged)
    Q_PROPERTY(int currentLine READ currentLine NOTIFY currentLineChanged)

public:
    explicit CodesysBridge(QObject* parent = nullptr);
    ~CodesysBridge() override;

    bool         isConnected() const;
    QString      endpointUrl() const;
    void         setEndpointUrl(const QString& url);
    QString      dropFolder() const;
    void         setDropFolder(const QString& path);
    MachineState machineState() const;
    int          currentLine() const;

    Q_INVOKABLE bool connect();
    Q_INVOKABLE void disconnect();

    // Atomic write: writes to <dropFolder>/<id>.gcode.tmp then renames.
    // Returns the final on-disk path, or empty on failure.
    Q_INVOKABLE QString dropGCode(const QString& jobId, const QString& gcode);

    // Tell the PLC "new job available at <path>" via OPC UA. The PLC HMI's
    // "Open Job" button reacts to this. No-op if not connected.
    Q_INVOKABLE bool notifyJobReady(const QString& path);

    // Optional: bring the MilCAM window to the foreground from CodeSys side.
    // Implemented via wmctrl on Linux/X11. Used by TargetVisu's "Open CAM"
    // button which calls SysProcessExecuteCommand("wmctrl -a MilCAM").
    Q_INVOKABLE static bool raiseSelfWindow();

Q_SIGNALS:
    void connectionChanged(bool connected);
    void endpointUrlChanged(const QString& url);
    void dropFolderChanged(const QString& path);
    void machineStateChanged(MachineState state);
    void currentLineChanged(int line);
    void plcMessage(const QString& level, const QString& text);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace MilCAM
