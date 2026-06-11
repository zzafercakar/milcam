// milcam/app/src/gui/HmiSwitch.cpp
#include "gui/HmiSwitch.h"
#include "gui/CncBackdrop.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QIcon>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

HmiSwitch::HmiSwitch(QWidget* parent) : QWidget(parent) {
    setObjectName("SwitchCluster");
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    cncBtn_ = new QToolButton(this);
    cncBtn_->setObjectName("HmiButton");
    cncBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    cncBtn_->setIcon(QIcon(":/icons/hmi.svg"));
    cncBtn_->setIconSize(QSize(24, 24));
    cncBtn_->setText("CNC");
    connect(cncBtn_, &QToolButton::clicked, this, &HmiSwitch::returnToCnc);
    lay->addWidget(cncBtn_);
}

void HmiSwitch::returnToCnc() {
    // Restore the CNC HMI by writing the framebuffer frame captured at launch back
    // to /dev/fb0, then exit. This returns the operator to the exact CodeSYS CNC
    // screen that was showing before MilCAM opened — without relying on CodeSYS to
    // repaint (which is unreliable to trigger here). CodeSYS keeps the live elements
    // (DRO, status) updated on top afterwards. Falls back to white if no capture.
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
    QApplication::quit();
}
