// milcam/app/src/main.cpp
#include "gui/MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QFont>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("MilCAM");
    QApplication::setApplicationVersion("0.3.0");
    QApplication::setOrganizationName("SMB Technics");

    // Pin a font that exists on the device (linuxfb has no Helvetica -> box engine).
    app.setFont(QFont("DejaVu Sans", 11));

    QFile qss(":/style.qss");                 // compiled-in stylesheet
    if (qss.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow w;
    // Kiosk panel: fullscreen maps directly to the linuxfb framebuffer surface.
    w.showFullScreen();
    return app.exec();
}
