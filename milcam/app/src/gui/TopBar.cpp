// milcam/app/src/gui/TopBar.cpp
#include "gui/TopBar.h"
#include "gui/HmiSwitch.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QIcon>

namespace {
QToolButton* action(QWidget* parent, const QString& icon, const QString& text) {
    auto* b = new QToolButton(parent);
    b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    b->setIcon(QIcon(":/icons/" + icon + ".svg"));
    b->setIconSize(QSize(26, 26));
    b->setText(text);
    return b;
}
}  // namespace

TopBar::TopBar(QWidget* parent) : QWidget(parent) {
    setObjectName("TopBar");
    setFixedHeight(64);
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    auto* brand = new QLabel("◆ MilCAM", this);
    brand->setObjectName("Brand");
    lay->addWidget(brand);

    lay->addSpacing(16);
    lay->addWidget(action(this, "new", "New"));
    lay->addWidget(action(this, "open", "Open"));
    lay->addWidget(action(this, "save", "Save"));
    lay->addWidget(action(this, "post", "Post"));

    lay->addStretch(1);                  // pushes the switch cluster to the right
    lay->addWidget(new HmiSwitch(this)); // fixed top-right switch
}
