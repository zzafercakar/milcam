// milcam/app/src/main.cpp
#include "gui/MainWindow.h"
#include "gui/CncBackdrop.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QTimer>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

// The CNC HMI frame captured at launch (see CncBackdrop.h). Defined here.
std::vector<unsigned char>& cncBackdrop() { static std::vector<unsigned char> b; return b; }

// Signal handler to trigger graceful return-to-CNC on SIGTERM/SIGINT (e.g., killall -TERM).
// Sets a flag; the main loop checks it and triggers qApp->quit() which calls the cleanup handlers.
static std::atomic<bool> shutdown_requested(false);
static void signal_handler(int sig) { shutdown_requested = true; }

// Restore the saved CNC frame to fb0 and exit cleanly.
static void restore_cnc_and_exit() {
    const std::vector<unsigned char>& bg = cncBackdrop();
    int fb = ::open("/dev/fb0", O_WRONLY);
    if (fb >= 0) {
        const size_t frame = 1024u * 768u * 4u;
        std::vector<unsigned char> white;
        const unsigned char* data;
        size_t n;
        if (bg.size() == frame) { data = bg.data(); n = frame; }
        else { white.assign(frame, 0xFF); data = white.data(); n = frame; }
        size_t off = 0;
        while (off < n) {
            ssize_t w = ::write(fb, data + off, n - off);
            if (w <= 0) break;
            off += (size_t)w;
        }
        ::close(fb);
    }
    std::exit(0);
}

// Load the CNC HMI frame from a saved file, or capture from /dev/fb0 if not available.
// Saves it at /root/milcam/cnc_bg.raw (created manually once per device, when HMI looks clean).
// This avoids restoring corrupted frames if the capture happened during chaos.
static void captureCncBackdrop() {
    const char* saved_path = "/root/milcam/cnc_bg.raw";
    const size_t frame_size = 1024u * 768u * 4u;
    std::vector<unsigned char> buf(frame_size);

    // Try to load from saved file first.
    int f = ::open(saved_path, O_RDONLY);
    if (f >= 0) {
        ssize_t off = 0, n = (ssize_t)buf.size();
        while (off < n) {
            ssize_t r = ::read(f, buf.data() + off, n - off);
            if (r <= 0) break;
            off += r;
        }
        ::close(f);
        if (off == (ssize_t)frame_size) {
            cncBackdrop().swap(buf);
            return;
        }
    }

    // Fallback: capture /dev/fb0 at launch time (the CodeSYS CNC HMI on screen right now).
    int fb = ::open("/dev/fb0", O_RDONLY);
    if (fb < 0) return;
    ssize_t off = 0, n = (ssize_t)frame_size;
    while (off < n) {
        ssize_t r = ::read(fb, buf.data() + off, n - off);
        if (r <= 0) break;
        off += r;
    }
    if (off == n) cncBackdrop().swap(buf);
    ::close(fb);
}

int main(int argc, char** argv) {
    captureCncBackdrop();                 // grab the CNC HMI before Qt touches fb0

    QApplication app(argc, argv);
    QApplication::setApplicationName("MilCAM");
    QApplication::setApplicationVersion("0.3.0");
    QApplication::setOrganizationName("SMB Technics");

    // Install signal handler for graceful shutdown (e.g., killall -TERM milcam).
    ::signal(SIGTERM, signal_handler);
    ::signal(SIGINT, signal_handler);

    // Pin a font that exists on the device (linuxfb has no Helvetica -> box engine).
    app.setFont(QFont("DejaVu Sans", 11));

    QFile qss(":/style.qss");             // compiled-in stylesheet
    if (qss.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    MainWindow w;
    // Kiosk panel: fullscreen maps directly to the linuxfb framebuffer surface.
    w.showFullScreen();

    // Timer to check for shutdown signal and restore CNC frame before exiting.
    QTimer shutdown_timer;
    shutdown_timer.setInterval(100);
    QObject::connect(&shutdown_timer, &QTimer::timeout, [&]() {
        if (shutdown_requested) {
            shutdown_timer.stop();
            restore_cnc_and_exit();
        }
    });
    shutdown_timer.start();

    return app.exec();
}
