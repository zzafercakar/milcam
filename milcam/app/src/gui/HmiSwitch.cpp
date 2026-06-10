// milcam/app/src/gui/HmiSwitch.cpp
#include "gui/HmiSwitch.h"
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
    // Clear the framebuffer to white BEFORE exiting so no MilCAM imagery is left
    // for the CodeSYS HMI to paint over. CodeSYS only repaints its own element
    // regions (partial), so without this its idle/background areas would still
    // show MilCAM. The CodeSYS HMI background is light, so white blends in and
    // the partial repaint then looks like a clean return. (Belt-and-suspenders
    // with the CodeSYS-side full repaint — see .ai/CODESYS_RETURN_BUTTON.md.)
    int fb = ::open("/dev/fb0", O_WRONLY);
    if (fb >= 0) {
        // 1024x768 x 32bpp (BGRX), stride == width*4 → one contiguous white fill.
        std::vector<unsigned char> white(1024 * 768 * 4, 0xFF);
        ssize_t off = 0, n = (ssize_t)white.size();
        while (off < n) {
            ssize_t w = ::write(fb, white.data() + off, n - off);
            if (w <= 0) break;
            off += w;
        }
        ::close(fb);
    }
    // Exit MilCAM; the CodeSYS side detects the exit and forces its repaint.
    QApplication::quit();
}
