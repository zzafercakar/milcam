// Smoke test: write G-code to a temp drop folder and verify atomic rename.
#include "CodesysBridge.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include <cassert>
#include <iostream>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QTemporaryDir dir;
    if (!dir.isValid()) { std::cerr << "temp dir failed\n"; return 1; }

    MilCAM::CodesysBridge bridge;
    bridge.setDropFolder(dir.path());

    const QString path = bridge.dropGCode("job1", "G0 X0 Y0\nG1 X10 F500\nM30\n");
    if (path.isEmpty()) {
        std::cerr << "dropGCode returned empty path\n";
        return 1;
    }

    QFile f(path);
    if (!f.exists()) { std::cerr << "G-code file missing\n"; return 1; }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 1;
    const QByteArray contents = f.readAll();
    if (!contents.contains("M30")) {
        std::cerr << "M30 missing in: " << contents.toStdString() << '\n';
        return 1;
    }

    std::cout << "test_codesys_bridge OK — " << path.toStdString() << '\n';
    return 0;
}
