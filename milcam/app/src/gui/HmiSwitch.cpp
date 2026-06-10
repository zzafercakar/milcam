// milcam/app/src/gui/HmiSwitch.cpp
#include "gui/HmiSwitch.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QIcon>

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
    // Exit MilCAM so the CodeSYS HMI can reclaim the framebuffer. The CodeSYS
    // side detects the exit and forces a full TargetVisu repaint (it does not
    // auto-repaint on its own). CodeSYS re-launches MilCAM on demand.
    QApplication::quit();
}
