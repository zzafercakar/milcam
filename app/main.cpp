// SPDX-License-Identifier: Proprietary
// MilCAM — Touch-first CAM tool for CodeSys CNC panel PCs (VEC-VE class).
//
// Bootstraps:
//   - GLX OpenGL surface (OCCT requirement on Linux)
//   - Single-instance lock (so the launcher bar can wmctrl-raise the existing
//     window instead of starting a second copy)
//   - QML engine with VirtualKeyboard, CamFacade, ImportFacade, CodesysBridge
//     exposed as singletons.

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QLockFile>
#include <QDir>
#include <QStandardPaths>
#include <QLocale>
#include <QTranslator>
#include <QtQml/qqmlextensionplugin.h>

#include "AppVersion.h"
#include "CamFacade.h"
#include "ImportFacade.h"
#include "CodesysBridge.h"

#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <cstring>

static void crashHandler(int sig) {
    const char* msg = (sig == SIGSEGV) ? "\n=== MilCAM SIGSEGV ===\n"
                    : (sig == SIGABRT) ? "\n=== MilCAM SIGABRT ===\n"
                    : (sig == SIGFPE ) ? "\n=== MilCAM SIGFPE  ===\n"
                                       : "\n=== MilCAM crash    ===\n";
    (void)write(STDERR_FILENO, msg, std::strlen(msg));
    void* frames[64];
    int n = backtrace(frames, 64);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    std::_Exit(1);
}

int main(int argc, char* argv[]) {
    std::signal(SIGSEGV, crashHandler);
    std::signal(SIGABRT, crashHandler);
    std::signal(SIGFPE,  crashHandler);

    // OCCT GLX requirement — DO NOT switch to EGL on Linux.
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
    qputenv("QT_IM_MODULE",    QByteArrayLiteral("qtvirtualkeyboard"));

    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QGuiApplication app(argc, argv);
    app.setApplicationName("MilCAM");
    app.setApplicationVersion(MILCAM_APP_VERSION);
    app.setOrganizationName("MilCAM");

    // Single-instance enforcement — the launcher bar should wmctrl-raise rather
    // than fork a second process. Lock lives under XDG runtime dir.
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                           + QStringLiteral("/milcam.lock");
    QLockFile lock(lockPath);
    lock.setStaleLockTime(0);
    if (!lock.tryLock(100)) {
        qWarning("MilCAM is already running; raising existing window.");
        MilCAM::CodesysBridge::raiseSelfWindow();
        return 0;
    }

    QQmlApplicationEngine engine;

    MilCAM::ImportFacade  importer;
    MilCAM::CodesysBridge plc;
    // CamFacade is reused from the CADNC adapter (see adapter/inc/CamFacade.h).

    engine.rootContext()->setContextProperty("ImportFacade",  &importer);
    engine.rootContext()->setContextProperty("CodesysBridge", &plc);
    engine.rootContext()->setContextProperty("appVersion",
        QStringLiteral(MILCAM_APP_VERSION));

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qFatal("MilCAM: failed to load qrc:/qml/Main.qml");
        return 1;
    }

    return app.exec();
}

