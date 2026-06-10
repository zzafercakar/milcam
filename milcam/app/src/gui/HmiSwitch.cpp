// milcam/app/src/gui/HmiSwitch.cpp
#include "gui/HmiSwitch.h"
#include "hmi/VtSwitch.h"            // milcam::vtSwitchProgram / kCodesysVt
#include <QToolButton>
#include <QHBoxLayout>
#include <QProcess>
#include <QIcon>

HmiSwitch::HmiSwitch(QWidget* parent) : QWidget(parent) {
    setObjectName("SwitchCluster");
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    hmiBtn_ = new QToolButton(this);
    hmiBtn_->setObjectName("HmiButton");
    hmiBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    hmiBtn_->setIcon(QIcon(":/icons/hmi.svg"));
    hmiBtn_->setIconSize(QSize(28, 28));
    hmiBtn_->setText("HMI");
    connect(hmiBtn_, &QToolButton::clicked, this, &HmiSwitch::switchToCodesys);
    lay->addWidget(hmiBtn_);
}

void HmiSwitch::switchToCodesys() {
    // Activate the CodeSYS VT. startDetached: fire-and-forget; MilCAM keeps
    // running on its own VT and is shown again by the CodeSYS-side chvt button.
    QProcess::startDetached(
        QString::fromStdString(milcam::vtSwitchProgram()),
        { QString::fromStdString(milcam::vtSwitchArg(milcam::kCodesysVt)) });
}
