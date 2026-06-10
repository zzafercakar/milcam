// milcam/app/src/gui/StatusBar.cpp
#include "gui/StatusBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>

StatusBar::StatusBar(QWidget* parent) : QWidget(parent) {
    setObjectName("StatusBar");
    setFixedHeight(56);
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 4, 12, 4);
    lay->addWidget(new QLabel("mm", this));
    lay->addWidget(new QLabel(" ·  X: 0.000   Y: 0.000", this));
    lay->addStretch(1);
    lay->addWidget(new QLabel("Post: CODESYS ✓", this));
    lay->addStretch(1);
    auto* send = new QPushButton(QIcon(":/icons/send.svg"), "SEND G-CODE", this);
    send->setObjectName("SendButton");
    send->setIconSize(QSize(24, 24));
    send->setEnabled(false);            // enabled once a job exists (P3)
    lay->addWidget(send);
}
