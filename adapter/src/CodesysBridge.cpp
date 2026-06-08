// SPDX-License-Identifier: Proprietary
// CodesysBridge implementation — initial scaffold.
//
// Wiring plan (see .ai/CODESYS_BRIDGE.md):
//   * dropGCode: implemented now — pure filesystem, no PLC dep.
//   * raiseSelfWindow: implemented now via wmctrl / qprocess.
//   * connect / notifyJobReady / state subscriptions: stub until open62541 is
//     introduced in Faz 3.

#include "CodesysBridge.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>

namespace MilCAM {

struct CodesysBridge::Impl {
    QString      endpointUrl = QStringLiteral("opc.tcp://192.168.1.123:4840");
    QString      dropFolder  = QStringLiteral("/var/cnc/jobs");
    bool         connected   = false;
    MachineState state       = MachineState::Unknown;
    int          currentLine = 0;
};

CodesysBridge::CodesysBridge(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>()) {}

CodesysBridge::~CodesysBridge() = default;

bool         CodesysBridge::isConnected() const { return impl_->connected; }
QString      CodesysBridge::endpointUrl() const { return impl_->endpointUrl; }
QString      CodesysBridge::dropFolder()  const { return impl_->dropFolder; }
MachineState CodesysBridge::machineState() const { return impl_->state; }
int          CodesysBridge::currentLine() const { return impl_->currentLine; }

void CodesysBridge::setEndpointUrl(const QString& url) {
    if (impl_->endpointUrl == url) return;
    impl_->endpointUrl = url;
    Q_EMIT endpointUrlChanged(url);
}

void CodesysBridge::setDropFolder(const QString& path) {
    if (impl_->dropFolder == path) return;
    impl_->dropFolder = path;
    Q_EMIT dropFolderChanged(path);
}

bool CodesysBridge::connect() {
    // STUB: open62541 client_connect goes here.
    impl_->connected = false;
    Q_EMIT plcMessage("info", QStringLiteral("OPC UA client not yet wired (Faz 3)."));
    return false;
}

void CodesysBridge::disconnect() {
    if (!impl_->connected) return;
    impl_->connected = false;
    Q_EMIT connectionChanged(false);
}

QString CodesysBridge::dropGCode(const QString& jobId, const QString& gcode) {
    QDir dir(impl_->dropFolder);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            Q_EMIT plcMessage("error",
                QStringLiteral("Cannot create drop folder: %1").arg(impl_->dropFolder));
            return {};
        }
    }
    const QString finalName = QStringLiteral("%1.gcode").arg(jobId);
    const QString tmpName   = finalName + QStringLiteral(".tmp");
    const QString finalPath = dir.filePath(finalName);
    const QString tmpPath   = dir.filePath(tmpName);

    QFile f(tmpPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        Q_EMIT plcMessage("error",
            QStringLiteral("Cannot write %1: %2").arg(tmpPath, f.errorString()));
        return {};
    }
    {
        QTextStream out(&f);
        out << "; MilCAM job " << jobId << '\n'
            << "; generated " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << '\n'
            << gcode;
    }
    f.close();

    // Atomic rename — POSIX guarantees readers see either old or new file
    QFile::remove(finalPath);
    if (!QFile::rename(tmpPath, finalPath)) {
        Q_EMIT plcMessage("error",
            QStringLiteral("Atomic rename failed: %1 -> %2").arg(tmpPath, finalPath));
        return {};
    }
    return finalPath;
}

bool CodesysBridge::notifyJobReady(const QString& path) {
    // STUB: write OPC UA symbol "JobReadyPath" once client is wired.
    if (!impl_->connected) {
        Q_EMIT plcMessage("warn",
            QStringLiteral("notifyJobReady(%1) ignored: PLC not connected.").arg(path));
        return false;
    }
    return true;
}

bool CodesysBridge::raiseSelfWindow() {
    QProcess p;
    p.start(QStringLiteral("wmctrl"), { QStringLiteral("-a"), QStringLiteral("MilCAM") });
    if (!p.waitForFinished(1000)) return false;
    return p.exitCode() == 0;
}

} // namespace MilCAM
