// milcam/app/src/main.cpp
#include "gui/MainWindow.h"
#include "gui/CncBackdrop.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <fcntl.h>
#include <unistd.h>

// The CNC HMI frame captured at launch (see CncBackdrop.h). Defined here.
std::vector<unsigned char>& cncBackdrop() { static std::vector<unsigned char> b; return b; }

// Read the current /dev/fb0 contents (the CodeSYS CNC HMI on screen right now)
// BEFORE Qt paints over it, so MilCAM can restore it on exit.
static void captureCncBackdrop() {
    int fb = ::open("/dev/fb0", O_RDONLY);
    if (fb < 0) return;
    std::vector<unsigned char> buf(1024 * 768 * 4);   // 1024x768 x 32bpp
    ssize_t off = 0, n = (ssize_t)buf.size();
    while (off < n) {
        ssize_t r = ::read(fb, buf.data() + off, n - off);
        if (r <= 0) break;
        off += r;
    }
    if (off == n) cncBackdrop().swap(buf);            // keep only a full, valid frame
    ::close(fb);
}

int main(int argc, char** argv) {
    captureCncBackdrop();                 // grab the CNC HMI before Qt touches fb0

    QApplication app(argc, argv);
    QApplication::setApplicationName("MilCAM");
    QApplication::setApplicationVersion("0.3.0");
    QApplication::setOrganizationName("SMB Technics");

    // Pin a font that exists on the device (linuxfb has no Helvetica -> box engine).
    app.setFont(QFont("DejaVu Sans", 11));

    QFile qss(":/style.qss");             // compiled-in stylesheet
    if (qss.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow w;
    // Kiosk panel: fullscreen maps directly to the linuxfb framebuffer surface.
    w.showFullScreen();
    return app.exec();
}
