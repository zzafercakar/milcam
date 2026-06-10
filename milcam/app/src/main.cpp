// main.cpp — MilCAM application entry point
// Targets linuxfb (no GL) on the SMB Technics aarch64 panel PC.
// The panel is a kiosk device: show fullscreen immediately, no window chrome.

#include <QApplication>
#include "gui/MainWindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // Metadata used by QSettings, about dialogs, and OS window managers.
    QApplication::setApplicationName("MilCAM");
    QApplication::setApplicationVersion("0.3.0");
    QApplication::setOrganizationName("SMB Technics");
    QApplication::setOrganizationDomain("smbtechnics.com");

    MainWindow w;

    // On the panel PC the app is the only visible program (kiosk mode).
    // showFullScreen() also works correctly with linuxfb / eglfs where
    // the concept of "window" maps directly to the framebuffer surface.
    w.showFullScreen();

    return app.exec();
}
